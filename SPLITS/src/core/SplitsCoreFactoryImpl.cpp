/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>
#include "SplitsCoreFactory.hpp"
#include "SplitsCoreImpl.hpp"

namespace SPLITS {
namespace Core {

SplitsCoreFactory::SplitsCoreFactory() {
    std::cout << "CoreFactory ctor" << std::endl;
}
SplitsCoreFactory::~SplitsCoreFactory() {
    std::cout << "CoreFactory dtor" << std::endl;
}

SplitsCoreFactory & SplitsCoreFactory::getInstance() {
    static SplitsCoreFactory instance;
    return instance;
}

std::shared_ptr<SplitsCore> SplitsCoreFactory::getCore(void) {
    return std::make_shared<SPLITS::Core::SplitsCoreImpl>();
}
}
}
