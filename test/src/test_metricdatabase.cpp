/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
 */

#include <cstdint>

#include <gtest/gtest.h>

#include "metric/metricdatabase.h"

using namespace metric;

TEST(MetricDatabase, TestProperties) {
  MetricDatabase database;

  database.Name("Kaiju World");
  EXPECT_EQ(database.Name(), "Kaiju World");

  database.Description("World of monsters");
  EXPECT_EQ(database.Description(), "World of monsters");

  database.Filename("King_Kong.db");
  EXPECT_EQ(database.Filename(), "King_Kong.db");

  database.Enable(true);
  EXPECT_TRUE(database.IsEnabled());
  EXPECT_TRUE(database.IsOperable());
  database.Enable(false);
  EXPECT_FALSE(database.IsEnabled());
  EXPECT_FALSE(database.IsOperable());

  const auto* group = database.CreateGroup("Godzilla", 1);
  ASSERT_TRUE(group != nullptr);
  EXPECT_EQ(group->Name(), "Godzilla");
  EXPECT_EQ(group->Identity(), 1);
  EXPECT_EQ(database.Groups().size(), 1);
  database.DeleteGroup(group->Name(), group->Identity());
  EXPECT_EQ(database.Groups().size(), 0);

  const auto* godzilla_group = database.CreateGroup("Godzillas", 123);
  ASSERT_TRUE(godzilla_group != nullptr);
  const auto* metric = database.CreateMetric(*godzilla_group, "Godzilla");
  ASSERT_TRUE(metric != nullptr);
  EXPECT_EQ(database.Metrics().size(),1);
  database.DeleteMetric(*godzilla_group, "Godzilla");
  EXPECT_EQ(database.Metrics().size(),0);
}

TEST(MetricDatabase, TestTypeConversion) {
  for (size_t index = 0;
    index <= static_cast<size_t>(TypeOfDatabase::A2lFile);
    ++index) {
    TypeOfDatabase type = static_cast<TypeOfDatabase>(index);
    std::string_view text = MetricDatabase::TypeToString(type);
    TypeOfDatabase type1 = MetricDatabase::TypeFromString(std::string(text));
    EXPECT_EQ(type, type1);
  }
}

TEST(MetricDatabase, TestSorting) {
  MetricDatabase database;
  database.Name("Kaiju World");
  database.Description("World of monsters");

  const auto* king_kong_group = database.CreateGroup("King Kongs", 101);
  ASSERT_EQ(database.Groups().size(), 1);
  const auto* metric1 = database.CreateMetric(*king_kong_group, "King Kong 1");
  const auto* metric2 = database.CreateMetric(*king_kong_group, "King Kong 2");
  ASSERT_EQ(database.Metrics().size(), 2);

  const auto* godzilla_group = database.CreateGroup("Godzillas", 101);
  ASSERT_EQ(database.Groups().size(), 2);
  const auto* metric3 = database.CreateMetric(*godzilla_group, "Godzilla 2");
  const auto* metric4 = database.CreateMetric(*godzilla_group, "Godzilla 1");

  ASSERT_EQ(database.Metrics().size(), 4);

  EXPECT_EQ(database.Groups()[0]->Name(), king_kong_group->Name());
  EXPECT_EQ(database.Groups()[1]->Name(), godzilla_group->Name());

  EXPECT_EQ(database.Metrics()[0]->Name(), metric1->Name());
  EXPECT_EQ(database.Metrics()[1]->Name(), metric2->Name());
  EXPECT_EQ(database.Metrics()[2]->Name(), metric3->Name());
  EXPECT_EQ(database.Metrics()[3]->Name(), metric4->Name());

  database.SortGroups();
  EXPECT_EQ(database.Groups()[0]->Name(), godzilla_group->Name());
  EXPECT_EQ(database.Groups()[1]->Name(), king_kong_group->Name());

  database.SortMetricsByGroup();
  EXPECT_EQ(database.Metrics()[0]->Name(), metric4->Name());
  EXPECT_EQ(database.Metrics()[1]->Name(), metric3->Name());
  EXPECT_EQ(database.Metrics()[2]->Name(), metric1->Name());
  EXPECT_EQ(database.Metrics()[3]->Name(), metric2->Name());

  database.SortMetricsByName();
  EXPECT_EQ(database.Metrics()[0]->Name(), metric4->Name());
  EXPECT_EQ(database.Metrics()[1]->Name(), metric3->Name());
  EXPECT_EQ(database.Metrics()[2]->Name(), metric1->Name());
  EXPECT_EQ(database.Metrics()[3]->Name(), metric2->Name());
}

TEST(MetricDatabase, TestAccess) {
  MetricDatabase database;
  database.Name("Kaiju World");
  database.Description("World of monsters");

  const auto* king_kong_group = database.CreateGroup("King Kongs", 101);
  const auto* metric1 = database.CreateMetric(*king_kong_group, "King Kong 1");
  const auto* metric2 = database.CreateMetric(*king_kong_group, "King Kong 2");

  const auto* godzilla_group = database.CreateGroup("Godzillas", 102);
  const auto* metric3 = database.CreateMetric(*godzilla_group, "Godzilla 2");
  const auto* metric4 = database.CreateMetric(*godzilla_group, "Godzilla 1");
  EXPECT_EQ(database.Groups().size(), 2);
  EXPECT_EQ(database.Metrics().size(), 4);

  const auto* godzilla = database.GetGroupByName("Godzillas");
  EXPECT_TRUE(godzilla != nullptr);
  EXPECT_EQ(godzilla_group, godzilla);

  const auto* king_kong = database.GetGroupByIdentity(101);
  EXPECT_TRUE(king_kong != nullptr);
  EXPECT_EQ(king_kong_group, king_kong);

  const auto* godzilla1 = database.GetMetricByGroupName("Godzillas", "Godzilla 1");
  EXPECT_TRUE(godzilla1 != nullptr);
  EXPECT_EQ(godzilla1, metric4);

  const auto* king_kong1 = database.GetMetricByGroupIdentity(101, "King Kong 1");
  EXPECT_TRUE(king_kong1 != nullptr);
  EXPECT_EQ(king_kong1, metric1);

  ASSERT_EQ(database.MetricsByName().size(), 4);
  EXPECT_EQ(database.MetricsByName()[3], metric2);

  EXPECT_EQ(database.MetricsByGroupName("Godzillas").size(), 2);
  EXPECT_EQ(database.MetricsByGroupName("Godzillas")[1], metric3);

  EXPECT_EQ(database.MetricsByGroupIdentity(102).size(), 2);
  EXPECT_EQ(database.MetricsByGroupName("Godzillas")[0], metric4);
}