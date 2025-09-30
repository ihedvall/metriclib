/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

#include <iostream>
#include <chrono>
#include <format>
#include <string_view>
#include <array>
#include <filesystem>
#include <atomic>

#include "metric/metriclogstream.h"

using namespace std::chrono;
using namespace std::filesystem;
namespace {
const std::array<std::string_view, 9> kSeverityList = {"Trace", "Debug",
                                                       "Info", "Notice",
                                                       "Warning", "Error",
                                                       "Critical","Alert",
                                                       "Emergency" };
metric::MetricLogFunction kLogFunction;
std::atomic<bool> kShowLocation = true;
std::atomic<size_t> kErrorCount = 0;

}  // end namespace

namespace metric {

MetricLogStream::MetricLogStream(std::source_location location,
                                 MetricLogSeverity severity)
   : location_(location),
      severity_(severity) {
}

MetricLogStream::~MetricLogStream() {
 MetricLogStream::LogString(location_, severity_, str());
}

void MetricLogStream::LogString(std::source_location location,
                                MetricLogSeverity severity,
                                const std::string& text) {
 if (severity >= MetricLogSeverity::kError ) {
   ++kErrorCount;
 }
 if (kLogFunction) {
   kLogFunction(location, severity, text);
 }
}

void MetricLogStream::SetLogFunction(const MetricLogFunction &func) {
 kLogFunction = func;
}

void MetricLogStream::ResetLogFunction() {
  kLogFunction = nullptr;
}
void MetricLogStream::LogToConsole(std::source_location location,
                                   MetricLogSeverity severity,
                                   const std::string& text) {
   try {

    const sys_time utc = system_clock::now();
    if (const auto* time_zone = current_zone(); time_zone != nullptr) {
      const auto local = time_zone->to_local(utc);
      const auto dur = local.time_since_epoch();
      const auto milli_sec = duration_cast<milliseconds>(dur);
      std::cout << std::format("{0:%F %H:%M}:{1:%S} ", local, milli_sec);
    }

    if (const auto index = static_cast<size_t>(severity);
        index < kSeverityList.size()) {
      std::cout << "[" << kSeverityList[index] << "] ";
    }

    std::cout << text << " ";
    if (ShowLocation()) {
      path filename = location.file_name();
      std::cout << "(" << filename.stem().string() << ":"
                << location.function_name() << " C:" << location.column()
                << " L:" << location.line() << ")";
    }
    std::cout << std::endl;
  } catch (const std::exception&) {

  }
}

void MetricLogStream::ShowLocation(bool show_location) {
  kShowLocation = show_location;
}

bool MetricLogStream::ShowLocation() { return kShowLocation; }
size_t MetricLogStream::ErrorCount() { return kErrorCount; }
void MetricLogStream::ResetErrorCount() {kErrorCount = 0;}

}  // namespace mdf