/*
 * Copyright 2025 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <MQTTAsync.h>

#include "metric/topic.h"


namespace mqtt {

class MqttNode;
class MqttTopic : public metric::Topic {
 public:
  explicit MqttTopic(MqttNode& parent);
  MqttTopic() = delete;

  void OnPublish() override;
  void OnMessage() override;

 private:
  MqttNode& parent_;

  void MakeJsonBody();
  void MakeTextBody();
  void HandleTextBody();
  void HandleJsonBody();
  static void OnSendFailure(void *context, MQTTAsync_failureData *response);
  static void OnSendFailure5(void *context, MQTTAsync_failureData5 *response);

};

}