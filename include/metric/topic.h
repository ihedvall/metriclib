/*
 * Copyright 2025 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <string>
#include <utility>
#include <vector>
#include <cstdint>
#include <mutex>
#include <memory>
#include <algorithm>
#include <functional>

#include "metric/metric.h"

namespace metric {

enum class QualityOfService : int {
  Qos0 = 0, ///< Fire and forget. The message may not be delivered.
  Qos1 = 1, ///< At least once. The message will be delivered.
  Qos2 = 2, ///< Once and once only. The message will be delivered.
};

using OnTopicMessage = std::function<void()>;

class Topic {
 public:
  Topic() = default;
  virtual ~Topic() = default;

  virtual void Name(std::string name);
   [[nodiscard]] const std::string& Name() const;

   void Description(std::string description) {
     description_ = std::move(description);
   }
   [[nodiscard]] const std::string& Description() const { return description_; }

  void Namespace(std::string name_space) {
    name_space_ = std::move(name_space);
  }
  [[nodiscard]] const std::string& Namespace() const {
    return name_space_;
  }

  void GroupId(std::string group_id) {
    group_id_ = std::move(group_id);
  }
  [[nodiscard]] const std::string& GroupId() const {
    return group_id_;
  }

  void MessageType(std::string message_type) {
    message_type_ = std::move(message_type);
  }
  [[nodiscard]] const std::string& MessageType() const {
    return message_type_;
  }

  void NodeId(std::string node_id) {
    node_id_ = std::move(node_id);
  }
  [[nodiscard]] const std::string& NodeId() const {
    return node_id_;
  }

  void DeviceId(std::string device_id) {
    device_id_ = std::move(device_id);
  }
  [[nodiscard]] const std::string& DeviceId() const {
    return device_id_;
  }

  void ContentType(std::string mime_type) {
    content_type_ = std::move(mime_type);
  }
  [[nodiscard]] const std::string& ContentType() const {
    return content_type_;
  }

  void Body(std::vector<uint8_t> body) { body_ = std::move(body); }
  const std::vector<uint8_t>& Body() const { return body_; }
  std::vector<uint8_t>& Body() { return body_; }

  std::string BodyToString() const;
  void StringToBody(const std::string& text);

  void Timestamp(uint64_t ns_since_1970) { ns_since_1970_ = ns_since_1970;}
  [[nodiscard]] uint64_t Timestamp() const { return ns_since_1970_; }

  void Publishing(bool publish) {
    publish_ = publish;
  }
  [[nodiscard]] bool IsPublishing() const {
    return publish_;
  }

  void Qos(QualityOfService qos) {
    qos_ =  qos;
  }
  [[nodiscard]] QualityOfService Qos() const {
    return qos_;
  }

  void Retained(bool retained) {
    retained_ =  retained;
  }
  [[nodiscard]] bool IsRetained() const {
    return retained_;
  }

  [[nodiscard]] bool IsUpdated() const;
  void ResetUpdated() const;

  [[nodiscard]] bool IsWildcard() const;

  std::shared_ptr<Metric> CreateMetric(std::string metric_name);
  void AddMetric(const std::shared_ptr<Metric>& metric);
  void RemoveMetric(std::string name);
  const std::vector<std::shared_ptr<Metric>>& Metrics() const {
    return metric_list_;
  }
  std::vector<std::shared_ptr<Metric>>& Metrics() {
    return metric_list_;
  }

  std::shared_ptr<Metric> GetMetric(const std::string& name) const;

  void SetAllMetricsInvalid() const;

  [[nodiscard]] bool IsText() const {
    return content_type_.empty() || content_type_.find("text") != std::string::npos;
  }

  [[nodiscard]] bool IsJson() const {
    return content_type_.find("json") != std::string::npos;
  }

  [[nodiscard]] bool IsXml() const {
    return content_type_.find("xml") != std::string::npos;
  }
  [[nodiscard]] bool IsProtobuf() const {
    return content_type_.find("protobuf") != std::string::npos;
  }

  void SetOnTopicMessage(OnTopicMessage message_callback) {
    message_callback_ = std::move(message_callback);
  }

  virtual void OnMessage();
  virtual void OnPublish();

 protected:
  mutable std::recursive_mutex topic_mutex_;
  std::vector<std::shared_ptr<Metric>> metric_list_;
  OnTopicMessage message_callback_;
  std::string content_type_;    ///< MIME type of data (MQTT 5)

 private:
  mutable std::string name_;   ///< MQTT topic name.
       ///< If empty '<namespace>/<group_id>/<message_type>/<node_id>/<device_id>'
  std::string description_;
  std::string name_space_; ///< Topic namespace
  std::string group_id_;
  std::string message_type_;
  std::string node_id_;
  std::string device_id_;



  bool publish_ = false;
  QualityOfService qos_ = QualityOfService::Qos0;
  bool retained_ = false;

  std::vector<uint8_t> body_;
  uint64_t ns_since_1970_ = 0; ///< Note nanoseconds since 1970

  void AssignLevelName(size_t level, const std::string& name);
};

} // end namespace metric
