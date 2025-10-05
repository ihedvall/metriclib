/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
 */

#pragma once

#include <string>
#include <string_view>
#include <memory>
#include <vector>

#include "metric/metricgroup.h"
#include "metric/metric.h"

namespace metric {

enum class TypeOfDatabase : int {
  Unknown = 0,
  Sqlite = 1,
  DbcFile = 2,
  A2lFile = 3,
};

class MetricDatabase {
 public:
  MetricDatabase() = default;
  virtual ~MetricDatabase() = default;

  [[nodiscard]] TypeOfDatabase Type() const { return type_; }

  static TypeOfDatabase TypeFromString(const std::string& type);
  static std::string_view TypeToString(TypeOfDatabase type);

  void Name(std::string name) { name_ = std::move(name); }
  [[nodiscard]] const std::string& Name() const { return name_; }

  void Description(std::string desc) { description_ = std::move(desc); }
  [[nodiscard]] const std::string& Description() const { return description_; }

  void Filename(std::string filename) { filename_ = std::move(filename); }
  [[nodiscard]] const std::string& Filename() const { return filename_; }

  virtual void Enable(bool enable);

  [[nodiscard]] virtual bool IsEnabled() const {return enabled_; }
  [[nodiscard]] virtual bool IsOperable() const {return operable_; }

  virtual std::shared_ptr<MetricGroup> CreateGroup(std::string name,
                                                   int64_t identity);
  void DeleteGroup(std::string name, int64_t identity);

  const std::vector<std::shared_ptr<MetricGroup>>& Groups() const {
    return group_list_;
  }

  std::shared_ptr<MetricGroup> GetGroupByName(const std::string& name) const;
  std::shared_ptr<MetricGroup> GetGroupByIdentity(int64_t identity) const;
  void SortGroups();

  virtual std::shared_ptr<Metric> CreateMetric(const MetricGroup& group,
    std::string name);
  void DeleteMetric(const MetricGroup& group, std::string name);
  const std::vector<std::shared_ptr<Metric>>& Metrics() const {
    return metric_list_;
  }
  std::vector<std::shared_ptr<Metric>> MetricsByName() const;
  std::vector<std::shared_ptr<Metric>> MetricsByGroupName(const std::string& group_name) const;
  std::vector<std::shared_ptr<Metric>> MetricsByGroupIdentity(int64_t group_identity) const;

  std::shared_ptr<Metric> GetMetricByGroupName(const std::string& group_name,
                           const std::string& metric_name) const;
  std::shared_ptr<Metric> GetMetricByGroupIdentity(int64_t group_identity,
                                  const std::string& metric_name) const;
  void SortMetricsByGroup();
  void SortMetricsByName();

  void Context(void* context) { context_ = context; }
  void* Context() const { return context_; }

 protected:
  std::atomic<bool> enabled_ = false;
  std::atomic<bool> operable_ = false;
  TypeOfDatabase type_ = TypeOfDatabase::Unknown;

  std::vector<std::shared_ptr<MetricGroup>> group_list_;
  std::vector<std::shared_ptr<Metric>> metric_list_;
  void *context_ = nullptr;
 private:
  std::string name_;
  std::string description_;
  std::string filename_;
};

}  // namespace metric


