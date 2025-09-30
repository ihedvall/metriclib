/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/


#include <format>
#include <chrono>
#include <sstream>
#include <string>

#include "metrichelper.h"

using namespace std::chrono;

namespace metric {
std::string FloatToString(float value) {
  return std::format("{}", value);
}

std::string DoubleToString(double value) {
  return std::format("{}", value);
}

uint64_t NowNs() {
  const sys_time<nanoseconds> now = system_clock::now();
  return now.time_since_epoch().count();
}

uint64_t NowMs() {
  const uint64_t now =NowNs();
  return now / 1'000'000;
}

std::string NanoSecToIso8601(uint64_t ns1970) {
  std::string iso;
  try {
    const auto temp = nanoseconds(ns1970);
    iso = std::format("{:%FT%TZ}",
      sys_time<nanoseconds>{temp});
  } catch (const std::exception &) {

  }
  return iso;
}

uint64_t Iso8601ToNanoSec(const std::string iso_time) {
  std::istringstream iso(iso_time);
  sys_time<nanoseconds> conv_time;
  iso >> parse("%FT%TZ", conv_time);
  return conv_time.time_since_epoch().count();
}

}  // namespace metric