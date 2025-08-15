/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

#include <cstdint>

#include <gtest/gtest.h>

#include "metric/metric.h"
#include "metric/metricproperty.h"

using namespace metric;

TEST(Metric, TestProperties) {
  Metric metric;
  metric.Name("Donald");
  EXPECT_EQ(metric.Name(), "Donald");

  metric.GroupName("Disney");
  EXPECT_EQ(metric.GroupName(), "Disney");

  metric.Identity(0x1234);
  EXPECT_EQ(metric.Identity(), 0x1234);

  metric.Timestamp(2345);
  EXPECT_EQ(metric.Timestamp(), 2345);

  metric.Description("Last name Duck");
  EXPECT_EQ(metric.Description(), "Last name Duck");

  metric.Unit("Duck");
  EXPECT_EQ(metric.Unit(), "Duck");

  for (int index = 0; index <= static_cast<int>(MetricType::DateTimeArray);
       ++index) {
    const auto data_type = static_cast<MetricType>(index);
    metric.DataType(data_type);
    EXPECT_EQ(metric.DataType(), data_type);

    const std::string_view data_type_text = Metric::DataTypeToString(data_type);
    EXPECT_EQ(data_type, Metric::StringToDataType(std::string(data_type_text)));
  }

  metric.Historical(true);
  EXPECT_TRUE(metric.IsHistorical());
  metric.Historical(false);
  EXPECT_FALSE(metric.IsHistorical());

    metric.Transient(true);
    EXPECT_TRUE(metric.IsTransient());
    metric.Transient(false);
    EXPECT_FALSE(metric.IsTransient());

    metric.Null(true);
    EXPECT_TRUE(metric.IsNull());
    metric.Null(false);
    EXPECT_FALSE(metric.IsNull());

    metric.Valid(true);
    EXPECT_TRUE(metric.IsValid());
    metric.Valid(false);
    EXPECT_FALSE(metric.IsValid());

    metric.Valid(true);
    EXPECT_TRUE(metric.IsValid());
    metric.Valid(false);
    EXPECT_FALSE(metric.IsValid());

    metric.ReadOnly(true);
    EXPECT_TRUE(metric.IsReadOnly());
    metric.ReadOnly(false);
    EXPECT_FALSE(metric.IsReadOnly());

    EXPECT_EQ(metric.Properties().size(), 2); // Description, Unit should exist
    MetricProperty partner_prop("Partner","Goofy");
    metric.AddProperty(partner_prop);
    EXPECT_EQ(metric.Properties().size(), 3);
    EXPECT_TRUE(metric.GetProperty("Partner") != nullptr);
    metric.DeleteProperty("Partner");
    EXPECT_EQ(metric.Properties().size(), 2);

    const auto* goofy_prop = metric.CreateProperty("Partner");
    EXPECT_TRUE(goofy_prop != nullptr);
    EXPECT_EQ(metric.Properties().size(), 3);
    EXPECT_TRUE(metric.GetProperty("Partner") != nullptr);
    metric.DeleteProperty("Partner");
    EXPECT_EQ(metric.Properties().size(), 2);

    metric.SetUpdated();
    EXPECT_TRUE(metric.IsUpdated());
    metric.ResetUpdated();
    EXPECT_FALSE(metric.IsUpdated());
}

TEST(Metric, TestValues) {
  Metric metric;

  metric.DataType(MetricType::Boolean);
  metric.Value(true);
  EXPECT_TRUE(metric.Value<bool>());
  metric.Value(false);
  EXPECT_FALSE(metric.Value<bool>());

  metric.DataType(MetricType::Int8);
  metric.Value(-9);
  EXPECT_EQ(metric.Value<int8_t>(), -9);

  metric.DataType(MetricType::Int16);
  metric.Value(-99);
  EXPECT_EQ(metric.Value<int16_t>(), -99);

  metric.DataType(MetricType::UInt8);
  metric.Value(9);
  EXPECT_EQ(metric.Value<uint8_t>(), 9);

  metric.DataType(MetricType::UInt16);
  metric.Value(99);
  EXPECT_EQ(metric.Value<uint16_t>(), 99);

  constexpr float float_value = 1.0F/3;
  metric.DataType(MetricType::Float);
  metric.Value(float_value);
  EXPECT_EQ(metric.Value<float>(), float_value);

  constexpr double double_value = 1.0/3;
  metric.DataType(MetricType::Double);
  metric.Value(double_value);
  EXPECT_EQ(metric.Value<double>(), double_value);

  constexpr std::string_view string_value = "Blatter";
  metric.DataType(MetricType::String);
  metric.Value(string_value);
  EXPECT_EQ(metric.Value<std::string>(), string_value);

  metric.DataType(MetricType::DateTime);
  metric.Value<uint64_t>(0);
  EXPECT_EQ(metric.Value<uint64_t>(), 0);
  const std::string date_string = metric.Value<std::string>();
  std::cout << date_string << std::endl;
  EXPECT_GT(date_string.size(), 0);
  metric.Value(date_string);
  EXPECT_EQ(metric.Value<uint64_t>(), 0);

}