/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>
#include "SplitsCodecFactory.hpp"
#include "SplitsCodecImpl.hpp"

namespace SPLITS {
namespace Codec {

    SplitsCodecFactory::SplitsCodecFactory() {
    std::cout << "CodecFactory ctor" << std::endl;
}
SplitsCodecFactory::~SplitsCodecFactory() {
    std::cout << "CodecFactory dtor" << std::endl;
}

SplitsCodecFactory & SplitsCodecFactory::getInstance() {
    static SplitsCodecFactory instance;
    return instance;
}

std::shared_ptr<SplitsCodec> SplitsCodecFactory::getCodec(StackType st) {
    return std::make_shared<SPLITS::Codec::SplitsCodecImpl>(st);
}
}
}
