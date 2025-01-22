/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/**
 * @file SplitsClientDataPublisher
 *
 * @description SPLITS dds data publisher, instantiated from client(EAP) side, this class has  a
 * built in message queue, caller can retrieve an free messsage from the queue using getFreeMsg()
 * function, fill it with desired dds IDL formated data, and queue it using enqueueMsg() function.
 * It will be published to service side by the publisher's publish() method.
 */
#ifndef __SPLITS_CLIENT_DATAPUBLISHER_HPP__
#define __SPLITS_CLIENT_DATAPUBLISHER_HPP__

#include <mutex>
#include <queue>
#include <condition_variable>
#include "v2xMsgSae.hpp"
#include "SplitsDataPublisher.hpp"

namespace SPLITS {
namespace Interface {

//default queue size
#define MSG_POOL_SIZE   16

class SplitsClientDataPublisher : public SplitsDataPublisher
{
public:
    SplitsClientDataPublisher() {}
    virtual ~SplitsClientDataPublisher() {}

    bool init(std::string topicPrefix) override {
        for (int i = 0; i < MSG_POOL_SIZE; i++) {
            auto msg = std::make_shared<v2xMsgSae>();
            //null the unused buffers
            msg->head = msg->buf = 0;
            //msg->frame = 
            putFreeMsg(msg);
        }
        return SplitsDataPublisher::init(topicPrefix);
    }

    /**
     * Obtain a free msg @ref v2xMsg from the message pool
     *
     * @returns A shared pointer to a v2xMsg data on success, or nullptr if the pool is empty
     */
    virtual std::shared_ptr<v2xMsg> getFreeMsg(void) {
        std::lock_guard<std::mutex> lk(poolMtx_);
        if (pool_.empty()) {
            return nullptr;
        } else {
            auto msg = pool_.front();
            pool_.pop();
            return msg;
        }
    }
    /**
     * Put a free msg @ref v2xMsg back to the message pool
     *
     * @param[in] msg pointer to the message data to be put back to the pool
     */
    virtual void putFreeMsg(std::shared_ptr<v2xMsg> msg) {
        std::lock_guard<std::mutex> lk(poolMtx_);
        pool_.push(msg);
    }

    /**
     * dequeue a msg @ref v2xMsg from the receiving message pool
     *
     * @returns A shared pointer to a v2xMsg data on success, or nullptr if the queue is empty
     */
    virtual std::shared_ptr<v2xMsg> dequeueMsg(void) {
        //not locking mutex here, because the dequeue msg is called when
        //cv.wait() return, and queuMtx_ is held automatically
        //std::lock_guard<std::mutex> lk(queueMtx_);
        if (queue_.empty()) {
            return nullptr;
        } else {
            auto msg = queue_.front();
            queue_.pop();
            return msg;
        }
    }
    /**
     * enqueuet a msg @ref v2xMsg to the reciving message queue
     *
     * @param[in] msg pointer to the message data to be queued to the queue
     */
    virtual void enqueueMsg(std::shared_ptr<v2xMsg> msg) {
        std::lock_guard<std::mutex> lk(queueMtx_);
        queue_.push(msg);
        cv_.notify_all();
    }

    void stop(void) override {
        stop_ = true;
        cv_.notify_all();
        td_.join();
    }

private:
    void publish(void) override;
    std::queue<std::shared_ptr<v2xMsg>> pool_;
    std::queue<std::shared_ptr<v2xMsg>> queue_;
    std::mutex queueMtx_;
    std::mutex poolMtx_;
    std::condition_variable cv_;


};

} //namespace Interface
} //namespace SPLITS

#endif //__SPLITS_CLIENT_DATAPUBLISHER_HPP__
