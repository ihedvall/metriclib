/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
 */

#include <cstdint>

#include <gtest/gtest.h>

#include "metric/metricgroup.h"

using namespace metric;

TEST(MetricGroup, TestProperties) {
  MetricGroup group;

  group.Name("Kaiju");
  EXPECT_EQ(group.Name(), "Kaiju");

  group.Description("Radio active monster");
  EXPECT_EQ(group.Description(), "Radio active monster");

  group.Type(TypeOfGroup::CanMessage);
  EXPECT_EQ(group.Type(), TypeOfGroup::CanMessage);

  group.Identity(0x1234);
  EXPECT_EQ(group.Identity(), 0x1234);

}