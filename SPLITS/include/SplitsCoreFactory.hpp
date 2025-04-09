/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file SplitsCoreFactory.hpp
 *
 * @brief SplitsCoreFactory is the factory that create the core instance.
 */
#include "SplitsCore.hpp"
#include <stdbool.h>

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
    virtual std::shared_ptr<SplitsCore> getCore(bool csae = false);

//protected:
    SplitsCoreFactory();
    virtual ~SplitsCoreFactory();
};

}
}
#endif
