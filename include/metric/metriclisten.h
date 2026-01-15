/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
 */
#pragma once

#include <string>
#include <functional>
#include <algorithm>
#include <sstream>

namespace metric {

using ListenFunction = std::function<void(const std::string& pre_text,
  uint64_t timestamp, const std::string& text)>;

class MetricListen {
 public:
  virtual ~MetricListen() = default;

  void PreText(std::string pre_text) { pre_text_ = std::move(pre_text); }
  [[nodiscard]] const std::string& PreText() const { return pre_text_; }

  void LogLevel(size_t log_level) {log_level_ = log_level; }
  [[nodiscard]] size_t LogLevel() const {return log_level_;}

  void SetListenFunction(const ListenFunction& func) {
    listen_function_ = func;
  }
  void ResetListenFunction() {
    listen_function_ = nullptr;
  }

  [[nodiscard]] virtual bool IsActive() const;



  void ListenString(std::string text) {
    OnAddMessage(0, std::move(text));
  }

  template <typename... Args>
  void ListenArgs(Args... args ) {
    std::ostringstream temp;
    (temp << ... << args);
    OnAddMessage(0, temp.str());
  }

  static void LogToConsole( const std::string& pre_text,
    uint64_t nano_sec_1970, const std::string& text);
 protected:
  ListenFunction listen_function_;
  size_t log_level_ = 0;
  std::string pre_text_ = "METRIC";

  virtual void OnAddMessage(uint64_t nano_sec_1970, std::string text);
};

}  // namespace metric


