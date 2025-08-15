/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
 */

#include <cstdint>

#include <gtest/gtest.h>

#include "metric/metricproperty.h"

using namespace metric;

TEST(MetricProperty, TestProperties) {
  MetricProperty prop;
  prop.Key("Donald");
  EXPECT_EQ(prop.Key(), "Donald");

  for (int index = 0; index <= static_cast<int>(MetricType::DateTimeArray);
       ++index) {
    const auto data_type = static_cast<MetricType>(index);
    prop.DataType(data_type);
    EXPECT_EQ(prop.DataType(), data_type);
  }
  prop.Null(true);
  EXPECT_TRUE(prop.IsNull());
  prop.Null(false);
  EXPECT_FALSE(prop.IsNull());


}

TEST(MetricProperty, TestValues) {
  MetricProperty prop;

  prop.DataType(MetricType::Boolean);
  prop.Value(true);
  EXPECT_TRUE(prop.Value<bool>());
  prop.Value(false);
  EXPECT_FALSE(prop.Value<bool>());

  prop.DataType(MetricType::Int8);
  prop.Value(-9);
  EXPECT_EQ(prop.Value<int8_t>(), -9);

  prop.DataType(MetricType::Int16);
  prop.Value(-99);
  EXPECT_EQ(prop.Value<int16_t>(), -99);

  prop.DataType(MetricType::UInt8);
  prop.Value(9);
  EXPECT_EQ(prop.Value<uint8_t>(), 9);

  prop.DataType(MetricType::UInt16);
  prop.Value(99);
  EXPECT_EQ(prop.Value<uint16_t>(), 99);

  constexpr float float_value = 1.0F/3;
  prop.DataType(MetricType::Float);
  prop.Value(float_value);
  EXPECT_EQ(prop.Value<float>(), float_value);

  constexpr double double_value = 1.0/3;
  prop.DataType(MetricType::Double);
  prop.Value(double_value);
  EXPECT_EQ(prop.Value<double>(), double_value);

  constexpr std::string_view string_value = "Blatter";
  prop.DataType(MetricType::String);
  prop.Value(string_value);
  EXPECT_EQ(prop.Value<std::string>(), string_value);

  prop.DataType(MetricType::DateTime);
  prop.Value<uint64_t>(0);
  EXPECT_EQ(prop.Value<uint64_t>(), 0);
  const std::string date_string = prop.Value<std::string>();
  std::cout << date_string << std::endl;
  EXPECT_GT(date_string.size(), 0);
  prop.Value(date_string);
  EXPECT_EQ(prop.Value<uint64_t>(), 0);
}