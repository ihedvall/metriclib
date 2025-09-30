/*
 * Copyright 2024 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#include <array>
#include <chrono>
#include <string>
#include <string_view>
#include <thread>
#include <memory>

#include <MQTTAsync.h>

#include "metric/metric.h"
#include "metric/metriclogstream.h"
#include "mqtt/detectbroker.h"
#include "mqtt/mqttnode.h"

#include "testmqtt.h"

using namespace std::chrono_literals;
using namespace metric;

namespace {

constexpr std::array<std::string_view, 5> kBrokerList = {
    "127.0.0.1",
    "192.168.66.21",
    "test.mosquitto.org",
    "broker.hivemq.com",  // HiveMQ test broker
    "broker.emqx.io"      // EMQX test broker
};


std::string kBroker;
std::string kBrokerName;
ProtocolVersion kBrokerVersion_ = ProtocolVersion::Mqtt5;

} // end namespace

namespace mqtt {

void TestMqtt::SetUpTestSuite() {
  MetricLogStream::SetLogFunction(MetricLogStream::LogToConsole);

  for ( const auto& broker : kBrokerList) {
    DetectBroker detect;
    detect.Host(broker.data());
    detect.Port(1883);
    detect.Version(ProtocolVersion::Mqtt5);
    const bool exist5 = detect.Init();
    detect.Exit();
    if (exist5) {
      kBroker = detect.Host();
      kBrokerName = detect.Name();
      kBrokerVersion_ = detect.Version();
      break;
    }
  }

  METRIC_TRACE() << "Test broker. Broker: " << kBrokerName;

}

void TestMqtt::TearDownTestSuite() {
  MetricLogStream::SetLogFunction(nullptr);
}

TEST_F(TestMqtt, TestConnection) {
  if (kBroker.empty()) {
    GTEST_SKIP();
  }

  MqttNode publisher;
  publisher.Listen().SetListenFunction(MetricListen::LogToConsole);
  publisher.Listen().PreText("PUB");

  publisher.Host(kBroker);
  publisher.Port(1883);
  publisher.Name("Pub");
  publisher.Version(ProtocolVersion::Mqtt5);

  EXPECT_FALSE(publisher.IsConnected());
  EXPECT_TRUE(publisher.Init());
  publisher.InService();

  for (size_t connect = 0; connect < 50; ++connect) {
    if (publisher.IsConnected()) {
      break;
    }
    std::this_thread::sleep_for(100ms);
  }
  ASSERT_TRUE(publisher.IsConnected());

  EXPECT_TRUE(publisher.Exit());

  for (size_t disconnect = 0; disconnect < 1000; ++disconnect) {
    if (!publisher.IsConnected()) {
      break;
    }
    std::this_thread::sleep_for(1ms);
  }
  EXPECT_FALSE(publisher.IsConnected());
}

TEST_F(TestMqtt, MqttClient) {
  if (kBroker.empty()) {
    GTEST_SKIP();
  }

  MqttNode publisher;
  publisher.Listen().SetListenFunction(MetricListen::LogToConsole);
  publisher.Listen().PreText("PUB");

  publisher.Host(kBroker);
  publisher.Port(1883);
  publisher.Name("Pub");
  publisher.Version(ProtocolVersion::Mqtt5);
  publisher.InService();

  constexpr std::string_view topic_name = "ihedvall/test/mqtt/string_value";
  auto write_topic = publisher.CreateTopic(topic_name.data());
  ASSERT_TRUE(write_topic);
  write_topic->Qos(QualityOfService::Qos1);
  write_topic->Retained(true);
  write_topic->Publishing(true);
  write_topic->ContentType("text/plain");

  std::shared_ptr<Metric> write_metric =
      write_topic->CreateMetric("string_value");
  write_metric->DataType(MetricType::String);
  write_metric->Value("StringVal"); // Initial value

  EXPECT_FALSE(publisher.IsConnected());
  EXPECT_TRUE(publisher.Init());


  MqttNode subscriber;
  subscriber.Listen().SetListenFunction(MetricListen::LogToConsole);
  subscriber.Listen().PreText("SUB");

  subscriber.Host(kBroker);
  subscriber.Port(1883);
  subscriber.Name("Sub");
  subscriber.Version(ProtocolVersion::Mqtt5);

  subscriber.AddSubscription(topic_name.data(), QualityOfService::Qos1);

  auto read_topic = subscriber.CreateTopic(topic_name.data());
  ASSERT_TRUE(read_topic);
  read_topic->Qos(QualityOfService::Qos1);
  read_topic->Retained(true);
  read_topic->Publishing(false);

  bool value_read = false;
  auto read_value = read_topic->CreateMetric("string_value");
  read_value->DataType(MetricType::String);

  read_topic->SetOnTopicMessage([&] () -> void {
    value_read = true;
  });

  EXPECT_FALSE(subscriber.IsConnected());
  EXPECT_TRUE(subscriber.Init());


  // Check that both clients are connected
  for (size_t connect = 0; connect < 50; ++connect) {
    if (publisher.IsOnline() && subscriber.IsOnline()) {
      break;
    }
    std::this_thread::sleep_for(100ms);
  }


  ASSERT_TRUE(publisher.IsOnline());
  ASSERT_TRUE(subscriber.IsOnline());

  // Flush out any retained values
  std::this_thread::sleep_for(900ms);

  // Publish some dummy values
  for (size_t index = 0; index < 10; ++index) {
    value_read = false;
    std::ostringstream temp;
    temp << "Pelle_" << index;
    write_metric->Value(temp.str());
    publisher.PublishTopics();


    for (size_t timeout = 0; timeout < 20; ++timeout) {
      if (value_read) {
        break;
      }
      std::this_thread::sleep_for(100ms);
    }
    EXPECT_TRUE(value_read) << "No value read. Value: " << read_value->Value<std::string>();
  }
  EXPECT_EQ(read_topic->ContentType(), "text/plain");
  publisher.Exit();
  subscriber.Exit();
  // Check that both clients are connected
  for (size_t disconnect = 0; disconnect < 1000; ++disconnect) {
    if (!publisher.IsConnected() && !subscriber.IsConnected()) {
      break;
    }
    std::this_thread::sleep_for(1ms);
  }
  EXPECT_FALSE(publisher.IsConnected());
  EXPECT_FALSE(subscriber.IsConnected());
}

TEST_F(TestMqtt, MqttPlainText) {
  if (kBroker.empty()) {
    GTEST_SKIP();
  }

  MqttNode publisher;
  publisher.Listen().SetListenFunction(MetricListen::LogToConsole);
  publisher.Listen().PreText("PUB");

  publisher.Host(kBroker);
  publisher.Port(1883);
  publisher.Name("Pub");
  publisher.Version(ProtocolVersion::Mqtt5);

  constexpr std::string_view topic_name = "ihedvall/test/mqtt/string_value";
  auto write_topic = publisher.CreateTopic(topic_name.data());
  ASSERT_TRUE(write_topic);
  write_topic->Qos(QualityOfService::Qos1);
  write_topic->Retained(true);
  write_topic->Publishing(true);
  write_topic->ContentType("text/plain");

  std::shared_ptr<Metric> write_metric =
      write_topic->CreateMetric("string_value");
  write_metric->DataType(MetricType::String);
  write_metric->Value("StringVal"); // Initial value

  EXPECT_FALSE(publisher.IsConnected());
  EXPECT_TRUE(publisher.Init());


  MqttNode subscriber;
  subscriber.Listen().SetListenFunction(MetricListen::LogToConsole);
  subscriber.Listen().PreText("SUB");

  subscriber.Host(kBroker);
  subscriber.Port(1883);
  subscriber.Name("Sub");
  subscriber.Version(ProtocolVersion::Mqtt5);

  constexpr std::string_view subscription = "ihedvall/test/mqtt/#";
  subscriber.AddSubscription(subscription.data(), QualityOfService::Qos1);

  // Do not create any topics or metrics.

  EXPECT_TRUE(subscriber.Init());

  // Check that both clients are connected
  for (size_t connect = 0; connect < 50; ++connect) {
    if (publisher.IsOnline() && subscriber.IsOnline()) {
      break;
    }
    std::this_thread::sleep_for(100ms);
  }


  ASSERT_TRUE(publisher.IsOnline());
  ASSERT_TRUE(subscriber.IsOnline());

  // Flush out any retained values
  std::this_thread::sleep_for(900ms);

  // Publish some dummy values
  write_metric->Value("Pelle_1");
  publisher.PublishTopics();

  for (size_t timeout = 0; timeout < 20; ++timeout) {
    if (!subscriber.Topics().empty()) {
      break;
    }
    std::this_thread::sleep_for(100ms);
  }
  EXPECT_GT(subscriber.Topics().size(), 0);
  auto read_topic = subscriber.GetTopic(topic_name.data());
  ASSERT_TRUE(read_topic);
  EXPECT_TRUE(read_topic->IsUpdated());
  EXPECT_EQ(read_topic->Metrics().size(), 1);
  EXPECT_EQ(read_topic->ContentType(), "text/plain");
  publisher.Exit();
  subscriber.Exit();

}

} // end namespace