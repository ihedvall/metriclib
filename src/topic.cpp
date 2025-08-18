/*
 * Copyright 2025 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#include <string_view>
#include <cstring>
#include <sstream>
#include <algorithm>

#include "metric/topic.h"

namespace {
constexpr std::string_view kSparkplugNamespace = "spBv1.0";
}

namespace metric {

void Topic::Name(std::string name) {
  name_ = std::move(name);

  size_t level = 0;
  std::ostringstream temp;
  for (const char in_char : name_) {
    if (in_char == '/') {
      AssignLevelName(level,temp.str());
      temp.str({});
      temp.clear();
      ++level;
    } else {
      temp << in_char;
    }
  }
  if (level > 0 && !temp.str().empty()) {
    AssignLevelName(level,temp.str());
  }

  // Handle special case of STATE message
  if (Namespace() == kSparkplugNamespace && GroupId() == "STATE") {
    NodeId(MessageType());
    MessageType(GroupId());
    GroupId("");
  }
}

const std::string &Topic::Name() const {
  if (name_.empty()) {

    // Assume that the user uses the sparkplug namespace for topics.
    std::ostringstream temp;
    auto add_topic_part = [&] (const std::string& topic_part) {
      if (topic_part.empty()) {
        return;
      }
      if (!temp.str().empty()) {
        temp << "/";
      }
      temp << topic_part;
    };

    add_topic_part(name_space_);
    add_topic_part(group_id_);
    add_topic_part(message_type_);
    add_topic_part(node_id_);
    add_topic_part(device_id_);
    name_ = temp.str();
  }
  return name_;
}

bool Topic::IsUpdated() const {
  std::lock_guard lock(topic_mutex_);
  return std::ranges::any_of(metric_list_, [] (const auto& metric ) -> bool {
    return metric && metric->IsUpdated();
   } );
}

void Topic::ResetUpdated() const {
  std::lock_guard lock(topic_mutex_);
  for (const auto& metric : metric_list_) {
    if (metric) {
      metric->ResetUpdated();
    }
  }
}

bool Topic::IsWildcard() const {
  return strchr(name_.c_str(), '+') != nullptr || strchr(name_.c_str(), '#') != nullptr;
}

void Topic::AssignLevelName(size_t level, const std::string &name) {
  switch (level) {
    case 0:
      if (name_space_.empty()) {
        name_space_ = name;
      }
      break;

    case 1:
      if (group_id_.empty()) {
        group_id_ = name;
      }
      break;

    case 2:
      if (message_type_.empty()) {
        message_type_ = name;
      }
      break;

    case 3:
      if (node_id_.empty()) {
        node_id_ = name;
      }
      break;

    case 4:
      if (device_id_.empty()) {
        device_id_ = name;
      }
      break;

    default:
      break;
  }
}

void Topic::AddMetric(const std::shared_ptr<Metric>& metric) {
  std::scoped_lock lock(topic_mutex_);
  return metric_list_.push_back(metric);
}

void Topic::RemoveMetric(std::string name) {
  std::scoped_lock lock(topic_mutex_);
  std::erase_if(metric_list_, [&name] (auto& metric) -> bool {
    return !metric || metric->Name() == name;
  });
}

std::shared_ptr<Metric> Topic::GetMetric(const std::string &name) const {
  std::scoped_lock lock(topic_mutex_);
  auto itr = std::ranges::find_if(metric_list_, [&] (auto& metric) -> bool {
    return metric && metric->Name() == name;
  });
  return itr != metric_list_.end() ? *itr : std::shared_ptr<Metric>();
}

void Topic::SetAllMetricsInvalid() {
  std::scoped_lock lock(topic_mutex_);
  for ( const auto& metric : metric_list_ ) {
    if (metric) {
      metric->Valid(false);
    }
  }
}

} // end namespace util::mqtt