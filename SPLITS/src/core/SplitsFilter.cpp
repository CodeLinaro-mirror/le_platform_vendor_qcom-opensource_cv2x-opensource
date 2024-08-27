/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <cmath>
#include "BsmMsgGenerator.hpp"
#include "SplitsCodec.hpp"
#include "v2xMsgSae.hpp"
#include "SplitsFilter.hpp"

using J2735::idl::BasicSafetyMessage;

namespace SPLITS {

namespace Core {

double SplitsFilter::bsmCompute2dDistance(double hvLat, double hvLon,
    double rvLat, double rvLon){

    double rLat1 = deg2rad(hvLat); // deg2rad(lat1);
    double rLat2 = deg2rad(rvLat); // deg2rad(lat2);
    double rLon1 =  deg2rad(hvLon); // deg2rad(lon1);
    double rLon2 = deg2rad(rvLon); // deg2rad(lon2);
    double alpha = 6378137; // mean radius of earth in meters
    double f = 0.003353; // earth's flattening
    double f1 = pow((f * (2.0 - f)), (0.5));
    double f1_sqd = pow(f1, 2.0);
    double numer = (alpha * (1.0-f1_sqd));
    double denom = pow(1.0 - (f1_sqd * pow(sin(rvLat),2)), 1.5);
    double meridRad = (numer / denom);
    double distance = meridRad *
        ( acos(sin(rLat1) * sin(rLat2) + cos(rLat1) * cos(rLat2) * cos(rLon1 - rLon2)) ); // cos-1
    return distance;
}
long double SplitsFilter::deg2rad(double deg){
    return deg * (M_PI / 180.0);
}

 double SplitsFilter::rad2deg(long double rad){
    return tan (rad * M_PI / 180.0);
}

} //namespace Core
} //namespace SPLITS