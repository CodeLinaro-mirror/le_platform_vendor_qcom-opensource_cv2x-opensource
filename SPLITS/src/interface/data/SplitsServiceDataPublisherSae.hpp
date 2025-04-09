/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef __SPLITS_SERVICE_DATAPUBLISHER_SAE_HPP__
#define __SPLITS_SERVICE_DATAPUBLISHER_SAE_HPP__

#include "SplitsServiceDataPublisher.hpp"

namespace SPLITS {
namespace Interface {

class SplitsServiceDataPublisherSae : public SplitsServiceDataPublisher {
public:
    SplitsServiceDataPublisherSae();

    /**
     * publish a message via fast-dds interface.
     *
     * @param[in] msg message to be published.
     * @return none.
     */
    void publish(std::shared_ptr<v2xMsg> msg) override;
};

} //namespace Interface
} //namespace SPLITS

#endif //__SPLITS_SERVICE_DATAPUBLISHER_SAE_HPP__
