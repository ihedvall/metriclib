/*
 * Copyright 2025 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#include <iostream>

#include <nlohmann/json.hpp>

#include "metric/metriclogstream.h"
#include "mqtt/mqtttopic.h"
#include "mqtt/mqttnode.h"

using namespace metric;
using namespace nlohmann;

namespace mqtt {
MqttTopic::MqttTopic(MqttNode &parent)
: parent_(parent) {
}

void MqttTopic::OnPublish() {
  if (!IsPublishing()) {
    return;
  }

  // Fill the body with MQTT
  if (IsJson()) {
    MakeJsonBody();
  } else {
    MakeTextBody();
  }

 // lrv_ = PayloadBody<std::vector<uint8_t>>();
  auto& listen = parent_.Listen();
  if (listen.IsActive() && listen.LogLevel() != 2) {
    const auto text = BodyToString();
    std::ostringstream temp;
    temp << "Publish " << Name() <<": " <<  text;
    listen.ListenString(temp.str());
  }

  auto& body = Body();
  MQTTAsync_message  message = MQTTAsync_message_initializer;
  message.payload = body.data();
  message.payloadlen = static_cast<int>(body.size());
  message.qos = static_cast<int>(Qos());
  message.retained = IsRetained() ? 1 : 0;
  message.properties = MQTTProperties_initializer;

  MQTTProperty format_indicator;
  format_indicator.identifier = MQTTPROPERTY_CODE_PAYLOAD_FORMAT_INDICATOR;
  format_indicator.value.byte = 0x01;
  MQTTProperties_add(&message.properties, &format_indicator);


  if (!content_type_.empty()) {
    MQTTProperty content_type;
    content_type.identifier = MQTTPROPERTY_CODE_CONTENT_TYPE;
    content_type.value.data.len = static_cast<int>(content_type_.size());
    content_type.value.data.data = content_type_.data();
    MQTTProperties_add(&message.properties, &content_type);
  }


  MQTTAsync_responseOptions options = MQTTAsync_responseOptions_initializer;
  if (parent_.Version() == ProtocolVersion::Mqtt5) {
    options.onFailure5 = OnSendFailure5;
  } else {
    options.onFailure = OnSendFailure;
  }
  options.context = this;

  const auto send = MQTTAsync_sendMessage(parent_.Handle(), Name().c_str(), &message, &options );
  if (send != MQTTASYNC_SUCCESS) {
    SetAllMetricsInvalid();
    std::ostringstream err;
    err << "Failed to publish to the MQTT broker.";
    const auto* cause = MQTTAsync_strerror(send);
    if (cause != nullptr && strlen(cause) > 0) {
      err << "Error: " << cause;
    }
    METRIC_ERROR() << err.str();
  }

}

void MqttTopic::OnMessage() {
  if (IsText()) {
    HandleTextBody();
  } else if (IsJson()) {
    HandleJsonBody();
  }
  Topic::OnMessage(); // Calls any callback function
}

void MqttTopic::MakeJsonBody() {
  std::ostringstream json;
  json << "{" << std::endl;
  for (const auto& metric : metric_list_) {
    if (!metric || metric->Name().empty()) {
      continue;
    }
    json << "\"" << metric->Name() << "\": ";
    if (metric->IsNull() || !metric->IsValid()) {
      json << "null";
    } else {
      switch (metric->DataType()) {
        case MetricType::Int8:
        case MetricType::Int16:
        case MetricType::Int32:
        case MetricType::Int64:
          json << metric->Value<int64_t>();
          break;

        case MetricType::UInt8:
        case MetricType::UInt16:
        case MetricType::UInt32:
        case MetricType::UInt64:
          json << metric->Value<uint64_t>();
          break;

        case MetricType::Float:
        case MetricType::Double:
          json << metric->Value<std::string>(); // Use internal format
          break;

        case MetricType::Boolean:
          json << (metric->Value<bool>() ? "true" : "false");
          break;

        default:
          json << "\"" << metric->Value<std::string>() << "\"";
          break;
      }
    }
  }
  json << "}";

  StringToBody(json.str());
}

void MqttTopic::MakeTextBody() {
  std::ostringstream text;
  for (const auto& metric : metric_list_) {
    if (!metric) {
      continue;
    }
    if (metric->IsNull() || !metric->IsValid()) {
      text << "*";
    } else {
      switch (metric->DataType()) {
        case MetricType::Int8:
        case MetricType::Int16:
        case MetricType::Int32:
        case MetricType::Int64:
          text << metric->Value<int64_t>();
          break;

        case MetricType::UInt8:
        case MetricType::UInt16:
        case MetricType::UInt32:
        case MetricType::UInt64:
          text << metric->Value<uint64_t>();
          break;

        case MetricType::Float:
        case MetricType::Double:
          text << metric->Value<std::string>(); // Use internal format
          break;

        case MetricType::Boolean:
          text << (metric->Value<bool>() ? "true" : "false");
          break;

        default:
          text << "\"" << metric->Value<std::string>() << "\"";
          break;
      }
    }
  }


  StringToBody(text.str());
}

/** \brief Parse the topic value as plain text string.
 *
 * When parsing the topic value as a plain text string,
 * the topic can only have one metric.
 * Special handling if the topic doesn't have any metric.
 * In that case create a metric with the same name as the topic.
 * Only one metric can be added to the topic.
 */
void MqttTopic::HandleTextBody() {

  const std::string value = BodyToString(); // Assume text body
  auto metric = GetMetric(Name());

  // If the end-user added this metric, then assume that the body relates to it.
  if (!metric && !metric_list_.empty()) {
    metric = metric_list_.front();
  }

  // If no metric exists, create a default one.
  if (!metric) {
    metric = CreateMetric(Name());
    metric->DataType(MetricType::String);
  }
  metric->Timestamp(Timestamp());
  metric->Value(value);
  //metric->FireOnMessage();
}

/** \brief Parse the body string as JSON key-value metrics
 *
 * If the topic content type is set to application/json,
 * the string should be treated a JSON list of metric name and value list.
 * If a metric isn't found, then it have to be created.
 * Little bit tricky with the data type.
 */
void MqttTopic::HandleJsonBody() {

  const std::string json_string = BodyToString();
  try {
    json json_list = json::parse(json_string);
    for (const auto& [key, value] : json_list.items()) {
      std::string metric_name= key;
      auto metric = GetMetric(metric_name);
      if (!metric) {
        metric = CreateMetric(metric_name);
      }
      if (!metric) {
        continue;
      }
      if (metric->DataType() == MetricType::Unknown) {
        switch (value.type()) {
          case detail::value_t::string:
            metric->DataType(MetricType::String);
            break;

          case detail::value_t::boolean:
            metric->DataType(MetricType::Boolean);
            break;

          case detail::value_t::number_integer:
            metric->DataType(MetricType::Int64);
            break;

          case detail::value_t::number_unsigned:
            metric->DataType(MetricType::UInt64);
            break;

          case detail::value_t::number_float:
            metric->DataType(MetricType::Double);
            break;

          default:
            // Cannot set the data type
            break;
        }
      }

      if (value.type() == detail::value_t::null) {
        metric->Null(true);
        metric->Valid(false);
      } else {
        metric->Null(false);
        metric->Valid(true);
        metric->Value(value.get<std::string>());
      }
      metric->Timestamp(Timestamp());
      //metric->FireOnMessage();
    } // end for loop

  } catch (const std::exception& err) {
    METRIC_ERROR() << "JSON parser error. JSON: " << json_string
    << ", Error: " << err.what();
  }


}

void MqttTopic::OnSendFailure(void *context, MQTTAsync_failureData *response) {
  auto *topic = reinterpret_cast<MqttTopic *>(context);
  if (topic == nullptr ) {
    return;
  }
  topic->SetAllMetricsInvalid();

  auto& listen = topic->parent_.Listen();
  if (listen.IsActive() && response != nullptr) {
    std::ostringstream text;
    text << "Publish Failure. Topic: " << topic->Name()
      << ", Error: " << MQTTAsync_strerror(response->code);
    listen.ListenString(text.str());
  }
}

void MqttTopic::OnSendFailure5(void *context, MQTTAsync_failureData5 *response)
{
  auto *topic = reinterpret_cast<MqttTopic *>(context);
  if (topic == nullptr ) {
    return;
  }
  topic->SetAllMetricsInvalid();
  auto& listen = topic->parent_.Listen();
  if (listen.IsActive() && response != nullptr) {
    std::ostringstream text;
    text << "Publish Failure. Topic: " << topic->Name()
      << ", Error: " << MQTTAsync_strerror(response->code);
      listen.ListenString(text.str());
  }
}

} // End namespace mqtt