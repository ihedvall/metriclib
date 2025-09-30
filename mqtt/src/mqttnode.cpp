/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/
#include "mqtt/mqttnode.h"

#include <chrono>
#include <functional>

#include "metric/metriclogstream.h"
#include "mqtt/mqtttopic.h"

using namespace std::chrono_literals;
using namespace metric;

namespace {

  std::string_view StateToString(mqtt::MqttNode::ClientState state) {

    switch (state) {
      case mqtt::MqttNode::ClientState::Idle: return "Idle";
      case mqtt::MqttNode::ClientState::WaitOnConnect: return "Wait on Connect";
      case mqtt::MqttNode::ClientState::Online: return "Online";
      case mqtt::MqttNode::ClientState::WaitOnDisconnect: return "Wait on Disconnect";
      default: break;
    }
    return "Unknown State";
  }
}


namespace mqtt {
MqttNode::MqttNode() {
  ResetConnectionLost();
  MqttNode::InService();
}

MqttNode::~MqttNode() {
  MqttNode::OutOfService();
  if (listen_.IsActive()) {
   listen_.ListenString("Stopping client");
  }
  MqttNode::Exit();
  for (auto& topic : topic_list_ ) {
   if (!topic) {
     continue;
   }
   topic->SetAllMetricsInvalid();
  }
}

std::shared_ptr<Topic> MqttNode::CreateTopic(std::string name) {
  std::scoped_lock list_lock(topic_mutex_);
  auto exist = std::ranges::find_if(topic_list_, [&name] (auto& topic) -> bool {
    return topic && topic->Name() == name;
  });
  if (exist != topic_list_.end()) {
    return *exist;
  }
  auto new_topic = std::make_shared<MqttTopic>(*this);
  new_topic->Name(std::move(name));
  topic_list_.push_back(std::move(new_topic));
  return topic_list_.back();
}

/*
ITopic *MqttClient::AddMetric(const std::shared_ptr<Metric>& metric) {
 if (!metric || metric->Name().empty()) {
   LOG_ERROR() << "Cannot add a metric with no name.";
   return nullptr;
 }

 auto* topic = GetTopic(metric->Name()); // Note that this call adds the topic to its list.
 if ( topic == nullptr) {
   topic = CreateTopic();
   if (topic == nullptr) {
     LOG_ERROR() << "Failed to create a topic. Topic: " <<  metric->Name();
     return nullptr;
   }
   topic->Topic(metric->Name());

   topic->Publish(true);
 }


 // Set default value
 auto& payload = topic->GetPayload();
 const auto text = metric->GetMqttString();
 if (metric->IsNull()) {
   payload.StringToBody("");
 } else {
   payload.StringToBody(text);
 }

 payload.AddMetric(metric);

 return topic;
}
*/
bool MqttNode::IsConnected() const {
 return handle_ != nullptr && MQTTAsync_isConnected(handle_);
}

bool MqttNode::Init() {
  InitMqtt();
  // Create the worker task
  stop_client_task_ = true;
  if (work_thread_.joinable()) {
   work_thread_.join();
  }
  if (listen_.IsActive()) {
    listen_.ListenString("Starting node thread");
  }
  ResetConnectionLost();
  client_timer_ = 0;
  stop_client_task_ = false;
  work_thread_ = std::thread(&MqttNode::ClientTask, this);
  if (listen_.IsActive()) {
    listen_.ListenString("Started node thread");
  }
  client_event_.notify_one();
  return true;
}

bool MqttNode::CreateClient() {
  std::ostringstream connect_string;
  switch (Transport()) {
   case TransportLayer::MqttWebSocket:
     connect_string << "ws://";
     break;

   case TransportLayer::MqttTcpTls:
     connect_string << "ssl://";
     break;

   case TransportLayer::MqttWebSocketTls:
     connect_string << "wss://";
     break;

   default:
     connect_string << "tcp://";
     break;
  }
 connect_string << Host() << ":" << Port();
 if (listen_.IsActive()) {
   listen_.ListenString("Creating client");
 }

 MQTTAsync_createOptions create_options = MQTTAsync_createOptions_initializer5;

 const auto create = MQTTAsync_createWithOptions(&handle_, connect_string.str().c_str(),
                       Name().c_str(),
                       MQTTCLIENT_PERSISTENCE_NONE, nullptr,
                       Version() == ProtocolVersion::Mqtt5 ? &create_options : nullptr);
if (create != MQTTASYNC_SUCCESS) {
   std::ostringstream err;
   err << "Didn't create the MQTT handle.";
   const auto* cause = MQTTAsync_strerror(create);
   if (cause != nullptr && strlen(cause) > 0) {
     err << "Error: " << cause;
   }

   METRIC_ERROR() << err.str();
   return false;
 }

 const auto callback = MQTTAsync_setCallbacks(handle_, this,
                                              OnConnectionLost,
                                              OnMessageArrived,
                                              OnDeliveryComplete);
 if (callback != MQTTASYNC_SUCCESS) {
   std::ostringstream err;
   err << "Didn't to set the MQTT callbacks.";
   const auto* cause = MQTTAsync_strerror(callback);
   if (cause != nullptr && strlen(cause) > 0) {
     err << "Error: " << cause;
   }

   METRIC_ERROR() << err.str();
   return false;
 }
 if (listen_.IsActive()) {
   listen_.ListenString("Created client");
 }
 return true;
}

bool MqttNode::SendConnect() {

 // Reset the connection lost to detect any failing startup
 ResetConnectionLost();
 ResetDelivered();

 MQTTAsync_connectOptions connect_options = MQTTAsync_connectOptions_initializer;
 if (Version() == ProtocolVersion::Mqtt5) {
   connect_options = MQTTAsync_connectOptions_initializer5;
 }
 connect_options.keepAliveInterval = 10; // 10 seconds between keep alive messages
 // connect_options.cleansession = MQTTASYNC_TRUE;
 connect_options.connectTimeout = 5; // Wait max 5 seconds on connect.
 connect_options.onSuccess = OnConnect;
 connect_options.onFailure = OnConnectFailure;
 connect_options.context = this;
 if (Version() == ProtocolVersion::Mqtt5) {
   connect_options.MQTTVersion = MQTTVERSION_5;
   connect_options.onSuccess = nullptr;
   connect_options.onFailure = nullptr;
   connect_options.onSuccess5 = OnConnect5;
   connect_options.onFailure5 = OnConnectFailure5;
 }
 if (!UserName().empty() && !Password().empty()) {
   connect_options.username = UserName().c_str();
   connect_options.password = Password().c_str();
 }

 if (Transport() == TransportLayer::MqttTcpTls ||
     Transport() == TransportLayer::MqttWebSocketTls) {
   InitSsl(); // Fill the ssl_options_ structure with values
   connect_options.ssl = &ssl_options_;
 }

 const auto connect = MQTTAsync_connect(handle_, &connect_options);
 if (connect != MQTTASYNC_SUCCESS) {
   std::ostringstream err;
   err << "Didn't connect to the MQTT broker.";
   const auto* cause = MQTTAsync_strerror(connect);
   if (cause != nullptr && strlen(cause) > 0) {
     err << "Error: " << cause;
   }

   METRIC_ERROR() << err.str();
   return false;
 }
 return true;
}

bool MqttNode::SendDisconnect() {

 ResetConnectionLost();
 ResetDelivered();
 MQTTAsync_disconnectOptions disconnect_options = MQTTAsync_disconnectOptions_initializer;
 if (Version() == ProtocolVersion::Mqtt5) {
   disconnect_options = MQTTAsync_disconnectOptions_initializer5;
   disconnect_options.onSuccess = nullptr;
   disconnect_options.onFailure = nullptr;
   disconnect_options.onSuccess5 = OnDisconnect5;
   disconnect_options.onFailure5 = OnDisconnectFailure5;
 } else {
   disconnect_options.onSuccess = OnDisconnect;
   disconnect_options.onFailure = OnDisconnectFailure;
 }
 disconnect_options.context = this;
 disconnect_options.timeout = 5000;

 const auto disconnect = MQTTAsync_disconnect(handle_, &disconnect_options);
 if (disconnect != MQTTASYNC_SUCCESS) {
   std::ostringstream err;
   err << "Didn't disconnect from the MQTT broker.";
   const auto* cause = MQTTAsync_strerror(disconnect);
   if (cause != nullptr && strlen(cause) > 0) {
     err << "Error: " << cause;
   }

   if (listen_.IsActive()) {
     listen_.ListenString(err.str());
   }
 }
 return disconnect == MQTTASYNC_SUCCESS;
}

bool MqttNode::Exit() {
 stop_client_task_ = true;
 client_event_.notify_one();
 if (work_thread_.joinable()) {
   work_thread_.join();
 }
 if (handle_ != nullptr) {
   MQTTAsync_destroy(&handle_);
   handle_ = nullptr;
 }
 return true;
}

void MqttNode::ConnectionLost(const std::string& cause) {
 std::ostringstream err;
 err << "Connection lost.";
 if (!cause.empty()) {
   err << " Error: " << cause;
 }

 if (listen_.IsActive()) {
   listen_.ListenString( err.str() );
 }
 SetConnectionLost();
}

void MqttNode::Message(const std::string& topic_name, const MQTTAsync_message& message) {
  if (topic_name.empty()) {
    return;
  }

  auto topic = CreateTopic(topic_name);
  if (!topic) {
    METRIC_ERROR() << "Didn't create topic. Topic: " << topic_name;
    return;
  }

  // Save last topic body
  auto& body = topic->Body();
  try {
    body.resize(message.payloadlen, 0);
    if (message.payload != nullptr && message.payloadlen > 0) {
      std::memcpy(body.data(), message.payload, message.payloadlen);
    }
  } catch(const std::exception& err) {
    METRIC_ERROR() << "Didn't parse payload. Topic: " << topic_name << ", Error: " << err.what();
    return;
  }

 // Save the timestamp, Qos and retained flag
  topic->Timestamp(NowNs());
  topic->Qos(static_cast<QualityOfService>(message.qos));
   topic->Retained(message.retained == 1);
   if (MQTTProperties_hasProperty(&message.properties,
    MQTTPROPERTY_CODE_CONTENT_TYPE) ) {
    if (const MQTTProperty* content_type =
      MQTTProperties_getProperty(&message.properties,
        MQTTPROPERTY_CODE_CONTENT_TYPE);
        content_type != nullptr && content_type->value.data.len > 0) {
        std::string temp(content_type->value.data.data,
          content_type->value.data.len);
        topic->ContentType(temp);
    }
  }


  if (listen_.IsActive() && listen_.LogLevel() != 1) {
    std::ostringstream msg;
    msg << "Message: " << topic_name << ", Value: " << topic->BodyToString();
    listen_.ListenString(msg.str());
  }
  topic->OnMessage();
  ResetConnectionLost();
}

void MqttNode::DeliveryComplete(MQTTAsync_token ) {
 ResetConnectionLost();
}

void MqttNode::Connect(const MQTTAsync_successData& response) {
 const auto& connect = response.alt.connect;
 const std::string server_url =  connect.serverURI != nullptr ? connect.serverURI : "";
 if (Name().empty()) {
   Name(server_url);
 }

 const int version = connect.MQTTVersion;
 switch (version) {
   case MQTTVERSION_3_1:
     Version(ProtocolVersion::Mqtt31);
     break;

   case MQTTVERSION_5:
     Version(ProtocolVersion::Mqtt5);
     break;

   case MQTTVERSION_3_1_1:
   case MQTTVERSION_DEFAULT:
   default:
     Version(ProtocolVersion::Mqtt311);
     break;

 }
 const int session_present = connect.sessionPresent;
 if (listen_.IsActive()) {
   std::ostringstream msg;
   msg << "Connected. Server: " << server_url << " Version: "
       << version << ", Session: " << session_present;
   listen_.ListenString(msg.str());
 }
 ResetConnectionLost();
 SetDelivered();
 client_event_.notify_one();
}


void MqttNode::ConnectFailure(const  MQTTAsync_failureData* response) {
 std::ostringstream err;
 err << "Connect failure.";
 if (response != nullptr) {
   const auto code = response->code;
   const auto* cause = MQTTAsync_strerror(code);
   if (cause != nullptr && strlen(cause) > 0) {
     err << " Error: " << cause;
   }

 }
 if (listen_.IsActive()) {
   listen_.ListenString(err.str());
 }
 SetConnectionLost();
 SetDelivered();
 client_event_.notify_one();
}

void MqttNode::Connect5(const MQTTAsync_successData5& response) {
 const auto& connect = response.alt.connect;
 const std::string server_url =  connect.serverURI != nullptr ? connect.serverURI : "";
 if (Name().empty()) {
   Name(server_url);
 }

 const int version = connect.MQTTVersion;
 switch (version) {
   case MQTTVERSION_3_1:
     Version(ProtocolVersion::Mqtt31);
     break;

   case MQTTVERSION_5:
     Version(ProtocolVersion::Mqtt5);
     break;

   case MQTTVERSION_3_1_1:
   case MQTTVERSION_DEFAULT:
   default:
     Version(ProtocolVersion::Mqtt311);
     break;

 }
 const int session_present = connect.sessionPresent;
 if (listen_.IsActive()) {
   std::ostringstream msg;
   msg << "Connected. Server: " << server_url << ", Version: "
       << version << ", Session: " << session_present;
   listen_.ListenString(msg.str());
 }
 ResetConnectionLost();
 SetDelivered();
 client_event_.notify_one();
}


void MqttNode::ConnectFailure5(const MQTTAsync_failureData5* response) {
 std::ostringstream err;
 err << "Connect failure.";
 if (response != nullptr) {
   const auto code = response->code;
   const auto* cause = MQTTAsync_strerror(code);
   if (cause != nullptr && strlen(cause) > 0) {
     err << " Error: " << cause;
   }

 }
 if (listen_.IsActive()) {
   listen_.ListenString(err.str());
 }
 SetConnectionLost();
 SetDelivered();
 client_event_.notify_one();
}

void MqttNode::SubscribeFailure(const MQTTAsync_failureData &response) {
 std::ostringstream err;
 err << "Subscribe Failure. Error: " << MQTTAsync_strerror(response.code);
 if (response.message != nullptr) {
   err << ". Message: " << response.message;
 }
 if (listen_.IsActive()) {
   listen_.ListenString(err.str());
 }
 METRIC_ERROR() << err.str();
}

void MqttNode::SubscribeFailure5(const MQTTAsync_failureData5 &response) {
 std::ostringstream err;
 err << "Subscribe Failure. Error: " << MQTTAsync_strerror(response.code);
 if (response.message != nullptr) {
   err << ". Message: " << response.message;
 }
 if (listen_.IsActive()) {
   listen_.ListenString(err.str() );
 }
 METRIC_ERROR() << err.str();
}

void MqttNode::Disconnect(const MQTTAsync_successData*) {
 SetDelivered();
 ResetConnectionLost();
 client_event_.notify_one();
}

void MqttNode::DisconnectFailure(const MQTTAsync_failureData* response) {
 std::ostringstream err;
 err << "Disconnect failure.";
 if (response != nullptr) {
   const int code = response->code;
   const auto* cause = MQTTAsync_strerror(code);
   if (cause != nullptr && strlen(cause) > 0) {
     err << " Error: " << cause;
   }
 }

 if (listen_.IsActive()) {
   listen_.ListenString(err.str());
 }
 SetConnectionLost();
 SetDelivered();
 client_event_.notify_one();
}

void MqttNode::Disconnect5(const MQTTAsync_successData5*) {
 SetDelivered();
 ResetConnectionLost();
 client_event_.notify_one(); // Speed up the disconnect
}

void MqttNode::DisconnectFailure5(const MQTTAsync_failureData5* response) {
 std::ostringstream err;
 err << "Disconnect failure.";
 if (response != nullptr) {
   const int code = response->code;
   const auto* cause = MQTTAsync_strerror(code);

   if (cause != nullptr && strlen(cause) > 0) {
     err << " Error: " << cause << ".";
   }
   if (response->message != nullptr) {
     err << " Message: " << response->message;
   }
 }

 if (listen_.IsActive()) {
   listen_.ListenString(err.str());
 }
 SetConnectionLost();
 SetDelivered();
 client_event_.notify_one();
}

void MqttNode::OnConnectionLost(void *context, char *cause) {
  if (auto *client = static_cast<MqttNode *>(context); client != nullptr) {
    std::string reason = cause != nullptr ? cause : "";
    client->ConnectionLost(reason);
  }
  if (cause != nullptr) {
    MQTTAsync_free(cause);
  }
}

int MqttNode::OnMessageArrived(void* context, char* topic_name, int topicLen,
                               MQTTAsync_message* message) {
 const std::string topic_id = topic_name != nullptr && topicLen > 0 ? topic_name : "";
 if (auto *client = static_cast<MqttNode *>(context);
     client != nullptr && message != nullptr && !topic_id.empty()) {
   client->Message(topic_id, *message);
 }
 if (topic_name != nullptr) {
   MQTTAsync_free(topic_name);
 }

 if (message != nullptr) {
   MQTTProperties_free(&message->properties);
   MQTTAsync_freeMessage(&message);
 }
 return MQTTASYNC_TRUE;
}

void MqttNode::OnDeliveryComplete(void *context, MQTTAsync_token token) {
 if (auto *client = static_cast<MqttNode *>(context); client != nullptr) {
   client->DeliveryComplete(token);
 }
}

void MqttNode::OnConnect(void* context, MQTTAsync_successData* response) {
 if (auto *client = static_cast<MqttNode *>(context);
     client != nullptr && response != nullptr) {
   client->Connect(*response);
 }
}

void MqttNode::OnConnectFailure(void* context, MQTTAsync_failureData* response) {
 if ( auto *client = static_cast<MqttNode *>(context); client != nullptr) {
   client->ConnectFailure(response);
 }
}

void MqttNode::OnConnect5(void* context, MQTTAsync_successData5* response) {
 if (auto *client = static_cast<MqttNode *>(context);
     client != nullptr && response != nullptr) {
   client->Connect5(*response);
 }
}

void MqttNode::OnConnectFailure5(void* context, MQTTAsync_failureData5* response) {
 if (auto *client = static_cast<MqttNode *>(context); client != nullptr) {
   client->ConnectFailure5(response);
 }
}

void MqttNode::OnSubscribeFailure(void *context, MQTTAsync_failureData *response) {
 if (auto *client = static_cast<MqttNode *>(context);
     client != nullptr && response != nullptr) {
   client->SubscribeFailure(*response);
 }
}

void MqttNode::OnSubscribeFailure5(void *context, MQTTAsync_failureData5 *response) {
 if (auto *client = static_cast<MqttNode *>(context);
      client != nullptr && response != nullptr) {
   client->SubscribeFailure5(*response);
 }
}

void MqttNode::OnDisconnect(void* context, MQTTAsync_successData* response) {
 if (auto *client = static_cast<MqttNode *>(context); client != nullptr) {
   client->Disconnect(response);
 }
}

void MqttNode::OnDisconnectFailure(void* context, MQTTAsync_failureData* response) {
 if (auto *client = static_cast<MqttNode *>(context); client != nullptr) {
   client->DisconnectFailure(response);
 }
}

void MqttNode::OnDisconnect5(void* context, MQTTAsync_successData5* response) {
 if (auto *client = static_cast<MqttNode *>(context); client != nullptr) {
   client->Disconnect5(response);
 }
}

void MqttNode::OnDisconnectFailure5(void* context, MQTTAsync_failureData5* response) {
 if (auto *client = static_cast<MqttNode *>(context); client != nullptr) {
   client->DisconnectFailure5(response);
 }
}


bool MqttNode::IsOnline() const {
 return client_state_ == ClientState::Online;
}

bool MqttNode::IsOffline() const {
 return client_state_ == ClientState::Idle;
}

void MqttNode::ClientTask() {
  client_timer_ = 0;
  client_state_ = ClientState::Idle;
  if (handle_ != nullptr) {
   MQTTAsync_destroy(&handle_);
   handle_ = nullptr;
  }

  while (!stop_client_task_) {
   std::unique_lock client_lock(client_mutex_);
   client_event_.wait_for(client_lock, 100ms, [&] () -> bool {
     return stop_client_task_.load();
   });

   if (stop_client_task_) {
     break;
   }
   ClientState old_state = client_state_;
   switch (client_state_) {
     case ClientState::Idle: // Wait for in-service command
       DoIdle();
       break;

     case ClientState::WaitOnConnect: // Wait for in-service command
       DoWaitOnConnect();
       break;

     case ClientState::Online:
       DoOnline();
       break;

     case ClientState::WaitOnDisconnect:
       DoWaitOnDisconnect();
       break;

     default: // Invalid/Unknown state
       client_timer_ = NowMs() + 10'000;
       client_state_ = ClientState::Idle;
       break;
   }
   if (old_state != client_state_ && listen_.IsActive()) {
     std::ostringstream temp;
     temp << StateToString(old_state) << " -> " << StateToString(client_state_);
     listen_.ListenString(temp.str());
   }
  }

  // Need to send disconnect or wait on the disconnect
  if (client_state_ != ClientState::Idle) {
   if (!IsConnected()) {
     if (listen_.IsActive()) {
       listen_.ListenString("Stop ignored due to not connected to server");
     }
   } else {
     if (listen_.IsActive()) {
       listen_.ListenString("Disconnecting");
     }
     if (client_state_ != ClientState::WaitOnDisconnect) {
       SendDisconnect();
     }
     // Wait for 5s for the disconnect to be delivered
     for (size_t timeout = 0;
          IsConnectionLost() && timeout < 50;
          ++timeout) {
       std::this_thread::sleep_for(100ms);
     }

     if (listen_.IsActive()) {
       listen_.ListenString("Disconnected");
     }
   }
  }

  if (listen_.IsActive()) {
    listen_.ListenString("Stopping client thread");
  }
  if (handle_ != nullptr) {
   MQTTAsync_destroy(&handle_);
   handle_ = nullptr;
  }

}

void MqttNode::StartSubscription() {
 for (const Subscription& sub : subscription_list_ ) {

   MQTTAsync_responseOptions options = MQTTAsync_responseOptions_initializer;
   if (Version() == ProtocolVersion::Mqtt5) {
     options.onSuccess5 = nullptr; // No need of successful subscription
     options.onFailure5 = OnSubscribeFailure5;
   } else {
     options.onSuccess = nullptr;
     options.onFailure = OnSubscribeFailure;
   }
   options.context = this;


   if (listen_.IsActive()) {
     listen_.ListenString("Subscribe: " + sub.subscription);
   }
   const auto subscribe = MQTTAsync_subscribe(handle_, sub.subscription.c_str(),
                                              static_cast<int>(sub.quality_of_service), &options);
   if (subscribe != MQTTASYNC_SUCCESS) {
     METRIC_ERROR() << "Subscription Failed. Topic: "
      << sub.subscription << ". Error: "
      << MQTTAsync_strerror(subscribe);
   }
 }

}

void MqttNode::DoIdle() {
 const auto now = NowMs();
 const bool timeout = now >= client_timer_;

 // Destroy any previously created context/handle.
 if (handle_ != nullptr) {
   MQTTAsync_destroy(&handle_);
   handle_ = nullptr;
 }

 // Check the retry timeout first (10s)
 // Check if in service
 if (!IsInService() ) {
   client_timer_ = 0; // Fiz so it starts directly when on-line is requested
   return;
 }
 if (!timeout) { // Retry timer upon connect failure
   return;
 }

 // In-service create a communication context/handle and connect
 // to the MQTT server.
 const auto create = CreateClient();
 if (!create) {
   client_timer_ = now + 10'000; // 10 second to next create try
   return;
 }

 const auto connect = SendConnect();
 if (!connect) {
   client_timer_ = now + 10'000; // 10 second to next create try
   return;
 }

 // Switch state and wait for connection
 client_timer_ = now + 5'000; // Wait 5 second for connection
 client_state_ = ClientState::WaitOnConnect;
}

void MqttNode::DoWaitOnConnect() {
 const auto now = NowMs();
 if (const bool timeout = now >= client_timer_;timeout) {
   client_timer_ = now + 10'000; // Retry in 10 seconds
   client_state_ = ClientState::Idle;
   return;
 }

 // Check if connected and delivered.
 if (!IsConnected() || !IsDelivered()) {
   return;
 }

 // Start subscriptions and publish the topics for this client.
 // We will not check that it is delivered.
 StartSubscription();
 client_state_ = ClientState::Online;
}

void MqttNode::DoOnline() {
 const auto now = NowMs();
 if (stop_client_task_ || !IsInService() ) {
   SendDisconnect();
   client_timer_ = now + 5'000;
   client_state_ = ClientState::WaitOnDisconnect;
 } else {
   PublishTopics();
 }
}

void MqttNode::PublishTopics() {
  std::scoped_lock lock(topic_mutex_);
  for (auto& topic : topic_list_) {
    if (!topic || !topic->IsPublishing() || !topic->IsUpdated()) {
      continue;
    }
    if (topic->IsUpdated()) {
      topic->ResetUpdated();
      topic->OnPublish();
    }
  }
}

void MqttNode::DoWaitOnDisconnect() {
 const auto now = NowMs();
 const bool timeout = now >= client_timer_;
 if (timeout || IsDelivered() ) {
   client_timer_ = now + 10'000; // Retry in 10s
   client_state_ = ClientState::Idle;
 }
}

void MqttNode::InitMqtt() const {
  static bool done_init = false;
  if (!done_init) {
    MQTTAsync_init_options init_options = MQTTAsync_init_options_initializer;
    if (Transport() == TransportLayer::MqttTcpTls || Transport() == TransportLayer::MqttWebSocketTls) {
      init_options.do_openssl_init = 1;
    }
    MQTTAsync_global_init(&init_options);
    done_init = true;
    if (listen_.IsActive()) {
      listen_.ListenString("Initialized the PAHO MQTT libraryr");
    }
  }

}

void MqttNode::InitSsl() {
  /*
 ssl_options_ = MQTTAsync_SSLOptions_initializer;
 if (!trust_store_.empty()) {
   ssl_options_.trustStore = trust_store_.c_str();
 }
 if (!key_store_.empty()) {
   ssl_options_.keyStore = key_store_.c_str();
 }
 if (!private_key_.empty()) {
   ssl_options_.privateKey = private_key_.c_str();
 }
 if (!private_key_password_.empty()) {
   ssl_options_.privateKeyPassword = private_key_password_.c_str();
 }
 if (!enabled_cipher_suites_.empty()) {
   ssl_options_.enabledCipherSuites = enabled_cipher_suites_.c_str();
 }
 ssl_options_.enableServerCertAuth = enable_cert_auth_ ? 1 : 0;
 ssl_options_.sslVersion = ssl_version_;
 if (!ca_path_.empty()) {
   ssl_options_.CApath = ca_path_.c_str();
 }
 ssl_options_.ssl_error_cb = SslErrorCallback;
 ssl_options_.ssl_error_context = this;
 ssl_options_.disableDefaultTrustStore = disable_default_trust_store_ ? 1 : 0;
 */
}

int MqttNode::SslErrorCallback(const char *error, size_t len, void *) {
 if (len == 0 || error == nullptr) {
   return 0;
 }
 std::string err(len,'\0');
 memcpy(err.data(), error, err.size());

 METRIC_ERROR() << err;
 return 1;
}

} // end namespace