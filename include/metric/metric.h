/*
 * Copyright 2025 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <cstdint>
#include <string>
#include <atomic>
#include <vector>
#include <map>
#include <mutex>

#include "metric/metrictype.h"
#include "metric/metricproperty.h"

namespace metric {

/**
 * @class Metric
 * @brief The Metric class represents a generic value with various properties.
 *
 */
 class Metric;

class Metric {
 public:
  Metric() = default;
  explicit Metric(std::string name);
  explicit Metric(const std::string_view& name);

  void Name(std::string name);
  [[nodiscard]] std::string Name() const;

  void Alias(uint64_t alias) {
    alias_ = alias;
  }
  [[nodiscard]] uint64_t Alias() const {
    return alias_;
  }

  void Timestamp(uint64_t ms_since_1970) {
    timestamp_ = ms_since_1970;
  }

  [[nodiscard]] uint64_t Timestamp() const {
    return timestamp_;
  }

  void Description(std::string desc);
  [[nodiscard]] std::string Description() const;

  void Unit(const std::string& unit);
  [[nodiscard]] std::string Unit() const;

  void Type(MetricType type) {
    datatype_ = type;
  }

  [[nodiscard]] MetricType Type() const {
    return datatype_;
  }

  void IsHistorical(bool historical_value) {
    is_historical_ = historical_value;
  }
  [[nodiscard]] bool IsHistorical() const {
    return is_historical_;
  }

  void IsTransient(bool transient_value) {
    is_transient_ = transient_value;
  }
  [[nodiscard]] bool IsTransient() const {
    return is_transient_;
  }

  void IsNull(bool null_value) {
    is_null_ = null_value;
  }
  [[nodiscard]] bool IsNull() const {
    return is_null_;
  }

  void IsValid(bool valid) const {
    valid_ = valid;
  }
  [[nodiscard]] bool IsValid() const {
    return valid_;
  }

  void IsReadWrite(bool read_only) {
    read_only_ = read_only;
  }
  [[nodiscard]] bool IsReadOnly() const {
    return read_only_;
  }

  void AddProperty(const MetricProperty& property);
  [[nodiscard]] MetricProperty* CreateProperty(const std::string& key);
  [[nodiscard]] MetricProperty* GetProperty(const std::string& key);
  [[nodiscard]] const MetricProperty* GetProperty(const std::string& key) const;
  void DeleteProperty(const std::string& key);

  const MetricPropertyList& Properties() const {
    return property_list_;
  }

  template <typename T>
  void Value(T value);

  template<typename T>
  [[nodiscard]] T Value() const;


  void SetUpdated() { updated_ = true; }
  void ResetUpdated() { updated_ = false; }
  [[nodiscard]] bool IsUpdated() {
    return updated_;
  }

 private:
  std::string name_;
  std::atomic<uint64_t> alias_ = 0;
  std::atomic<uint64_t> timestamp_ = 0;
  std::atomic<MetricType> datatype_ = MetricType::String;
  std::atomic<bool> is_historical_ = false;
  std::atomic<bool> is_transient_ = false;
  std::atomic<bool> is_null_ = false;

  mutable std::atomic<bool> valid_ = false; ///< Indicate if the metric is GOOD or STALE
  std::atomic<bool> read_only_ = false; ///< Indicate if the metric can be changes remotely

  MetricPropertyList property_list_;

  mutable std::recursive_mutex metric_mutex_;
  std::string value_;

  std::atomic<bool> updated_ = false;

  std::string GetStringProperty(const std::string& key) const;
  void SetStringProperty(std::string key, std::string value);
};

template<typename T>
void Metric::Value(T value) {

  try {
    bool updated = false;
    {
      std::scoped_lock lock(metric_mutex_);
      std::string new_value = std::to_string(value);
      if (new_value != value_) {
        updated = true;
        value_ = std::move(new_value);
      }
    }
    IsValid(true);
    if (updated) {
      SetUpdated();
    }
  } catch (const std::exception& ) {
    IsValid(false);
  }

}

template<>
void Metric::Value(std::string value);

template<>
void Metric::Value(std::string_view& value);

template<>
void Metric::Value(const char* value);

template<>
void Metric::Value(bool value);

template<>
void Metric::Value(float value);

template<>
void Metric::Value(double value);

template<typename T>
T Metric::Value() const {
  T temp = {};

  try {
    std::lock_guard lock(metric_mutex_);
    std::istringstream str(value_);
    str >> temp;
  } catch (const std::exception& ) {
    IsValid(false);
  }
  return temp;
}


template<>
std::string Metric::Value() const;

template<>
int8_t Metric::Value() const;

template<>
uint8_t Metric::Value() const;

template<>
bool Metric::Value() const;

} // end namespace