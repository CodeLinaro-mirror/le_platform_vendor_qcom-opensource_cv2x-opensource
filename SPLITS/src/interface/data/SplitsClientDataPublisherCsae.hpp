/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/**
 * @file SplitsClientDataPublisherCsae
 *
 * @description SPLITS dds data publisher for CSAE, instantiated from client(EAP) side. This class
 *  is derrived from class SplitsClientDataPublisher, it overrides the init method to intialize
 *  the message pool for CSAE.
 */
#ifndef __SPLITS_CLIENT_DATAPUBLISHER_CSAE_HPP__
#define __SPLITS_CLIENT_DATAPUBLISHER_CSAE_HPP__

#include "SplitsClientDataPublisher.hpp"

namespace SPLITS {
namespace Interface {

//default queue size
#define MSG_POOL_SIZE   16

class SplitsClientDataPublisherCsae : public SplitsClientDataPublisher {
public:
    bool init(std::string topicPrefix) override;
    void publish(void) override;
};

} //namespace Interface
} //namespace SPLITS

#endif //__SPLITS_CLIENT_DATAPUBLISHER_CSAE_HPP__
