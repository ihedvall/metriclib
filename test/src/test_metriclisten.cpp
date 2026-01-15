/*
* Copyright 2026 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

#include <string>
#include <stdexcept>
#include <cstdint>

#include <gtest/gtest.h>

#include "metric/metriclisten.h"

using namespace metric;

namespace {

size_t kListenCount = 0;
void TestListenFunction(const std::string& pre_text, uint64_t timestamp,
  const std::string& text) {
  ++kListenCount;
  MetricListen::LogToConsole(pre_text, timestamp, text);
}

}

TEST(MetricListen, TestProperty) {

  MetricListen listen;
  listen.SetListenFunction(TestListenFunction);

  EXPECT_TRUE(listen.IsActive());

  listen.PreText("TEST");
  EXPECT_EQ(listen.PreText(), "TEST");

  listen.LogLevel(2);
  EXPECT_EQ(listen.LogLevel(), 2);

  kListenCount = 0;
  listen.ListenString("Test1");
  EXPECT_EQ(kListenCount, 1);

  try {
    throw std::runtime_error("Test exception");
  } catch (const std::exception& err) {
    listen.ListenArgs("Test2. Error: ", err.what());
  }
  EXPECT_EQ(kListenCount, 2);

  listen.ListenArgs("Test3. Count: ", kListenCount, ", Error: ", "Test error");

  listen.ResetListenFunction();


}