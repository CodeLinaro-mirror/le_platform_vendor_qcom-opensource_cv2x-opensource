/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file SplitsCoreFactory.hpp
 *
 * @brief SplitsCoreFactory is the factory that create the core instance.
 */
#include "SplitsCore.hpp"

#ifndef _SPLITSCORE_FACTORY_HPP_
#define _SPLITSCORE_FACTORY_HPP_

namespace SPLITS {
namespace Core {

class SplitsCoreFactory {

public:
    /**
     * Get the SplitsCoreFactory singleton
     *
     * @returns reference to the SplitsCoreFactory singleton
     */
    static SplitsCoreFactory & getInstance();

    /**
     * Get SplitsCore instance
     *
     * @returns shared poitner to SplitsCore instance on success or nullptr otherwise.
     */
    virtual std::shared_ptr<SplitsCore> getCore();

//protected:
    SplitsCoreFactory();
    virtual ~SplitsCoreFactory();
};

}
}
#endif
