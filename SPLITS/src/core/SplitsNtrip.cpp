/*
 *Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <cstring>

#include "SplitsNtrip.hpp"
#include "utils.h"
#include "Logger.hpp"

#define RESP_BUFFER_SIZE    1032
#define SOCKET_READ_TO    5
#define RETRY_COUNT    10
#define ACK_STRING "ICY 200 OK\r\n"
#define RTCM_MSGPOOL_SIZE   5
uint8_t truncBuffer[RESP_BUFFER_SIZE];
bool append = false;
int appendOffset = 0;

namespace SPLITS {

namespace Core {

SplitsNtrip::SplitsNtrip(std::shared_ptr<SplitsConfig::RTCMConfig> config) {
    config_ = config;
}
int SplitsNtrip::init(void)
{
    std::string::size_type pos;
    LOGD(__FUNCTION__, " HostName", config_->getHostName());
    pos= config_->getHostName().find("file:");
    if (pos != std::string::npos) {
        //using a file as RTCM source, for testing only.
        auto fileName = config_->getHostName().substr(pos + 5);
        sourceFp_ = std::fopen(fileName.c_str(), "r");
        if (!sourceFp_) {
            LOGD(__FUNCTION__, " failed to open file ", fileName);
            return -1;
        }
        LOGD(__FUNCTION__, " succesfully opened file: ", fileName);
        ntripThread_ = std::thread([this]() {
            while(!stop_) {
                if (processRtcmFromFile() < 0) {
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        });
    } else {
        ntripThread_ = std::thread([this](){
            this->connectNtrip();
        });
    }

    return 0;
}
void SplitsNtrip::deinit(void)
{
    stop_ = true;
    if (ntcSocketFd_ > 0) {
        close(ntcSocketFd_);
    }
    ntripThread_.join();
}
#if 0
Status SplitsNtrip::fillRTCM(std::shared_ptr<v2xMsgSae> msg)
{
    std::shared_ptr<J2735::idl::RTCMcorrection> sp;
    if (msg == nullptr) {
        return Status::INVALIDPARAM;
    }
    if ((sp = msg->RTCMcorrection()) == nullptr) {
        sp = std::make_shared<J2735::idl::RTCMcorrection>();
        if (sp == nullptr) {
            return Status::NOMEMORY;
        }
        msg->RTCMcorrection(sp);
    }
    sp->rev(J2735::idl::rtcmv_rtcmRev3);
    sp->timeStamp(get_minute_of_the_year());
    {
        std::lock_guard<std::mutex> lk(msgMtx_);
        sp->msgCnt(rtcmMsgQueue_.size());
        J2735::idl::RTCMmessageList idlMsgList;
        while(rtcmMsgQueue_.empty() == false) {
            auto msg = rtcmMsgQueue_.front();
            rtcmMsgQueue_.pop();
            idlMsgList.push_back(*msg);
            LOGD(__FUNCTION__, " msg size:", msg->size());
            rtcmMsgPool_.push(msg);
        }
        sp->msgs(std::move(idlMsgList));
    }
    return Status::SUCCESS;
}
#endif
int SplitsNtrip::connectNtrip(void)
{
    struct sockaddr_in server_addr;
    int flags, ret;
    std::string con_request;
    std::string configFile;
    char response[RESP_BUFFER_SIZE];

    while(stop_ == false) {
        ntcSocketFd_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (ntcSocketFd_ < 0) {
            LOGD(__FUNCTION__, " SplitsNtrip::connectNtrip: Socket create failed");
            return -1;
        }
        std::memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(config_->getHostName().c_str());
        server_addr.sin_port = htons(config_->getPort());

        LOGD(__FUNCTION__, " Connecting to server: ",
             config_->getHostName(), ":", config_->getPort());
        ret = connect(ntcSocketFd_, (struct sockaddr *)&server_addr, sizeof(server_addr));
        LOGD(__FUNCTION__, " returned");
        if (ret < 0) {
            LOGD(__FUNCTION__, " connection failed, retry after ", RETRY_COUNT, "sec");
            usleep(RETRY_COUNT * 1000000);
            close(ntcSocketFd_);
            continue;
        } else {
            LOGD(__FUNCTION__, " connection success");
        }

        std::memset(&response, 0 , sizeof(response));
        con_request = "GET /" + config_->getMountPoint() +
              " HTTP/1.1\r\n" + "User-Agent: NTRIP GNR/1.0.0 (Win32)\r\n" +
              "Authorization: Basic " +
              config_->getUserNamePwdInBase64Format() +
              "\r\nConnection: close\r\n";
        ret = send(ntcSocketFd_, con_request.c_str(), con_request.size(), 0);

        if (ret < 0) {
            close(ntcSocketFd_);
            LOGD(__FUNCTION__, " SplitsNtrip::connectNtrip: send failed: ", ret);
            return -1;
        }
        if (config_->getNmeaGGAInterval()) {
            ret = sendGGAString();
            //check ret
        }

        // set for nonblocking socket
        flags = fcntl(ntcSocketFd_,F_GETFL,0);
        if (flags < 0) {
            LOGE(__FUNCTION__, " SplitsNtrip::connectNtrip: fcntl returned error");
            return -1;
        }
        fcntl(ntcSocketFd_, F_SETFL, flags | O_NONBLOCK);
        ret = waitForSock(ntcSocketFd_);
        if (ret <= 0) {
            if (reconnect_ == true) {
                close(ntcSocketFd_);
                reconnect_ = false;
                continue;
            }
        }

        ret = recv(ntcSocketFd_, response, sizeof(response), 0);
        if(ret < 0 ) {
            LOGE(__FUNCTION__, " recv failed");
            close(ntcSocketFd_);
            return -1;
        } else if (ret > 0 && !strncmp(ACK_STRING, (char*)response, 12)) {
            // Please refer to injectFromFile() for alternative use case sample.
            while (ret && reconnect_ == false) {
                ret = processRtcmFromServer();
            }
            if (reconnect_ == true) {
                close(ntcSocketFd_);
                reconnect_ = false;
                continue;
            }
        } else {
            LOGE(__FUNCTION__, " Initial response invalid: ", response);
            close(ntcSocketFd_);
            return -1;
        }
    }
    return 0;
}
int SplitsNtrip::waitForSock(int fd) {
    fd_set rfds;
    struct timeval tv;
    int ret;
    int retryCount = 0;
    tv.tv_sec = SOCKET_READ_TO;
    tv.tv_usec = 0;

    while(retryCount < RETRY_COUNT) {
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        ret = select(fd + 1, &rfds, NULL, NULL, &tv);
        if (ret < 0 ) {
            if (errno == -EINTR) {
                LOGD(__FUNCTION__, " select interrupted, continue...");
                continue;
            } else {
                LOGD(__FUNCTION__, " select returned error, existing..");
                stop_ = true;
                break;
            }
        } else if (ret == 0) {
            //time out and no data
            retryCount++;
        } else {
            // data ready to read
            break;
        }
    }

    if (retryCount == RETRY_COUNT && ret == 0) {
        reconnect_ = true;
    }
    return ret;
}

int SplitsNtrip::processRtcmFromServer(void) {
    int i = 0, length = 0;
    uint8_t buffer[RESP_BUFFER_SIZE];
    static int msg_type;
    int ret;

    memset(buffer, 0, sizeof(buffer));
    ret = waitForSock(ntcSocketFd_);

    if (ret <= 0) {
        return ret;
    }

    int nmeaGGAInterval = config_->getNmeaGGAInterval();
    if (nmeaGGAInterval) {
        auto TimeNow = std::chrono::system_clock::now();
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(TimeNow -
                lastNmeaSentTime_);
        if (static_cast<int>(elapsed_ms.count()) >= nmeaGGAInterval) {
            ret = sendGGAString();
        }

    }

    ret = recv(ntcSocketFd_, buffer, sizeof(buffer), 0);
    if (ret < 0) {
        LOGE(__FUNCTION__, " ReadRtcmPacket: recv failed ", ret);
        return ret;
    }

    if (append) {
         int totalSize = appendOffset + ret;
         std::memcpy(truncBuffer + appendOffset, buffer, ret);

         append = false;
         appendOffset = 0;
         onMsgAvailable(truncBuffer, totalSize);
#if 0
         {
             std::lock_guard<std::mutex> lk(msgMtx_);
             if (rtcmMsgQueue_.size() < RTCM_MSGPOOL_SIZE) {
                 auto msg = rtcmMsgPool_.front();
                 rtcmMsgPool_.pop();
                 msg->clear();
                 msg->assign(truncBuffer, truncBuffer + totalSize);
                 rtcmMsgQueue_.push(msg);

             }
         }
#endif
     } else {
        for (i = 0; i < (ret - 4);) {
            if ((buffer[i] == 0xD3) && ((buffer[i+1] & 0xFC) == 0x00)) {
                //Found RTCM preamble.
                length = buffer[i + 2];
                length |= (buffer[i + 1] & 0x03) << 8;
                msg_type = buffer[i + 3] << 4;
                msg_type |= buffer[i + 4] >> 4;

                if (ret < length + i + 6) {
                    //didn't read the whole packet, portion will be available for next recv call
                    //save current packet to truncBuffer and append it to next recv'd bytes.
                    memset(truncBuffer, 0, sizeof(truncBuffer));
                    memcpy(truncBuffer, buffer + i, ret - i);
                    append = true;
                    appendOffset = ret - i;
                    i += appendOffset;
                } else {
                    onMsgAvailable(buffer + i, length + 6);
#if 0
                    {
                        std::lock_guard<std::mutex> lk(msgMtx_);
                        if (rtcmMsgQueue_.size() == RTCM_MSGPOOL_SIZE) {
                            //queue is full, msgs are not broadcasted out, we drop the newly received msgs
                            return ret;
                        }
                        auto msg = rtcmMsgPool_.front();
                        rtcmMsgPool_.pop();
                        msg->clear();
                        msg->assign(buffer + i, buffer + i + length + 6);
                        rtcmMsgQueue_.push(msg);
                    }
#endif

                    i += length + 6;
                }
            } else {
                i += 1;
            }
        }
    }
    return ret;
}
int SplitsNtrip::processRtcmFromFile(void)
{
    int size = 0;
    int ret;
    uint8_t temp[2];
    uint8_t buffer[RESP_BUFFER_SIZE];
    uint8_t *p = buffer;
    memset(buffer, 0, sizeof(buffer));
    do {
       ret = std::fread(temp, 2, 1, sourceFp_);
       if (temp[0] == '\r' && temp[1] == '\n') {
          break;
       } else {
          *p = temp[0];
          *(p+1) = temp[1];
          p += 2;
          size += 2;
       }
    } while(size < RESP_BUFFER_SIZE);

    if (ret < 2) {
        if (std::feof(sourceFp_)) {
            LOGD(__FUNCTION__, " End of file reached, rewinding");
            std::rewind(sourceFp_);
            return 0;
        } else if (std::ferror(sourceFp_)) {
            LOGD(__FUNCTION__, " file read error");
            return -1;
        }
    }
    onMsgAvailable(buffer, size);
#if 0
    {
        std::lock_guard<std::mutex> lk(msgMtx_);
        if (rtcmMsgQueue_.size() == RTCM_MSGPOOL_SIZE) {
            //queue is full, msgs are not broadcasted out, we drop the newly received msgs
            return 0;
        }
        auto msg = rtcmMsgPool_.front();
        rtcmMsgPool_.pop();
        msg->clear();
        LOGD(__FUNCTION__, " copy data to queue: ", size);
        msg->assign(buffer, buffer + size);
        rtcmMsgQueue_.push(msg);
    }
#endif

    return 0;
}
} //namespace Core
} //namespace SPLITS
