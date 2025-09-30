/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
 */

#include <chrono>
#include <iostream>
#include "metric/metriclisten.h"

using namespace std::chrono;

namespace metric {

bool MetricListen::IsActive() const {
  return (bool) listen_function_;
}

void MetricListen::OnAddMessage(uint64_t nano_sec_1970, std::string text) {
  if (listen_function_) {
    if (nano_sec_1970 == 0) {
      // Convert to now
      const sys_time utc = system_clock::now();
      const nanoseconds dur = utc.time_since_epoch();
      nano_sec_1970 = static_cast<uint64_t>(dur.count());
    }
    listen_function_(pre_text_,nano_sec_1970, std::move(text));
  }
}

void MetricListen::LogToConsole(const std::string& pre_text,
  uint64_t ns1970,const std::string& text) {
  try {
    const uint64_t ms1970 = ns1970 / 1'000'000;
    duration<uint64_t, std::milli> dur(ms1970);
    sys_time<milliseconds> utc(dur);
    if (const auto* time_zone = current_zone(); time_zone != nullptr) {
      const auto local = time_zone->to_local(utc);
      std::cout << pre_text << "> " << std::format("{:%F %T} ", local);
    }
    std::cout << text << std::endl;
  } catch (const std::exception&) {

  }
}

}  // namespace metric