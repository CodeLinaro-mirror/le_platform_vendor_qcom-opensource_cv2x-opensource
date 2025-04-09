/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <sys/types.h>
#include <stdbool.h>
#include <grp.h>
#include <unistd.h>
#include <iostream>
#include "SplitsCoreFactory.hpp"
#include "Logger.hpp"

gid_t getGidByName(const char* names) {
    struct group* tempGrp;
    gid_t tmpGid = {};
    if ((tempGrp = getgrnam(names)) != NULL) {
        tmpGid = tempGrp->gr_gid;
    }
    return tmpGid;
}

int setSupplementaryGroups(const char** grps, int size) {
    gid_t groupId[size];
    for (int i = 0; i < size; i++) {
        groupId[i] = getGidByName(*(grps+i));
    }
    int numGroups = getgroups(0, NULL);
    if (-1 == numGroups) {
        LOGE("getgroups failed");
        return -1;
    }
    gid_t gid[numGroups+size];
    if (-1 == getgroups(numGroups, gid)) {
        LOGE("getgroups failed");
        return -1;
    }
    for (int i = 0; i < size; i++) {
        gid[numGroups + i] = groupId[i];
    }
    if (-1 == setgroups(numGroups+size, gid)) {
        LOGE("setgroups failed");
        return -1;
    }
    return 0;
}

int main(int argc, char **argv) {

    LOGD("Splits Service Starts");

    int c;
    const char* groups[] = {"system", "diag", "radio", "locclient", "mvm",
                            "dlt", "spi", "gpio", "logd"};
    std::string rxpduFileName;
    std::string txpduFileName;
    std::string cfgFileName;
    std::string csvFileDir;
    if (setSupplementaryGroups(&groups[0], 1)) {
        return -1;
    }
    bool csae = false;
    while ((c = getopt(argc, argv, "t:p:f:F:c")) != -1) {
        switch(c) {
        case 'c':
            csae = true;
            LOGI("CCSA V2X stack");
            break;
        case 'p':
             if (optarg) {
             rxpduFileName = optarg;
             } else {
                 LOGE("Must followed by a pdu file name if '-p' supplied");
                 return -1;
             }
             break;
        case 't':
             if (optarg) {
                 txpduFileName = optarg;
             } else {
                 LOGE("Must followed by a pdu file name if '-t' supplied");
                 return -1;
             }
             break;
        case 'f':
             if (optarg) {
                 cfgFileName = optarg;
             } else {
                 LOGE("Must followed by a config file name if '-f' supplied");
                 return -1;
             }
             break;
        case 'F':
            if (optarg) {
                csvFileDir = optarg;
            } else {
                LOGE("Must followed by a directory if '-F' supplied");
                return -1;
            }
            break;
        default:
            LOGE("Usage:");
            LOGE("splits_service [-f cfgFileName]");
            LOGE("               [-p rxpduFileName]");
            LOGE("               [-t txpduFileName]");
            LOGE("               [-F csvFileDir]");
            LOGE("               [-c for CCSA]");
            break;
         }
     }
    auto coreFactory = SPLITS::Core::SplitsCoreFactory::getInstance();
    auto core = coreFactory.getCore(csae);
    if (nullptr == core) {
        LOGE("failed to get SPLITS core");
        return -1;
    }
    if (core->init(cfgFileName, rxpduFileName, txpduFileName, csvFileDir) < 0) {
        LOGE("failed to init core");
        return -1;
    }

    core->run();
    return 0;
}
