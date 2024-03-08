/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CAPICV2XFACTORYIMPL_HPP
#define CAPICV2XFACTORYIMPL_HPP
#include <telux/cv2x/Cv2xFactory.hpp>

namespace telux {
namespace cv2x {

class CapiCv2xFactory : public Cv2xFactory {
public:
    static Cv2xFactory & getInstance();
    static CapiCv2xFactory& getCapiCv2xFactory();

    std::shared_ptr<ICv2xRadioManager> getCv2xRadioManager(
        telux::common::InitResponseCb cb = nullptr);

private:
    CapiCv2xFactory();
    ~CapiCv2xFactory(){};

    std::mutex mutex_;
    void onGetCv2xRadioManagerResponse(telux::common::ServiceStatus status);
    std::weak_ptr<ICv2xRadioManager> radioManager_;
    std::vector<telux::common::InitResponseCb> cv2xManagerInitCallbacks_;
    telux::common::ServiceStatus cv2xManagerInitStatus_;
};

} //namespace cv2x
} //namespace telux
#endif //CAPICV2XFACTORYIMPL_HPP
