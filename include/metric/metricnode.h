/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

#pragma once

#include <cstdint>
#include <string>
#include <algorithm>
#include <atomic>
#include <memory>
#include <vector>
#include <mutex>

#include "metric/topic.h"
#include "metric/metriclisten.h"

namespace metric {

enum class TransportLayer: int {
  Unknown = 0,
  MqttTcp,
  MqttWebSocket,
  MqttTcpTls,
  MqttWebSocketTls,
};

enum class ProtocolVersion : int {
  Mqtt31 = 3,
  Mqtt311 = 4,
  Mqtt5 = 5
};

struct Subscription {
  std::string subscription;
  QualityOfService quality_of_service;
};

class MetricNode {
 public:
  MetricNode() = default;
  virtual ~MetricNode();

  void Name(std::string name) { name_ = std::move(name); }
  [[nodiscard]] const std::string& Name() const { return name_; }

  void Description(std::string desc) { description_ = std::move(desc); }
  [[nodiscard]] const std::string& Description() const { return description_; }

  void Transport(TransportLayer transport) {
    transport_layer_ = transport;
  }

  [[nodiscard]] TransportLayer Transport() const {
    return transport_layer_;
  }

  void Host(std::string address) {host_ = std::move(address); }
  [[nodiscard]] const std::string& Host() const { return host_; }

  void Port(uint16_t port) { port_ = port;}
  [[nodiscard]] uint16_t Port() const { return port_; }

  void ClientId(std::string identity) { client_id_ = std::move(identity); }
  [[nodiscard]] const std::string& ClientId() const { return client_id_; }

  void UserName(std::string name) {user_name_ = std::move(name); }
  [[nodiscard]] const std::string& UserName() const { return user_name_; }

  void Password(std::string password) {password_ = std::move(password); }
  [[nodiscard]] const std::string& Password() const { return password_; }

  void Version(ProtocolVersion version) {version_ = version; }
  [[nodiscard]] ProtocolVersion Version() const { return version_;}

  virtual bool Init();
  virtual bool Exit();

  virtual void InService();
  virtual void OutOfService();
  virtual bool IsInService() const;
  virtual bool IsConnected() const;
  virtual bool IsOperable() const;

  const std::vector<std::shared_ptr<Topic>>& Topics() const {
    return topic_list_;
  }
  virtual std::shared_ptr<Topic> CreateTopic(std::string name);
  std::shared_ptr<Topic> GetTopic(const std::string& name) const;
  std::shared_ptr<Topic> GetTopicByMessageType(const std::string &message_type);
  void DeleteTopic(std::string name);

  void AddSubscription(std::string subscription,
                       QualityOfService quality_of_service);
  void DeleteSubscription(std::string subscription);
  const std::vector<Subscription>& Subscriptions() const {
    return subscription_list_;
  }

  [[nodiscard]] MetricListen& Listen() const {
    return listen_;
  }

  static uint64_t NowMs();
  static uint64_t NowNs();
 protected:

  mutable std::atomic<bool> initialized_ = false;
  mutable std::atomic<bool> connected_ = false;
  mutable std::atomic<bool> in_service_ = false;
  mutable std::atomic<bool> operable_= false;

  mutable std::mutex topic_mutex_;
  std::vector<std::shared_ptr<Topic>> topic_list_;
  std::vector<Subscription> subscription_list_;

  mutable MetricListen listen_;
  void ResetConnectionLost() { connection_lost_ = false; }
  void SetConnectionLost() { connection_lost_ = true; }
  [[nodiscard]] bool IsConnectionLost() const { return connection_lost_; }
 private:
  std::string name_;
  std::string description_;
  TransportLayer transport_layer_ = TransportLayer::Unknown;
  std::string host_ = "127.0.0.1";
  uint16_t port_ = 1883;
  ProtocolVersion version_ = ProtocolVersion::Mqtt5;

  std::atomic<bool> connection_lost_ = false;

  std::string client_id_;
  std::string user_name_;
  std::string password_;
  // ToDo: Configuration for OpenSSL/TLS

};

}  // namespace metric

