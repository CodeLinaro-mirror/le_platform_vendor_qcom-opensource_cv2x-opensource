/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file Logger.hpp
 *
 * @brief Logger class provide logging APIs
 */
#ifndef SPLITS_LOGGER_HPP_
#define SPLITS_LOGGER_HPP_

#include <sstream>

#ifdef TELSDK_LOG
#include <telux/common/Log.hpp>
#endif

#ifdef COMMONAPI_LOG
#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#define COMMONAPI_INTERNAL_COMPILATION
#endif
#include <CommonAPI/Logger.hpp>
#endif

#ifdef TELSDK_LOG
#define SPLITS_ERR(args...)  LOG(telux::common::LogLevel::LEVEL_ERROR, args)
#define SPLITS_DBG(args...)  LOG(telux::common::LogLevel::LEVEL_DEBUG, args)
#define SPLITS_INF(args...)  LOG(telux::common::LogLevel::LEVEL_INFO, args)
#define SPLITS_WARN(args...)  LOG(telux::common::LogLevel::LEVEL_WARNING, args)
#elif defined(COMMONAPI_LOG)
#define LINE_NO_STR(x) #x
#define LINE_NO(x) LINE_NO_STR(x)
#define SPLITS_ERR(args...)  COMMONAPI_ERROR(__FILE__, LINE_NO(__LINE__), args)
#define SPLITS_DBG(args...)  COMMONAPI_DEBUG(__FILE__, LINE_NO(__LINE__), args)
#define SPLITS_INF(args...)  COMMONAPI_INFO(__FILE__, LINE_NO(__LINE__), args)
#define SPLITS_WARN(args...)  COMMONAPI_WARNING(__FILE__, LINE_NO(__LINE__), args)
#else
// 1. strip all logs for release build
// 2. If prefer other logging ways, try to wrap it in SPLITS::Core::Log
#define SPLITS_ERR(args...)  // SPLITS::Core::Log(0, args)
#define SPLITS_DBG(args...)  // SPLITS::Core::Log(0, args)
#define SPLITS_INF(args...)  // SPLITS::Core::Log(0, args)
#define SPLITS_WARN(args...) // SPLITS::Core::Log(0, args)
#endif

#define LOGE(args...)        SPLITS_ERR(args)
#define LOGD(args...)        SPLITS_DBG(args)
#define LOGI(args...)        SPLITS_INF(args)
#define LOGW(args...)        SPLITS_WARN(args)

namespace SPLITS {
namespace Core {

template <typename T>
void appendLog(std::ostream& o, T t) {
    o << t << std::endl;
}

// recursive variadic function
template<typename T, typename... Args>
void appendLog(std::ostream& o, T t, Args... args) {
    appendLog(o, t);
    appendLog(o, args...);
}

template<typename... Args>
void Log(int loglevel, Args... args) {
    std::ostringstream oss;
    appendLog(oss, args...);
    // Other logging API,
    //  printf("%s\n", oss.str().c_str());
}

}
}

#endif  //SPLITS_LOGGER_HPP_
