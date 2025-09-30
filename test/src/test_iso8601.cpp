/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/
#include <chrono>
#include <format>
#include <string>
#include <ctime>
#include <cstdlib>

#include <gtest/gtest.h>

using namespace std::chrono;

TEST(Iso8601, NanoSec1970) {
  const time_t ref_time = time(nullptr);
  const sys_time<nanoseconds> now = system_clock::now();
  const uint64_t ns1970 = now.time_since_epoch().count();
  const uint64_t sec1970 = ns1970 / 1'000'000'000;

  int64_t diff = static_cast<int64_t>(sec1970);
  diff -= static_cast<int64_t>(ref_time);

  EXPECT_LT(std::abs(diff), 2);
}

TEST(Iso8601, ns1970ToString) {
  const sys_time<nanoseconds> now = system_clock::now();
  const uint64_t ns1970 = now.time_since_epoch().count();

  const auto temp = nanoseconds(ns1970);
  const std::string iso8601 = std::format("{:%FT%TZ}",
    sys_time<nanoseconds>{temp});
  std::cout << iso8601 << std::endl;
  EXPECT_TRUE(iso8601.find( ".") != std::string::npos);
}

TEST(Iso8601, ns1970FromString) {
  const sys_time<nanoseconds> now = system_clock::now();
  const uint64_t ns1970 = now.time_since_epoch().count();

  const auto temp = nanoseconds(ns1970);
  const std::string iso8601 = std::format("{:%FT%TZ}",
    sys_time<nanoseconds>{temp});

  std::cout << iso8601 << std::endl;
  EXPECT_TRUE(iso8601.find( ".") != std::string::npos);

  std::istringstream iso(iso8601);
  sys_time<nanoseconds> conv_time;
  iso >> parse("%FT%TZ", conv_time);
  EXPECT_EQ(ns1970 , conv_time.time_since_epoch().count());
}