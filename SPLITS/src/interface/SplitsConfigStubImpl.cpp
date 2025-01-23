/*
 *Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "SplitsCoreImpl.hpp"
#include "SplitsConfigStubImpl.hpp"

SplitsConfigStubImpl::SplitsConfigStubImpl(std::shared_ptr<SPLITS::Core::SplitsCoreImpl> core) {
    splitsCore_ = core;
}
void SplitsConfigStubImpl::readCoreConfig(const std::shared_ptr<CommonAPI::ClientId> _client,
            readCoreConfigReply_t _reply) {
    if (splitsCore_ != nullptr) {
        _reply(splitsCore_->getConfig());
    } else {
    }
}

void SplitsConfigStubImpl::writeCoreConfig(
    const std::shared_ptr<CommonAPI::ClientId> _client,
    SplitsConfig::CoreConfig _coreConfig, writeCoreConfigReply_t _reply) {
    if (splitsCore_ != nullptr) {
        splitsCore_->setConfig(_coreConfig);
        _reply();
    } else {
    }
}

