/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

#include <string>
#include <limits>
#include <gtest/gtest.h>
#include "metrichelper.h"

using namespace metric;

TEST(MetricHelper, TestFloat) {
  {
    constexpr float float_value = 1.0F / 3.0F;
    const std::string float_str = FloatToString(float_value);
    const float float_value2 = std::stof(float_str);
    EXPECT_EQ(float_value, float_value2);
    std::cout << float_str << std::endl;
  }
  {
    constexpr float float_value = std::numeric_limits<float>::max();
    const std::string float_str = FloatToString(float_value);
    const float float_value2 = std::stof(float_str);
    EXPECT_EQ(float_value, float_value2);
    std::cout << float_str << std::endl;
  }
  {
    constexpr float float_value = std::numeric_limits<float>::min();
    const std::string float_str = FloatToString(float_value);
    const float float_value2 = std::stof(float_str);
    EXPECT_EQ(float_value, float_value2);
    std::cout << float_str << std::endl;
  }
  {
    constexpr float float_value = std::numeric_limits<float>::lowest();
    const std::string float_str = FloatToString(float_value);
    const float float_value2 = std::stof(float_str);
    EXPECT_EQ(float_value, float_value2);
    std::cout << float_str << std::endl;
  }
}

TEST(MetricHelper, TestDouble) {
  {
    constexpr double double_value = 1.0 / 3.0;
    const std::string double_str = DoubleToString(double_value);
    const double double_value2 = std::stod(double_str);
    EXPECT_EQ(double_value, double_value2);
    std::cout << double_str << std::endl;
  }
  {
    constexpr double double_value = std::numeric_limits<double>::max();
    const std::string double_str = DoubleToString(double_value);
    const double double_value2 = std::stod(double_str);
    EXPECT_EQ(double_value, double_value2);
    std::cout << double_str << std::endl;
  }
  {
    constexpr double double_value = std::numeric_limits<double>::min();
    const std::string double_str = DoubleToString(double_value);
    const double double_value2 = std::stod(double_str);
    EXPECT_EQ(double_value, double_value2);
    std::cout << double_str << std::endl;
  }
  {
    constexpr double double_value = std::numeric_limits<double>::lowest();
    const std::string double_str = DoubleToString(double_value);
    const double double_value2 = std::stod(double_str);
    EXPECT_EQ(double_value, double_value2);
    std::cout << double_str << std::endl;
  }
}

TEST(MetricHelper, NowNs) {
  const uint64_t now = NowNs();
  EXPECT_GT(now, 0);
}

TEST(MetricHelper, NanoSecToIso8601) {
  const uint64_t now = NowNs();
  const std::string iso_time = NanoSecToIso8601(now);
  std::cout << iso_time << std::endl;
  EXPECT_GT(now, 0);
  EXPECT_TRUE(iso_time.find(".") != std::string::npos);
}

TEST(MetricHelper,Iso8601ToNanoSec) {
  const uint64_t now = NowNs();
  const std::string iso_time = NanoSecToIso8601(now);
  std::cout << iso_time << std::endl;
  EXPECT_GT(now, 0);
  EXPECT_TRUE(iso_time.find(".") != std::string::npos);

  const uint64_t ref_time = Iso8601ToNanoSec(iso_time);
  EXPECT_EQ(ref_time, now);
}