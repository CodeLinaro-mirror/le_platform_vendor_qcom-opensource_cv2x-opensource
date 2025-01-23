/*
 *Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <thread>
#include <bitset>
#include <unistd.h>
#include <sstream>
#include <iomanip>
#include <iostream>

#include <CommonAPI/CommonAPI.hpp>
#include <CommonAPI/AttributeCacheExtension.hpp>

#include "v1/commonapi/SPLITS/SplitsStatisticsProxy.hpp"
#include "v1/commonapi/SPLITS/SplitsStatistics.hpp"

#include "Logger.hpp"

using v1::commonapi::SPLITS::SplitsStatisticsProxy;
using v1::commonapi::SPLITS::SplitsStatistics;

enum class StatisticsType {
    TX,
    RX,
    NUM_OF_TYPE
};

static void printTxStatistics(const SplitsStatistics::TxStatistics& statistics) {
    std::stringstream str;
    str << "Flow Type:               " << statistics.getType().toString() << std::endl;
    str << "Number of Tx Total:      " << statistics.getTxNumber() << std::endl
        << "          Failed:        " << statistics.getTxFailed() << std::endl
        << "          Encode Failed: " << statistics.getEncodeFailed() << std::endl
        << "          Sign Failed:   " << statistics.getSignFailed() << std::endl;
    std::cout << str.str();
}

static void printRxStatistics(const SplitsStatistics::RxStatistics& statistics) {
    std::stringstream str;
    str << "Service IDs:             ";
    for (auto sid : statistics.getPsids()) {
        str << sid << " ";
    }
    str << std::endl;
    str << "Number of Rx Total:      " << statistics.getRxNumber() << std::endl
        << "          Failed:        " << statistics.getRxFailed() << std::endl
        << "          Decode Failed: " << statistics.getDecodeFailed() << std::endl
        << "          Verif Failed:  " << statistics.getVerifFailed() << std::endl;
    std::cout << str.str();
}

static void printMsgStatistics(const SplitsStatistics::MessageStatistics& statistics) {
    std::stringstream str;
    str << "Number of Tx Total:              " << statistics.getTxNumber() << std::endl
        << "          Rx Signed:             " << statistics.getRxSignedCount() << std::endl
        << "          Rx Unsigned:           " << statistics.getRxUnsignedCount() << std::endl
        << "          Decode Failed:         " << statistics.getDecodeFailed() << std::endl
        << "          Verification Failed:   " << statistics.getVerifFailed() << std::endl
        << "          Misbehavior:           " << statistics.getMisbehaviorsDetected() << std::endl
        << "          RelevanceCheck Failed: " << statistics.getRelevanceCheckFailed() << std::endl
        << "          ConsistencyCheck Filed:" << statistics.getConsistCheckFailed() << std::endl;
    std::cout << str.str();
}

static void printfSecurityStatistics(const SplitsStatistics::SecurityStatistics& statistics) {
    std::stringstream str;
    SplitsStatistics::HvBSMSpecific value = statistics.getBsm();
    str << "Time passed since last Pseudonym Certificate Change: "
        << value.getPseudonymCertChangePassed() << std::endl
        << "Distance since last Pseudonym Certificate Change: "
        << value.getPseudonymCertChangeDistance() << std::endl
        << "Temporary ID: " << value.getTempID() << std::endl;
    std::cout << str.str();
}

static void printfCongestionControlStatistics(
    const SplitsStatistics::CongestionControlStatistics& statistics) {
    std::stringstream str;
    str << "Num of RVs: " << statistics.getNumRVsInRange() << std::endl
        << "RV density: " << statistics.getVehicleDensity() << std::endl
        << "CQI " << statistics.getCQI() << std::endl
        << "Max ITT Interval: " << statistics.getTxMaxITTInterval() << std::endl;
    std::cout << str.str();
}

int main(int argc, char *argv[]) {
    std::bitset<static_cast<unsigned>(StatisticsType::NUM_OF_TYPE)> type;
    std::vector<SplitsStatistics::PSID> serviceIds;
    std::vector<SplitsStatistics::MessageID> messageIDs;
    std::vector<unsigned short> ports;
    bool total = true;
    bool sec = false;
    bool congestion = false;
    unsigned interval = 0; //second

    int opt = 0;
    while ((opt = getopt(argc, argv, "ci:k:m:p:qrst")) != -1) {
        switch (opt) {
        case 'c':
            congestion = true;
            break;
        case 'k':
            interval = 10;
            if (optarg) {
                interval = strtoul(optarg, nullptr, 10);
            }
            break;
        case 'r':
            type.set(static_cast<size_t>(StatisticsType::RX));
            break;
        case 's':
            sec = true;
            break;
        case 't':
            type.set(static_cast<size_t>(StatisticsType::TX));
            break;
        case 'p': {
            // parse the port numbers
            std::stringstream ss(optarg);
            unsigned short port;
            while (ss >> port) {
                ports.push_back(port);
            }
            if (ports.size() <= 0) {
                LOGE("Please provide valid ports");
                goto helpInfo;
            }
            total = false;
            break;
        }
        case 'i': {
            // parse the service numbers
            std::stringstream ss(optarg);
            SplitsStatistics::PSID sid;
            while (ss >> sid) {
                serviceIds.push_back(sid);
            }
            if (serviceIds.size() <= 0) {
                std::cout << "Please provide valid service IDs" << std::endl;
                goto helpInfo;
            }
            total = false;
            break;
        }
        case 'm': {
            // parse the message type
            std::stringstream ss(optarg);
            SplitsStatistics::MessageID message_id;
            while (ss >> message_id) {
                messageIDs.push_back(message_id);
            }
            if (messageIDs.size() <= 0) {
                std::cout << "Please provide valid message IDs" << std::endl;
                goto helpInfo;
            }
            total = false;
            break;
        }
        default: /* '?' */
helpInfo:
            std::cerr << "Usage: splits_statistics [-c] [-s] [-t] [-r] [-p <ports>] [-i <service IDs>] [-m]"
                      << std::endl;
            return -1;
        }
    }
    
    CommonAPI::Runtime::setProperty("LogContext", "SplitsService");
    CommonAPI::Runtime::setProperty("LogApplication", "SplitsService");
    CommonAPI::Runtime::setProperty("LibraryBase", "SplitsService");

    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();
    std::string domain = "local";
    std::string instance = "commonapi.SPLITS.SplitsStatistics";
 
    auto statisticsProxy =
        runtime->buildProxyWithDefaultAttributeExtension<
        SplitsStatisticsProxy,
        CommonAPI::Extensions::AttributeCacheExtension>(domain, instance, "Client-SPLITS");
    while (!statisticsProxy->isAvailable()) {
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }

    CommonAPI::CallStatus internalCallStatus;
    CommonAPI::CallInfo info;

    SplitsStatistics::SPLITSStatus status;

    // get uptime
    statisticsProxy->getSPLITSStatus(internalCallStatus, status, &info);

    std::cout << "OS UP time:     " << status.getOSuptime();
    std::cout << " (" << status.getCalendarOSUptime() << ")" << std::endl;
    std::cout << "SPLITS UP time: " << status.getUptime();
    std::cout << " (" << status.getCalendarUptime() << ")" << std::endl;

    // register incident events
    statisticsProxy->getIncidentAttribute().getChangedEvent().subscribe(
        [&](const ::v1::commonapi::SPLITS::SplitsStatistics::AbnormalIncident& val) {
            if (0 == val.getTimestamp()) {
                // invalid
                return;
            }
            printf("Received abnormal event at time = %lu: err: %s.%s\n",
                   val.getTimestamp(), val.getErr().toString(), val.getDetail().c_str());
        });

    std::function<void()> f_txStatistics =
        [&statisticsProxy]() {
            CommonAPI::CallStatus internalCallStatus;
            CommonAPI::CallInfo info;
            SplitsStatistics::AllTxStatistics statistics;
            statisticsProxy->getTxStatistics(internalCallStatus, statistics, &info);

            std::cout << "----------TX Statistics----------" << std::endl;
            for (auto t : statistics) {
                std::cout << "PSID: " <<  std::get<0>(t) << std::endl;
                printTxStatistics(std::get<1>(t));
                std::cout << std::endl;
            }
        };
    std::function<void(SplitsStatistics::PSID service_id)> f_txSidStatistics =
        [&statisticsProxy](SplitsStatistics::PSID service_id) {
            CommonAPI::CallStatus internalCallStatus;
            CommonAPI::CallInfo info;
            SplitsStatistics::TxStatistics statistics;
            statisticsProxy->getTxStatistics(service_id, internalCallStatus, statistics, &info);
            std::cout << "PSID: " <<  service_id << std::endl;
            printTxStatistics(statistics);
        };

    std::function<void()> f_rxStatistics =
        [&statisticsProxy]() {
            CommonAPI::CallStatus internalCallStatus;
            CommonAPI::CallInfo info;
            SplitsStatistics::AllRxStatistics statistics;
            statisticsProxy->getRxStatistics(internalCallStatus, statistics, &info);

            std::cout << "----------RX Statistics----------" << std::endl;
            for (auto t : statistics) {
                std::cout << "Port: " <<  std::get<0>(t) << std::endl;
                printRxStatistics(std::get<1>(t));
                std::cout << std::endl;
            }
        };
    std::function<void()> f_msgStatistics =
        [&statisticsProxy]() {
            CommonAPI::CallStatus internalCallStatus;
            CommonAPI::CallInfo info;
            SplitsStatistics::AllMessageStatistics statistics;
            statisticsProxy->getMsgStatistics(internalCallStatus, statistics, &info);

            std::cout << "----------Message Statistics----------" << std::endl;
            for (auto t : statistics) {
                std::cout << "Message ID: " <<  std::get<0>(t) << std::endl;
                printMsgStatistics(std::get<1>(t));
                std::cout << std::endl;
            }
        };

    std::function<void(SplitsStatistics::MessageID)> f_msgIdStatistics =
        [&statisticsProxy](SplitsStatistics::MessageID msg_id) {
            CommonAPI::CallStatus internalCallStatus;
            CommonAPI::CallInfo info;
            SplitsStatistics::MessageStatistics statistics;
            statisticsProxy->getMsgStatistics(msg_id, internalCallStatus, statistics, &info);
            std::cout << "Message ID: " <<  msg_id << std::endl;
            printMsgStatistics(statistics);
            std::cout << std::endl;
        };

    std::function<void()> f_securityStatistics =
        [&statisticsProxy]() {
            CommonAPI::CallStatus internalCallStatus;
            CommonAPI::CallInfo info;
            SplitsStatistics::SecurityStatistics statistics;
            statisticsProxy->getSecurityReport(internalCallStatus, statistics, &info);
            std::cout << "----------Security Statistics----------" << std::endl;

            printfSecurityStatistics(statistics);
            std::cout << std::endl;
        };

    std::function<void()> f_congestionControlStatistics =
        [&statisticsProxy]() {
            CommonAPI::CallStatus internalCallStatus;
            CommonAPI::CallInfo info;
            SplitsStatistics::CongestionControlStatistics statistics;
            statisticsProxy->getCongestionControlStatistics(internalCallStatus, statistics, &info);
            std::cout << "----------Congestion Control Statistics----------" << std::endl;

            printfCongestionControlStatistics(statistics);
            std::cout << std::endl;
        };

    do {
        if (congestion) {
            f_congestionControlStatistics();
        }

        if (sec) {
            f_securityStatistics();
        }

        if (total) {
            f_txStatistics();
            f_rxStatistics();
            f_msgStatistics();
        } else {
            for (auto sid : serviceIds) {
                f_txSidStatistics(sid);
            }

            for (auto msgid : messageIDs) {
                f_msgIdStatistics(msgid);
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(interval));
    } while(interval);

    return 0;
}
