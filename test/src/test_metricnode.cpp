/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/



#include <gtest/gtest.h>

#include "metric/metricnode.h"

using namespace metric;

TEST(MetricNode, TestProperties) {
  MetricNode node;

  node.Name("Node1");
  EXPECT_EQ(node.Name(), "Node1");

  node.Description("Desc1");
  EXPECT_EQ(node.Description(), "Desc1");

  for (int index = 0;
       index <= static_cast<int>(TransportLayer::MqttWebSocketTls);
       ++index ) {
    const auto layer = static_cast<TransportLayer>(index);
    node.Transport(layer);
    EXPECT_EQ(layer, node.Transport());
  }

  node.Host("Host1");
  EXPECT_EQ(node.Host(), "Host1");

  node.Port(1234);
  EXPECT_EQ(node.Port(), 1234);

  node.ClientId("Ident1");
  EXPECT_EQ(node.ClientId(), "Ident1");

  node.UserName("User1");
  EXPECT_EQ(node.UserName(), "User1");

  node.Password("Password1");
  EXPECT_EQ(node.Password(), "Password1");

  for (int index = 0;
       index <= static_cast<int>(ProtocolVersion::Mqtt5);
       ++index ) {
    const auto version = static_cast<ProtocolVersion>(index);
    node.Version(version);
    EXPECT_EQ(version, node.Version());
  }

  auto topic = node.CreateTopic("Topic1");
  ASSERT_TRUE(topic);
  EXPECT_EQ(topic->Name(), "Topic1");

  auto topic1 = node.GetTopic("Topic1");
  ASSERT_TRUE(topic1);
  EXPECT_EQ(topic1->Name(), "Topic1");
  EXPECT_EQ(node.Topics().size(), 1);
  node.DeleteTopic(topic->Name());
  EXPECT_EQ(node.Topics().size(), 0);

  node.AddSubscription("sub1", QualityOfService::Qos2);
  EXPECT_EQ(node.Subscriptions().size(), 1);
  node.DeleteSubscription("sub1");
  EXPECT_EQ(node.Subscriptions().size(), 0);
}