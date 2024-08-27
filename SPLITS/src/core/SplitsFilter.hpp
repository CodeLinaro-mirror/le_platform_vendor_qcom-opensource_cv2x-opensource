/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <memory>
#include <atomic>
#include <telux/common/CommonDefines.hpp>

using telux::common::Status;
using telux::loc::ILocationInfoEx;

namespace SPLITS {
namespace Core {

class SplitsCore;

class SplitsFilter :
    public std::enable_shared_from_this<SplitsFilter> {
public:
    std::shared_ptr<SplitsFilter> getptr() {
        return shared_from_this();
    }
    ~SplitsFilter() {}

    double bsmCompute2dDistance(double hvLat, double hvLon, double rvLat, double rvLon);
private:
    long double deg2rad(double deg);
    double rad2deg(long double rad);
};

} //namespace Core
} //namespace SPLITS