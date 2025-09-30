/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
 */

#include <cstdint>
#include <array>
#include <string_view>

#include <gtest/gtest.h>

#include "mqtt/detectbroker.h"
#include "metric/metriclogstream.h"

namespace {
constexpr std::array<std::string_view, 5> kBrokerList = {
  "127.0.0.1", // Broker is on this machine
  "192.168.66.21", // Broker is on a NAS
  "test.mosquitto.org", // Mosquitto test broker
  "broker.hivemq.com",  // HiveMQ test broker
  "broker.emqx.io"      // EMQX test broker
};
constexpr uint16_t kDefaultPort = 1883;

}

using namespace mqtt;
using namespace metric;

TEST(TestDetectBroker, DetectBrokerVersion3) {
  MetricLogStream::SetLogFunction(MetricLogStream::LogToConsole);
  METRIC_TRACE() << "DETECT VERSION 3";

  size_t count = 0;
  for (const auto& broker : kBrokerList) {
    DetectBroker detect;
    detect.Listen().SetListenFunction(MetricListen::LogToConsole);

    detect.Host(broker.data());
    detect.Transport(TransportLayer::MqttTcp);
    detect.Port(kDefaultPort);
    const bool found = detect.Init();
    METRIC_TRACE() << detect.Host() << ": " << (found ? "Found" : "Not Found");
    detect.Exit();

    if (found) {
      const std::string& name = detect.Name();
      const std::string& broker_name = detect.Host();
      const ProtocolVersion version = detect.Version();
      std::cout << "Name: " << name << std::endl;
      std::cout << "Broker: " << broker_name << std::endl;
      std::cout << "Version: " << static_cast<int>(version) << std::endl;
      ++count;
    }
    std::cout << std::endl;
  }
  EXPECT_GT(count, 0);
  MetricLogStream::SetLogFunction(nullptr);
}

TEST(TestDetectBroker, DetectBrokerVersion5) {
  MetricLogStream::SetLogFunction(MetricLogStream::LogToConsole);
  METRIC_TRACE() << "DETECT VERSION 5";

  size_t count = 0;
  for (const auto& broker : kBrokerList) {
    DetectBroker detect;
    detect.Listen().SetListenFunction(MetricListen::LogToConsole);
    detect.Host(broker.data());
    detect.Transport(TransportLayer::MqttTcp);
    detect.Port(kDefaultPort);
    detect.Version(ProtocolVersion::Mqtt5);
    const bool found = detect.Init();
    detect.Exit();
    METRIC_TRACE() << detect.Host() << ": " << (found ? "Found" : "Not Found");
    if (found) {
      const std::string& name = detect.Name();
      const std::string& broker_name = detect.Host();
      const ProtocolVersion version = detect.Version();
      std::cout << "Name: " << name << std::endl;
      std::cout << "Broker: " << broker_name << std::endl;
      std::cout << "Version: " << static_cast<int>(version) << std::endl;
      ++count;
    }
    std::cout << std::endl;
  }
  EXPECT_GT(count, 0);
  MetricLogStream::SetLogFunction(nullptr);
}


