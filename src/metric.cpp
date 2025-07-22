/*
 * Copyright 2025 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#include "metric/metric.h"

#include <utility>
#include "metrichelper.h"

namespace metric {


Metric::Metric(std::string  name)
  : name_(std::move(name)) {

}

Metric::Metric(const std::string_view& name)
    : name_(name.data()) {
}

void Metric::Name(std::string name) {
  std::scoped_lock lock(metric_mutex_);
  name_ = std::move(name);
}

std::string Metric::Name() const {
  std::scoped_lock lock(metric_mutex_);
  return name_;
}

void Metric::Description(std::string name) {
  SetStringProperty("description", std::move(name));
}

std::string Metric::Description() const {
  return GetStringProperty("description");
}

void Metric::Unit(const std::string &name) {
  SetStringProperty("unit", name);
}

std::string Metric::Unit() const {
  return GetStringProperty("unit");
}


/** @brief In MQTT the value are sent as string value. Sometimes the value is appended with
 * a unit string.
 *
 * The MQTT payload normally uses string values to send values. Sometimes a unit string is appended
 * to the string. So if the value is a decimal value, search for an optional unit string.
 * @param value String value with optional unit
 */
template<>
void Metric::Value(std::string value) {
  bool updated = false;
  {
    std::scoped_lock lock(metric_mutex_);
    if (updated = value != value_; updated ) {
      value_ = std::move(value);
    }
  }
  IsValid(true);
  if (updated) {
    SetUpdated();
  }
}

template<>
void Metric::Value(std::string_view value) {
  bool updated = false;
  {
    std::scoped_lock lock(metric_mutex_);
    if (updated = value_ != value; updated ) {
      value_ = value;
    }
  }
  IsValid(true);
  if (updated) {
    SetUpdated();
  }
}

template<>
void Metric::Value(const char* value) {
  bool updated = false;
  {
    std::scoped_lock lock(metric_mutex_);
    if (value == nullptr && !value_.empty()) {
      updated = true;
      value_.clear();
    } else if (value != nullptr && value_ != value) {
      updated = true;
      value_ = value;
    }
  }
  IsValid(true);
  if (updated) {
    SetUpdated();
  }
}

template<>
void Metric::Value(bool value) {
  bool updated = false;
  {
    std::scoped_lock lock(metric_mutex_);
    const std::string old_value = value_;
    value_ = value ? "1" : "0";
    updated = old_value != value_;
  }
  IsValid(true);
  if (updated) {
    SetUpdated();
  }
}

template<>
void Metric::Value(float value) {
  bool updated = false;
  {
    std::scoped_lock lock(metric_mutex_);
    std::string new_value = FloatToString(value);
    const auto pos = new_value.find(',');
    if (pos != std::string::npos) {
      new_value.replace(pos, 1, ".");
    }
    if (new_value != value_) {
      updated = true;
      value_ = std::move(new_value);
    }
  }
  IsValid(true);
  if (updated) {
    SetUpdated();
  }
}

template<>
void Metric::Value(double value) {
  bool updated = false;
  {
    std::scoped_lock lock(metric_mutex_);
    std::string new_value = DoubleToString(value);
    const auto pos = new_value.find(',');
    if (pos != std::string::npos) {
      new_value.replace(pos, 1, ".");
    }
    if (new_value != value_) {
      updated = true;
      value_ = std::move(new_value);
    }
  }
  IsValid(true);
  if (updated) {
    SetUpdated();
  }
}

template<>
std::string Metric::Value() const {
  std::scoped_lock lock(metric_mutex_);
  return value_;
}

template<>
int8_t Metric::Value() const {
  int8_t temp = 0;
  std::scoped_lock lock(metric_mutex_);
  try {
    temp = static_cast< int8_t>(std::stoi(value_));
  } catch (const std::exception&) {

  }
  return temp;
}

template<>
uint8_t Metric::Value() const {
  uint8_t temp = 0;
  std::scoped_lock lock(metric_mutex_);
  try {
    temp = static_cast< uint8_t>(std::stoul(value_));
  } catch (const std::exception&) {

  }
  return temp;
}

template<>
bool Metric::Value() const {
  std::scoped_lock lock(metric_mutex_);
  if (value_.empty()) {
    return false;
  }
  switch (value_[0]) {
    case 'Y':
    case 'y':
    case 'T':
    case 't':
    case '1':
      return true;

    default:
      break;
  }
  return false;
}

void Metric::AddProperty(const MetricProperty& property) {
  std::scoped_lock lock(metric_mutex_);
  const std::string& key = property.Key();
  if (auto exist = property_list_.find(key);
      exist == property_list_.end()) {
    property_list_.emplace(key, property);
  } else {
    exist->second = property;
  }
}

MetricProperty* Metric::CreateProperty(const std::string& key) {
  std::scoped_lock lock(metric_mutex_);
  if (const auto exist = property_list_.find(key);
      exist == property_list_.cend()) {
    MetricProperty temp;
    temp.Key(key);
    property_list_.emplace(key, temp);
  }
  return GetProperty(key);
}

MetricProperty *Metric::GetProperty(const std::string &key) {
  std::scoped_lock lock(metric_mutex_);
  if (auto exist = property_list_.find(key);
      exist != property_list_.end()) {
    return &exist->second;
  }
  return nullptr;
}

const MetricProperty *Metric::GetProperty(const std::string &key) const {
  std::scoped_lock lock(metric_mutex_);
  if (const auto exist = property_list_.find(key);
      exist != property_list_.cend()) {
    return &exist->second;
  }
  return nullptr;
}

void Metric::DeleteProperty(const std::string &key) {
  std::scoped_lock lock(metric_mutex_);
  auto itr = property_list_.find(key);
  if (itr != property_list_.end()) {
    property_list_.erase(itr);
  }
}

void Metric::SetStringProperty(std::string key, std::string value) {
  MetricProperty prop(std::move(key), std::move(value));
  std::scoped_lock lock(metric_mutex_);
  AddProperty(prop);
}

std::string Metric::GetStringProperty(const std::string &key) const {
  std::scoped_lock lock(metric_mutex_);
  if (const auto exist = property_list_.find(key);
      exist != property_list_.cend() ) {
    const auto& prop = exist->second;
    try {
      return prop.Value<std::string>();
    } catch (const std::exception&) {
      return {};
    }
  }
  return {};
}

} // end namespace