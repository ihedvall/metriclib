/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>

#include "metric/metriclogstream.h"


using namespace metric;

TEST(MetricLogStream, TestLogToConsole) {
  MetricLogStream::SetLogFunction(MetricLogStream::LogToConsole);

  METRIC_TRACE() << "Trace message";
  METRIC_DEBUG() << "Debug message";
  METRIC_INFO() << "Info message";
  METRIC_ERROR() << "Error message";
  MetricLogStream::ResetLogFunction();
}

TEST(MetricLogStream, TestNoLocation) {
  MetricLogStream::SetLogFunction(MetricLogStream::LogToConsole);
  MetricLogStream::ShowLocation(false);

  METRIC_TRACE() << "Trace message";
  METRIC_DEBUG() << "Debug message";
  METRIC_INFO() << "Info message";
  METRIC_ERROR() << "Error message";
}

TEST(MetricLogStream, TestErrorCount) {
  MetricLogStream::SetLogFunction(MetricLogStream::LogToConsole);
  MetricLogStream::ShowLocation(false);
  MetricLogStream::ResetErrorCount();

  METRIC_TRACE() << "Trace message";
  METRIC_DEBUG() << "Debug message";
  METRIC_INFO() << "Info message";
  METRIC_NOTICE() << "Notice message";
  METRIC_WARNING() << "Warning message";
  METRIC_ERROR() << "Error message";
  METRIC_CRITICAL() << "Critical message";
  METRIC_ALERT() << "Alert message";
  METRIC_EMERGENCY() << "Emergency message";

  // All message with Error and larger severity is treated as an error message.
  EXPECT_EQ(MetricLogStream::ErrorCount(), 4);

}