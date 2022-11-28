// https://www.hivemq.com/blog/mqtt-essentials-part-5-mqtt-topics-best-practices/
// https://github.com/ioBroker/ioBroker.mqtt/issues/182

/*
Test topics----------------
0_userdata.0.example_data
accuweather.0.Current.Temperature
accuweather.0.Current.Pressure
accuweather.0.Current.WeatherText
accuweather.0.Current.RelativeHumidity
*/

#define TEST_TOPIC "0_userdata/0/example_data"


void connectToMqtt() {
  log_i("Connecting to MQTT...");

  #ifdef MQTT_USER
    mqttClient.setCredentials(MQTT_USER, MQTT_PWD);
  #endif
  #ifdef MQTT_CLIENTID
    mqttClient.setClientId(MQTT_CLIENTID);
  #endif
  mqttClient.connect();
}


void onMqttConnect(bool sessionPresent) {
  Serial.print("Connected to MQTT broker "); Serial.print(MQTT_HOST);
  Serial.print(":"); Serial.println(MQTT_PORT);
  log_d("Session present: %s", sessionPresent ? "true" : "false");

  // mqttClient.subscribe(TEST_TOPIC, 0);//### muss in 'page init' passieren oder extra Meldung 'page mqtt'

  // uint16_t packetIdSub = mqttClient.subscribe(PubTopic, 2);
  // Serial.print("Subscribing at QoS 2, packetId: "); Serial.println(packetIdSub);

  // mqttClient.publish(PubTopic, 0, true, "ESP32 Test");
  // Serial.println("Publishing at QoS 0");

  // uint16_t packetIdPub1 = mqttClient.publish(PubTopic, 1, true, "test 2");
  // Serial.print("Publishing at QoS 1, packetId: "); Serial.println(packetIdPub1);

  // uint16_t packetIdPub2 = mqttClient.publish(PubTopic, 2, true, "test 3");
  // Serial.print("Publishing at QoS 2, packetId: "); Serial.println(packetIdPub2);
}


void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
  // (void) reason;
  // log_i("Disconnected from MQTT.");
  Serial.printf("Disconnected from MQTT: %u\n", static_cast<std::underlying_type<AsyncMqttClientDisconnectReason>::type>(reason));

  if (WiFi.isConnected()) {
    xTimerStart(mqttReconnectTimer, 0);
  }
}

void onMqttSubscribe(const uint16_t& packetId, const uint8_t& qos) {
  log_d("Subscribe acknowledged.  packetId: %d qos: %d", packetId, qos);
}

void onMqttUnsubscribe(const uint16_t& packetId) {
  log_d("Unsubscribe acknowledged.  packetId: %d", packetId);
}

void onMqttMessage(char* topic, char* payload, const AsyncMqttClientMessageProperties& properties,
                   const size_t& len, const size_t& index, const size_t& total)
{
  // (void) payload;

  log_v("Publish received.");
  Serial.print("  topic: ");  Serial.println(topic);
  Serial.print("  qos: ");    Serial.println(properties.qos);
  Serial.print("  dup: ");    Serial.println(properties.dup);
  Serial.print("  retain: "); Serial.println(properties.retain);
  Serial.print("  len: ");    Serial.println(len);
  Serial.print("  index: ");  Serial.println(index);
  Serial.print("  total: ");  Serial.println(total);

  char buf[200];//####
  payload[len] = '\0';
  sprintf(buf, "topic1.txt=\"%s\"", payload);
  NEX_sendCommand(buf, false);
}

void onMqttPublish(const uint16_t& packetId) {
  log_i("Publish acknowledged.  packetId: %d", packetId);
}
