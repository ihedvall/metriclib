/*
* Copyright 2025 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */


#include <chrono>
#include <thread>
#include "mqtt/detectbroker.h"

using namespace std::chrono_literals;

namespace mqtt {
bool DetectBroker::Init() {
  InService();
  const auto init = MqttNode::Init();
  if (!init) {
    return false;
  }

  bool connected = false;
  for (size_t delay = 0; delay < 100; ++delay) {
    if (IsConnectionLost()) {
      connected = false;
      break;
    }
    if (IsConnected()) {
      connected = true;
      break;
    }
    std::this_thread::sleep_for(100ms);
  }
  if (connected) {
    Exit();
  }
  return connected;
}

} // mqtt