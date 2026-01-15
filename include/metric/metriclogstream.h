/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

/** \file metriclogstream.h
* \brief The metric log stream file is intended to isolate the logging so
* the library can be built without dependency of the util and boost libraries.
* The applications in the library do however include the above libraries.
*/
#pragma once

#include <cstdint>
#include <string>
#include <sstream>
#include <source_location>
#include <functional>

namespace metric {

enum class MetricLogSeverity : uint8_t {
  kTrace = 0,  ///< Trace or listen message
  kDebug,      ///< Debug message
  kInfo,       ///< Informational message
  kNotice,     ///< Notice message. Notify the user.
  kWarning,    ///< Warning message
  kError,      ///< Error message
  kCritical,   ///< Critical message (device error)
  kAlert,      ///< Alert or alarm message
  kEmergency   ///< Fatal error message
};

#define METRIC_TRACE() MetricLogStream( std::source_location::current(), \
              MetricLogSeverity::kTrace)  ///< Trace log message
#define METRIC_DEBUG() MetricLogStream( std::source_location::current(), \
              MetricLogSeverity::kDebug)  ///< Debug log message
#define METRIC_INFO() MetricLogStream( std::source_location::current(), \
              MetricLogSeverity::kInfo)  ///< Info log message
#define METRIC_NOTICE() MetricLogStream( std::source_location::current(), \
              MetricLogSeverity::kNotice)  ///< Notice log message
#define METRIC_WARNING() MetricLogStream( std::source_location::current(), \
              MetricLogSeverity::kWarning)  ///< Warning log message
#define METRIC_ERROR() MetricLogStream( std::source_location::current(), \
              MetricLogSeverity::kError)  ///< Error log message
#define METRIC_CRITICAL() MetricLogStream( std::source_location::current(), \
              MetricLogSeverity::kCritical)  ///< Critical log message
#define METRIC_ALERT() MetricLogStream( std::source_location::current(), \
              MetricLogSeverity::kAlert)  ///< Alert log message
#define METRIC_EMERGENCY() MetricLogStream( std::source_location::current(), \
              MetricLogSeverity::kEmergency)  ///< Emergency log message

/** \brief MDF log function definition. */
using MetricLogFunction = std::function<void(
    std::source_location location,
    MetricLogSeverity severity,
    const std::string& text)>;

/** \brief MDF log stream interface.
*
*
*/
class MetricLogStream : public std::ostringstream {
public:
 MetricLogStream(std::source_location location,
                 MetricLogSeverity severity);  ///< Constructor
 ~MetricLogStream() override;                  ///< Destructor

 MetricLogStream() = delete;
 MetricLogStream(const MetricLogStream&) = delete;
 MetricLogStream(MetricLogStream&&) = delete;
 MetricLogStream& operator=(const MetricLogStream&) = delete;
 MetricLogStream& operator=(MetricLogStream&&) = delete;

 /** \brief Sets a log function. */
 static void SetLogFunction(const MetricLogFunction& func);
 static void ResetLogFunction();
 static void LogToConsole( std::source_location location,
                           MetricLogSeverity severity,
                           const std::string& text);


 static void ShowLocation(bool show_location);
 [[nodiscard]] static bool ShowLocation();

 [[nodiscard]] static size_t ErrorCount();
 static void ResetErrorCount();


protected:
 std::source_location location_;     ///< File and function location.
 MetricLogSeverity severity_;  ///< Log level of the stream

 /** \brief Defines the logging function. */
 virtual void LogString(std::source_location location,
                        MetricLogSeverity severity,
                        const std::string& text);
};

}  // namespace metric

