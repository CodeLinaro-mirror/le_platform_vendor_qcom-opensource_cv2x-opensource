
/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file SplitsCore.hpp
 *
 * @brief SplitsCore class encapsulate core functionalities
 */
#ifndef SPLITS_CORE_HPP_
#define SPLITS_CORE_HPP_

#include <memory>
#include <condition_variable>
#include <queue>
#include <map>
#include <mutex>
#include <iostream>
#include <mutex>
#include "v2xMsg.hpp"
#include "SplitsCodec.hpp"

#ifndef MIN_PACKET_LEN
#define MIN_PACKET_LEN      20
#endif
#ifndef MAX_PACKET_LEN
#define MAX_PACKET_LEN      8192
#endif
#ifndef CV2X_MAC_ADDR_LEN
#define CV2X_MAC_ADDR_LEN   3
#endif
#define RX_BUF_LEN      8192
#define TX_BUF_LEN      1024

using SPLITS::Codec::SplitsCodec;
using SPLITS::StackType;
using SPLITS::v2xMsg;

namespace SPLITS {

namespace Core {

enum class TransmitType {
    SPS,
    EVENT
};
enum class CoreState {
    NORMAL,
    CRITICAL
};
enum class Role {
    OBU,
    RSU
};

/**
 * This is class encapsulate abstract interface of SPLITS core functionalities.
 */
class SplitsCore {
public:

    virtual ~SplitsCore(){};

    virtual void deinit(void) = 0;

    /**
     * Initialize the core.
     *
     * @param[in] cfgFileName config file name.
     * @param[in] rxpduFileName if string is not empty, use Pdu file for receiving/sending, if
     * rxpdufileName is used, no radio initialization will be performed.
     * @returns 0 on success of negative on failure
     */
    virtual int init(std::string cfgFileName, std::string rxpduFileName, std::string txpduFileName) = 0;
    /**
     * send message.
     *
     * @param[in] serviceId message service id.
     * @return 0 on success or negavie on failure.
     *
     * The function invoke specific message generator, perform encode, sign and send to radio.
     */
    //virtual int send(int serviceId) = 0;
    /**
     * send message.
     *
     * @param[in] serviceId the service ID of the message
     * @param[in] msg the message data to send
     * @return 0 on success or negavie on failure.
     *
     */
    virtual int send(int serviceId, std::shared_ptr<v2xMsg> msg) = 0;

    /**
     * Receive data from radio and decode it, and send it to remote data subscriber.
     *
     * @param[in] index the receive flow index.
     * @param[in] bufLen length of the receiving data buffer.
     *
     * @returns length of the received data on success, 0 or negative if no data is received.
     */
    virtual int receive(const uint8_t index, const uint16_t bufLen) = 0;
    virtual void run(void) = 0;

    virtual void clearRadioInstance() = 0;

    virtual void closeAllRadio() = 0;

    /**
     * Obtain a free msg @ref v2xMsg from the message pool
     *
     * @returns A shared pointer to a v2xMsg data on success, or nullptr if the pool is empty
     */
    virtual std::shared_ptr<v2xMsg> getFreeMsg(void) {
        std::lock_guard<std::mutex> lk(rxPoolMtx_);
        if (rxPool_.empty()) {
            //std::cout<< "rx pool is empty\n";
            return nullptr;
        } else {
            auto msg = rxPool_.front();
            rxPool_.pop();
            //std::cout << "get: rxpool size is: " << rxPool_.size() <<"\n";
            return msg;
        }
    }
    /**
     * Put a free msg @ref v2xMsg back to the message pool
     *
     * @param[in] msg pointer to the message data to be put bac to the pool
     */
    virtual void putFreeMsg(std::shared_ptr<v2xMsg> msg) {
        std::lock_guard<std::mutex> lk(rxPoolMtx_);
        rxPool_.push(msg);
        //std::cout << "put: rxpool size is: " << rxPool_.size() <<"\n";
    }

    /**
     * dequeue a msg @ref v2xMsg from the receiving message pool
     * This function will block if the Rx queue is empty.
     *
     * @returns A shared pointer to a v2xMsg data on success, or nullptr if the queue is empty
     */
    virtual std::shared_ptr<v2xMsg> dequeueMsg(void) {
        std::unique_lock<std::mutex> lk(rxQueueMtx_);
        rxQueueCv_.wait(lk, [this]{return rxQueue_.empty() == false;});
        auto msg = rxQueue_.front();
        rxQueue_.pop();
        //std::cout << "dequeue: rx queue size is: " << rxQueue_.size() <<"\n";
        return msg;
    }
    /**
     * enqueuet a msg @ref v2xMsg to the reciving message queue
     *
     * @param[in] msg pointer to the message data to be queued to the queue
     */
    virtual void enqueueMsg(std::shared_ptr<v2xMsg> msg) {
        std::unique_lock<std::mutex> lk(rxQueueMtx_);
        rxQueue_.push(msg);
        rxQueueCv_.notify_all();
        //std::cout << "enqueue: rx queue size is: " << rxQueue_.size() <<"\n";
    }
protected:
    StackType st_;
    std::map<int, std::shared_ptr<v2xMsg>> txSpsMsgs_;
    std::map<int, std::shared_ptr<v2xMsg>> txEventMsgs_;
    std::mutex stateMutex_;
    CoreState coreState_;
    virtual int initMsgs(void) = 0;

private:
    std::queue<std::shared_ptr<v2xMsg>> rxPool_;
    std::queue<std::shared_ptr<v2xMsg>> rxQueue_;
    std::mutex rxQueueMtx_;
    std::mutex rxPoolMtx_;
    std::condition_variable rxQueueCv_;
};


}; //namespace Core
}; //namespace SPLITS

#endif  //SPLITS_CORE_HPP_
