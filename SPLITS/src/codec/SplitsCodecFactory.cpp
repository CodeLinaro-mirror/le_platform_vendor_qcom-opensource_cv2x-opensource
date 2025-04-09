/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>
#include "SplitsCodecFactory.hpp"
#include "SplitsCodecImplSae.hpp"
#include "SplitsCodecImplCsae.hpp"

namespace SPLITS {
namespace Codec {

SplitsCodecFactory::SplitsCodecFactory() {
}

SplitsCodecFactory::~SplitsCodecFactory() {
}

SplitsCodecFactory & SplitsCodecFactory::getInstance() {
    static SplitsCodecFactory instance;
    return instance;
}

std::shared_ptr<SplitsCodec> SplitsCodecFactory::getCodec(StackType st) {
    if (StackType::STACK_TYPE_SAE == st) {
        return std::make_shared<SPLITS::Codec::SplitsCodecImplSae>();
    } else if (StackType::STACK_TYPE_CSAE == st) {
        return std::make_shared<SPLITS::Codec::SplitsCodecImplCsae>();
    } else {
        return nullptr;
    }
}
}
}
