/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
 */
#include <cstdint>
#include <string>
#include <array>
#include <algorithm>

#include "metric/metricdatabase.h"

namespace {

constexpr std::array<std::string_view, 4> kTypeList =
    { "Unknown", "SQLite Database", "DBC File", "A2L File" };


struct {
  bool operator()(const std::unique_ptr<metric::MetricGroup>& group1,
                  const std::unique_ptr<metric::MetricGroup>& group2) const {
    if (!group1) {
      return true;
    }
    if (!group2) {
      return false;
    }
    if (group1->Name() < group2->Name()) {
      return true;
    }
    if (group1->Name() > group2->Name()) {
      return false;
    }
    return group1->Identity() < group2->Identity();
  }
} SortGroup;

struct {
  bool operator()(const std::unique_ptr<metric::Metric>& metric1,
                  const std::unique_ptr<metric::Metric>& metric2) const {
    if (!metric1) {
      return true;
    }
    if (!metric2) {
      return false;
    }
    if (metric1->GroupName() < metric2->GroupName()) {
      return true;
    }
    if (metric1->GroupName() > metric2->GroupName()) {
      return false;
    }
    if (metric1->GroupIdentity() < metric2->GroupIdentity()) {
      return true;
    }
    if (metric1->GroupIdentity() > metric2->GroupIdentity()) {
      return false;
    }
    return metric1->Name() < metric2->Name();
  }
} SortMetricByGroup;

struct {
  bool operator()(const std::unique_ptr<metric::Metric>& metric1,
                  const std::unique_ptr<metric::Metric>& metric2) const {
    if (!metric1) {
      return true;
    }
    if (!metric2) {
      return false;
    }
    return metric1->Name() < metric2->Name();
  }
} SortMetricByName;

struct {
  bool operator()(const metric::Metric* metric1,
                  const metric::Metric* metric2) const {
    if (metric1 == nullptr) {
      return true;
    }
    if (metric2 == nullptr) {
      return false;
    }
    return metric1->Name() < metric2->Name();
  }
} SortMetricByNamePointer;

}

namespace metric {

void MetricDatabase::Enable(bool enable) {
  enabled_ = enable;
  operable_ = enable;
}

MetricGroup* MetricDatabase::CreateGroup(std::string name, int32_t identity) {
  auto itr = std::ranges::find_if(group_list_, [&](const auto& group) -> bool {
    return group && group->Name() == name && group->Identity() == identity;
  });
  if (itr == group_list_.end()) {
    if (auto new_group = std::make_unique<MetricGroup>(); new_group) {
      new_group->Name(std::move(name));
      new_group->Identity(identity);
      group_list_.emplace_back(std::move(new_group));
    } else {
      return itr->get();
    }
  }
  return group_list_.back().get();
}

void MetricDatabase::DeleteGroup(std::string name, uint32_t identity) {
  std::erase_if(group_list_, [&](const auto& group) -> bool {
    return !group || (group->Name() == name && group->Identity() == identity);
  });
}

Metric* MetricDatabase::CreateMetric(const MetricGroup& group,
                                     std::string name) {
  auto itr =
      std::ranges::find_if(metric_list_, [&](const auto& metric) -> bool {
        return metric && group.Name() == metric->GroupName() &&
               group.Identity() == metric->GroupIdentity() &&
               metric->Name() == name;
      });
  if (itr == metric_list_.end()) {
    if (auto new_metric = std::make_unique<Metric>(); new_metric) {
      new_metric->Name(std::move(name));
      new_metric->GroupName(group.Name());
      new_metric->GroupIdentity(group.Identity());
      metric_list_.emplace_back(std::move(new_metric));
    } else {
      return itr->get();
    }
  }
  return metric_list_.back().get();
}

void MetricDatabase::DeleteMetric(const MetricGroup& group, std::string name) {
  std::erase_if(metric_list_, [&](const auto& metric) -> bool {
    return !metric || (metric->GroupName() == group.Name() &&
                       metric->GroupIdentity() == group.Identity() &&
                       metric->Name() == name);
  });
}

std::string_view MetricDatabase::TypeToString(TypeOfDatabase type) {
  for (size_t index = 0; index < kTypeList.size(); ++index) {
    const auto db_type = static_cast<TypeOfDatabase>(index);
    if (db_type == type) {
      return kTypeList[index];
    }
  }
  return kTypeList[0];
}

TypeOfDatabase MetricDatabase::TypeFromString(const std::string& type) {
  for (size_t index = 0; index < kTypeList.size(); ++index) {
    const auto db_type = static_cast<TypeOfDatabase>(index);
    if (kTypeList[index] == type) {
      return db_type;
    }
  }
  return TypeOfDatabase::Unknown;
}

MetricGroup* MetricDatabase::GetGroupByName(const std::string& name) const {
  auto itr =
      std::ranges::find_if(group_list_, [&name](const auto& group) -> bool {
        return group && group->Name() == name;
      });
  return itr != group_list_.end() ? itr->get() : nullptr;
}

MetricGroup* MetricDatabase::GetGroupByIdentity(int64_t identity) const {
  auto itr =
      std::ranges::find_if(group_list_, [&identity](const auto& group) -> bool {
        return group && group->Identity() == identity;
      });
  return itr != group_list_.end() ? itr->get() : nullptr;
}

Metric* MetricDatabase::GetMetricByGroupName(
    const std::string& group_name, const std::string& metric_name) const {
  auto itr =
      std::ranges::find_if(metric_list_, [&](const auto& metric) -> bool {
        return metric && metric->GroupName() == group_name &&
               metric->Name() == metric_name;
      });
  return itr != metric_list_.end() ? itr->get() : nullptr;
}

Metric* MetricDatabase::GetMetricByGroupIdentity(
    int64_t group_identity, const std::string& metric_name) const {
  auto itr =
      std::ranges::find_if(metric_list_, [&](const auto& metric) -> bool {
        return metric && metric->GroupIdentity() == group_identity &&
               metric->Name() == metric_name;
      });
  return itr != metric_list_.end() ? itr->get() : nullptr;
}

void MetricDatabase::SortGroups() {
  std::ranges::sort(group_list_, SortGroup);
}

void MetricDatabase::SortMetricsByGroup() {
  std::ranges::sort(metric_list_, SortMetricByGroup);
}

void MetricDatabase::SortMetricsByName() {
  std::ranges::sort(metric_list_, SortMetricByName);
}

std::vector<Metric*> MetricDatabase::MetricsByName() const {
  std::vector<Metric*> dest_list;
  dest_list.reserve(metric_list_.size());
  for (const auto& metric : metric_list_) {
    if (!metric) {
      continue;
    }
    dest_list.emplace_back(metric.get());
  }
  std::ranges::sort(dest_list, SortMetricByNamePointer);
  return dest_list;
}

std::vector<Metric*> MetricDatabase::MetricsByGroupName(
    const std::string& group_name) const {
  std::vector<Metric*> dest_list;
  for (const auto& metric : metric_list_) {
    if (!metric ||
        metric->GroupName() != group_name) {
      continue;
    }
    dest_list.emplace_back(metric.get());
  }
  std::ranges::sort(dest_list, SortMetricByNamePointer);
  return dest_list;
}

std::vector<Metric*> MetricDatabase::MetricsByGroupIdentity(
    int64_t group_identity) const {
  std::vector<Metric*> dest_list;
  for (const auto& metric : metric_list_) {
    if (!metric ||
        metric->GroupIdentity() != group_identity) {
      continue;
    }
    dest_list.emplace_back(metric.get());
  }
  std::ranges::sort(dest_list, SortMetricByNamePointer);
  return dest_list;
}

}  // namespace metric