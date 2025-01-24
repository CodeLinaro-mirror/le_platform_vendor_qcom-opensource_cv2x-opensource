/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

 /**
 * @file       AsyncTaskQueue.hpp
 *
 * @brief      Implements a queue that will hold on futures for async tasks. This allows async
 *             tasks to be created from within class methods and prevents the task from
 *             blocking.
 *
 */

#ifndef __ASYNCTASKQUEUE_HPP__
#define __ASYNCTASKQUEUE_HPP__

#include <atomic>
#include <condition_variable>
#include <deque>
#include <future>
#include <memory>
#include <mutex>

#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace common {

template <typename T>
class AsyncTaskQueue {
 public:
    AsyncTaskQueue()
       : orderedTaskThread_(nullptr)
       , shuttingDown_(false) {
    }

    /**
     * This method should be utilized by the user of the AsyncTaskQueue class to retire all the
     * task queue threads. This method should not be called from within the context of a
     * deferred task or an async task to avoid deadlocks and joining thread to itself.
     */
    void shutdown() {
        if (shuttingDown_) {
            return;
        }
        // Shutdown the worker thread
        shuttingDown_ = true;
        {
            std::lock_guard<std::mutex> lock(orderedTasksMutex_);
            orderedTasksCv_.notify_all();
        }
        if (orderedTaskThread_ != nullptr) {
            orderedTaskThread_->join();
        }
        orderedTaskThread_ = nullptr;
        // Wait for all threads to be completed
        while (true) {
            bool wait = false;
            std::deque<std::shared_future<void>>::iterator itr;
        {
            std::lock_guard<std::mutex> lock(tasksMutex_);
            purgeCompleted();
                itr = std::begin(tasksQueue_);
            // Iterate from head of queue and remove if the task is complete
                if (itr != std::end(tasksQueue_)) {
                if (itr->valid()) {
                        wait = true;
                }
                    tasksQueue_.erase(itr);
                } else {
                    break;
            }
        }
            if (wait) {
                itr->wait();
            }
        };
        if (orderedTasksQueue_.size() > 0) {
            std::cout << "Ordered task Queue size on shutdown: "
                    << orderedTasksQueue_.size()
                    << std::endl;
        }
    }

    ~AsyncTaskQueue() {
        // Indicate we are shutting down, notify to release the wait and join before winding up
        shutdown();
    }

    /**
     * This function performs two functions - it first purges completed tasks from the front
     * of the queue, then it adds the new task to the end of the queue. We do this to simplify
     * its use.
     * The task will be executed only if this object is valid when the task is ready for execution.
     * The assumption here is that the task will be accessing this object during execution.
     *
     * @param [in] func - function need to be executed async
     * @param [in] wp - class object this asyncTaskQueue access its members during execution
     * @param [in] policy - specify execute in order or not
     */
    template<class Function, typename ClassName>
    Status add(Function&& func, std::weak_ptr<ClassName> wp, std::launch policy) {
        auto wrapper = [func, wp]() {
            auto sp = wp.lock();
            if (sp) {
                func();
            }
        };

        if (policy == std::launch::deferred) {
            std::lock_guard<std::mutex> lock(orderedTasksMutex_);
            if (shuttingDown_) {
                return Status::NOTALLOWED;
            }
            auto f = std::async(policy, [=]() { wrapper(); }).share();
            orderedTasksQueue_.push_back(f);
            // If the thread does not exit, create one.
            if (orderedTaskThread_ == nullptr) {
                orderedTaskThread_
                    = std::make_shared<std::thread>(&AsyncTaskQueue::executeTask, this);
            }
            orderedTasksCv_.notify_one();
        } else {
            // Handling for an async task. Just add the future for persistence until it is purged
            // at a later point in time
            std::lock_guard<std::mutex> lock(tasksMutex_);
            if (shuttingDown_) {
                return Status::NOTALLOWED;
            }
            auto f = std::async(policy, [=]() { wrapper(); }).share();
            purgeCompleted();
            tasksQueue_.push_back(f);
        }
        return Status::SUCCESS;
    }

    void executeTask() {
        std::shared_future<T> f;
        while (true) {
            {
                std::unique_lock<std::mutex> lock(orderedTasksMutex_);
                orderedTasksCv_.wait(
                    lock, [this] { return (orderedTasksQueue_.size() > 0) || shuttingDown_; });
                if (shuttingDown_) {
                    break;
                }
                f = orderedTasksQueue_.front();
                orderedTasksQueue_.pop_front();
            }
            f.get();
        }
    }

 protected:
    /**
     * Removes completed tasks from the front of the task queue. For performance reasons, it
     * will not remove any completed tasks if there exits an uncompleted task ahead of it in
     * the task queue
     */
    void purgeCompleted() {

        // Set timeout time to now so that we timeout immediately. Unfortunately,
        // futures don't have any methods to immediately find out if it's ready.
        // We always have to supply some timeout.
        std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
        auto itr = std::begin(tasksQueue_);

        // Iterate from head of queue and remove if the task is complete
        while (itr != std::end(tasksQueue_)) {
            bool doRemove = false;
            if (itr->valid()) {
                // If the task has already completed, we can remove it.
                if (std::future_status::ready == itr->wait_until(now)) {
                    doRemove = true;
                }
            } else {
                // If the task is invalid, we'll just assume it's also complete
                // and remove it as well.
                doRemove = true;
            }

            // Remove the task if complete. If not, we'll just exit the loop.
            // Of course, we can just keep on looping and find all tasks that have
            // completed, but then we would be deleting from the middle of a queue
            // and we don't want to do that because it's a more expensive operation
            // than removing from the beginning of the queue.
            if (doRemove) {
                // Debug("  removing task.");
                itr = tasksQueue_.erase(itr);
            } else {
                break;
            }
        }
    }

    std::mutex tasksMutex_;                         // mutex protecting unordered, async queue
    std::deque<std::shared_future<T>> tasksQueue_;  // queue of futures for async tasks

    std::shared_ptr<std::thread> orderedTaskThread_;  // Thread to execute ordered task
    std::mutex orderedTasksMutex_;                    // mutex protecting ordered, deferred queue
    std::condition_variable orderedTasksCv_;  // Condition variable used for waking up the worker
                                              // thread
    std::deque<std::shared_future<T>> orderedTasksQueue_;  // queue of futures for deferred tasks
    std::atomic<bool> shuttingDown_;  // Flag indicating we are about to shutdown
};

}  // namespace common
}  // namespace telux

#endif  // #ifndef __ASYNCTASKQUEUE_HPP__
