/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <sys/types.h>
#include <grp.h>
#include <unistd.h>
#include <iostream>
#include "SplitsCoreFactory.hpp"

gid_t getGidByName(char* names) {
    struct group* tempGrp;
    gid_t tmpGid;
    if((tempGrp = getgrnam(names)) != NULL) {
        tmpGid = tempGrp->gr_gid;
    }
    return tmpGid;
}


int setSupplementaryGroups(char** grps, int size) {
    gid_t groupId[size];
    for (int i = 0; i < size; i++) {
        groupId[i] = getGidByName(*(grps+i));
    }
    int numGroups = getgroups(0, NULL);
    if (-1 == numGroups) {
        std::cout << "getgroups failed" << std::endl;
        return -1;
    }
    gid_t gid[numGroups+size];
    if (-1 == getgroups(numGroups, gid)) {
        std::cout << "getgroups failed" << std::endl;
        return -1;
    }
    for (int i = 0; i < size; i++) {
        gid[numGroups + i] = groupId[i];
    }
    if (-1 == setgroups(numGroups+size, gid)) {
        std::cout << "setgroups failed" << std::endl;
        return -1;
    }
    return 0;
}

int main(int argc, char **argv) {
    int c;
    char* groups[] = {"system", "diag", "radio", "locclient", "mvm",
                                    "dlt", "spi", "gpio", "logd"};
    std::string rxpduFileName;
    std::string txpduFileName;
    std::string cfgFileName;
    if (setSupplementaryGroups(&groups[0], 1)) {
        return -1;
    }
    while ((c = getopt(argc, argv, "t:p:f:")) != -1) {
        switch(c) {
        case 'p':
             if (optarg) {
             rxpduFileName = optarg;
             } else {
                 std::cout << "Must followed by a pdu file name if '-p' supplied" << std::endl;
                 return -1;
             }
             break;
        case 't':
             if (optarg) {
                 txpduFileName = optarg;
             } else {
                 std::cout << "Must followed by a pdu file name if '-t' supplied" << std::endl;
                 return -1;
             }
        case 'f':
             if (optarg) {
                 cfgFileName = optarg;
             } else {
                 std::cout << "Must followed by a config file name if '-f' supplied" << std::endl;
                 return -1;
             }

        default:
            std::cout << "Usage: splits_service [-f cfgFileName] [-p rxpduFileName | -t txpduFileName ]" << std::endl;
            break;

         }
     }
    auto coreFactory = SPLITS::Core::SplitsCoreFactory::getInstance();
    auto core = coreFactory.getCore();
    if (nullptr == core) {
        std::cout << "failed to get SPLITS core" << std::endl;
        return -1;
    }
    if (core->init(cfgFileName, rxpduFileName, txpduFileName) < 0) {
        std::cout << "failed to init core" << std::endl;
        return -1;
    }

    core->run();
    return 0;
}
