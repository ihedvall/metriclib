/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
 */
#include "metric/metricnode.h"

#include <chrono>

using namespace std::chrono;

namespace metric {

MetricNode::~MetricNode() {
  MetricNode::Exit();
}

bool MetricNode::Init() {
  initialized_ = true;
  return true;
}

bool MetricNode::Exit() {
  initialized_ = false;
  return true;
}

void MetricNode::InService() {
  in_service_ = true;
}

void MetricNode::OutOfService() {
  in_service_ = false;
  connected_ = false;
  operable_ = false;
}

bool MetricNode::IsInService() const {
  return in_service_;
}

bool MetricNode::IsConnected() const {
  return connected_;
}

bool MetricNode::IsOperable() const {
  return operable_;
}

std::shared_ptr<Topic> MetricNode::CreateTopic(std::string name) {
  std::scoped_lock list_lock(topic_mutex_);
  auto exist = std::ranges::find_if(topic_list_, [&name] (auto& topic) -> bool {
    return topic && topic->Name() == name;
  });
  if (exist != topic_list_.end()) {
    return *exist;
  }
  auto new_topic = std::make_shared<Topic>();
  new_topic->Name(std::move(name));
  topic_list_.push_back(std::move(new_topic));
  return topic_list_.back();
}

std::shared_ptr<Topic> MetricNode::GetTopic(
                                            const std::string& name) const {
  std::scoped_lock list_lock(topic_mutex_);
  const auto exist = std::ranges::find_if(topic_list_,
                                          [&name] (const auto& topic) -> bool {
    return topic && topic->Name() == name;
  });
  if (exist != topic_list_.end()) {
    return *exist;
  }
  return {};
}

std::shared_ptr<Topic> MetricNode::GetTopicByMessageType(
    const std::string& message_type) {
  std::scoped_lock list_lock(topic_mutex_);
  const auto exist = std::ranges::find_if(topic_list_,
                         [&message_type] (const auto& topic) -> bool {
    return topic && topic->MessageType() == message_type;
  });
  if (exist != topic_list_.end()) {
    return *exist;
  }
  return {};
}

void MetricNode::DeleteTopic(std::string name) {
  std::scoped_lock list_lock(topic_mutex_);
  std::erase_if(topic_list_, [&name](const auto& topic) -> bool {
    return topic && topic->Name() == name;
  });
}

void MetricNode::AddSubscription(std::string subscription,
  QualityOfService quality_of_service) {
  if (subscription.empty()) {
    return;
  }
  const auto exist = std::ranges::find_if(subscription_list_,
                    [&subscription] (const auto& sub) -> bool {
    return sub.subscription == subscription;
  });
  if (exist == subscription_list_.cend()) {
    subscription_list_.emplace_back(std::move(subscription), quality_of_service);
  }
}

void MetricNode::DeleteSubscription(std::string subscription) {
  std::erase_if(subscription_list_, [&subscription] (const auto& sub) -> bool {
    return sub.subscription == subscription;
  });
}

uint64_t MetricNode::NowMs() {
  const sys_time utc = system_clock::now();
  const nanoseconds dur = utc.time_since_epoch();
  return static_cast<uint64_t>(dur.count()) / 1'000'000;
}

uint64_t MetricNode::NowNs() {
  const sys_time utc = system_clock::now();
  const nanoseconds dur = utc.time_since_epoch();
  return static_cast<uint64_t>(dur.count());
}

}  // namespace metric