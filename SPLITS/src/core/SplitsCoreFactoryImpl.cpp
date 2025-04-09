/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>
#include "SplitsCoreFactory.hpp"
#include "SplitsCoreImpl.hpp"

namespace SPLITS {
namespace Core {

SplitsCoreFactory::SplitsCoreFactory() {
}
SplitsCoreFactory::~SplitsCoreFactory() {
}

SplitsCoreFactory & SplitsCoreFactory::getInstance() {
    static SplitsCoreFactory instance;
    return instance;
}

std::shared_ptr<SplitsCore> SplitsCoreFactory::getCore(bool csae) {
    StackType st = (csae) ? (StackType::STACK_TYPE_CSAE) : (StackType::STACK_TYPE_SAE);
    return std::make_shared<SPLITS::Core::SplitsCoreImpl>(st);
}
}
}
