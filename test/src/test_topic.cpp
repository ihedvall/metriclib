/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>

#include <cstdint>

#include "metric/topic.h"
#include "metric/metricdatabase.h"

using namespace metric;

TEST(Topic, TestProperties) {
  Topic topic;

  topic.Name("Rubber Duck");
  EXPECT_EQ(topic.Name(), "Rubber Duck");

  topic.Description("Debugging Technique");
  EXPECT_EQ(topic.Description(), "Debugging Technique");

  topic.Namespace("Main");
  EXPECT_EQ(topic.Namespace(), "Main");

  topic.GroupId("General");
  EXPECT_EQ(topic.GroupId(), "General");

  topic.MessageType("PIVOT");
  EXPECT_EQ(topic.MessageType(), "PIVOT");

  topic.NodeId("ComNode");
  EXPECT_EQ(topic.NodeId(), "ComNode");

  topic.DeviceId("CANDevice");
  EXPECT_EQ(topic.DeviceId(), "CANDevice");

  topic.ContentType("text/plain");
  EXPECT_EQ(topic.ContentType(), "text/plain");
  EXPECT_TRUE(topic.IsText());

  topic.ContentType("application/json");
  EXPECT_EQ(topic.ContentType(), "application/json");
  EXPECT_TRUE(topic.IsJson());

  topic.ContentType("application/xml");
  EXPECT_EQ(topic.ContentType(), "application/xml");
  EXPECT_TRUE(topic.IsXml());

  topic.ContentType("application/protobuf");
  EXPECT_EQ(topic.ContentType(), "application/protobuf");
  EXPECT_TRUE(topic.IsProtobuf());

  topic.Publishing(true);
  EXPECT_TRUE(topic.IsPublishing());
  topic.Publishing(false);
  EXPECT_FALSE(topic.IsPublishing());

  for (size_t index = 0; index <= static_cast<size_t>(QualityOfService::Qos2);
       ++index) {
    const QualityOfService qos = static_cast<QualityOfService>(index);
    topic.Qos(qos);
    EXPECT_EQ(topic.Qos(), qos);
  }

  topic.Retained(true);
  EXPECT_TRUE(topic.IsRetained());
  topic.Retained(false);
  EXPECT_FALSE(topic.IsRetained());

  EXPECT_FALSE(topic.IsWildcard());
  topic.Name("Olle/Pelle/#");
  EXPECT_TRUE(topic.IsWildcard());

  MetricDatabase database;
  const auto metric_group = database.CreateGroup("Morningstar", 666);
  EXPECT_TRUE(metric_group);
  const auto metric = database.CreateMetric(*metric_group, "Decker");
  EXPECT_TRUE(metric);

  topic.AddMetric(metric);
  EXPECT_EQ(topic.Metrics().size(), 1);

  const auto* const_topic = &topic;
  ASSERT_TRUE(const_topic != nullptr);
  EXPECT_EQ(const_topic->Metrics().size(), 1);

  auto metric1 = topic.GetMetric("Decker");
  EXPECT_TRUE(metric1);
  EXPECT_EQ(metric1, metric);

  EXPECT_FALSE(topic.IsUpdated());
  metric->Value("Mazikeen");
  EXPECT_TRUE(topic.IsUpdated());
  topic.ResetUpdated();
  EXPECT_FALSE(topic.IsUpdated());
  metric->Value("Lopez");
  EXPECT_TRUE(topic.IsUpdated());

  EXPECT_TRUE(metric->IsValid());
  topic.SetAllMetricsInvalid();
  EXPECT_FALSE(metric->IsValid());

  topic.RemoveMetric(metric->Name());
  EXPECT_EQ(topic.Metrics().size(), 0);

}

TEST(Topic, TestSparklugNaming) {
  // <namespace>/<group_id>/<message_type>/<node_id>/<device_id>
  {
    Topic topic;
    topic.Name("NS/GID/STATE");
    EXPECT_EQ(topic.Namespace(), "NS");
    EXPECT_EQ(topic.GroupId(), "GID");
    EXPECT_EQ(topic.MessageType(), "STATE");
    EXPECT_EQ(topic.NodeId(), "");
    EXPECT_EQ(topic.DeviceId(), "");
  }

  {
    Topic topic;
    topic.Name("NS1/GID1/MTYPE1/NID1/DID1");
    EXPECT_EQ(topic.Namespace(), "NS1");
    EXPECT_EQ(topic.GroupId(), "GID1");
    EXPECT_EQ(topic.MessageType(), "MTYPE1");
    EXPECT_EQ(topic.NodeId(), "NID1");
    EXPECT_EQ(topic.DeviceId(), "DID1");
  }
}