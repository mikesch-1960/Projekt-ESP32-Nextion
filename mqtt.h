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

void subscribePage() {
  if (!mqttClient.connected())
    return;

/*
  // Subscribe to "mytopic/test" and display received message to Serial
  client.subscribe("mytopic/test", [](const String & payload) {
    Serial.println(payload);
  });
*/

  char topic[256] = {0};
  log_d("[MQTT] Subscribing to mqtt components on page:");
  for (int idx = PG_upd.mqttStartIdx; idx < PG_upd.compCount; idx++) {
    const comp_t* comp = &PG_upd.compList[idx];
    PG_upd.readAbsoluteTopic(idx, topic);
    log_d("\t topic '%s' from component '%s'!", topic, comp->ptrName);
    mqttClient.subscribe(topic, 0);
    /*#### use packetIdSub to find the corresponding component?!
  uint16_t packetIdSub = mqttClient.subscribe(PubTopic, 2);
    */
  }
  log_d("[MQTT] %d components subscribed!", PG_upd.mqttStartIdx==MAX_COMPONENT_ITEMS ? 0 : PG_upd.compCount - PG_upd.mqttStartIdx);
}


void unsubscribePage() {
  if (!mqttClient.connected())
    return;

  char topic[256] = {0};
  log_d("[MQTT] Unsubscribing mqtt components on page:");
  for (int idx = PG_upd.mqttStartIdx; idx < PG_upd.compCount; idx++) {
    const comp_t* comp = &PG_upd.compList[idx];
    PG_upd.readAbsoluteTopic(idx, topic);
    log_d("\t topic '%s' from component '%s'!", topic, comp->ptrName);
    mqttClient.unsubscribe(topic);
  }
  log_d("[MQTT] %d components unsubscribed!", PG_upd.mqttStartIdx==MAX_COMPONENT_ITEMS ? 0 : PG_upd.compCount - PG_upd.mqttStartIdx);
}


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


void onMqttConnected(bool sessionPresent) {
  Serial.print("Connected to MQTT broker "); Serial.print(MQTT_HOST);
  Serial.print(":"); Serial.println(MQTT_PORT);
  log_d("Session present: %s", sessionPresent ? "true" : "false");

  subscribePage();
/*
  uint16_t packetIdSub = mqttClient.subscribe(PubTopic, 2);
  Serial.print("Subscribing at QoS 2, packetId: "); Serial.println(packetIdSub);

  mqttClient.publish(PubTopic, 0, true, "ESP32 Test");
  Serial.println("Publishing at QoS 0");

  uint16_t packetIdPub1 = mqttClient.publish(PubTopic, 1, true, "test 2");
  Serial.print("Publishing at QoS 1, packetId: "); Serial.println(packetIdPub1);

  uint16_t packetIdPub2 = mqttClient.publish(PubTopic, 2, true, "test 3");
  Serial.print("Publishing at QoS 2, packetId: "); Serial.println(packetIdPub2);
 */
}   // onMqttConnected()


void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
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


void onMqttPublish(const uint16_t& packetId) {
  log_i("Publish acknowledged.  packetId: %d", packetId);
}


void onMqttMessage(char* topic, char* payload, const AsyncMqttClientMessageProperties& properties,
                   const size_t& len, const size_t& index, const size_t& total)
{
  /*
    NOTE: The payload buffer is not terminated by null character and is only then size of the len parameter.
    So `payload[len] = '\0';` will set a the value behind the buffer!
  */
  log_d("[MQTT] ***** message received.  Topic:'%s' payload-len=%lu\n\tqos%d, dup=%d, retain=%d, index=%lu, total=%lu",
      topic, len, properties.qos, properties.dup, properties.retain, index, total);

  char plmsg[2000] = {0};
  bool chr = false;
  for (int i=0; i<len && i<2000; i++) {
    if (i % 30 == 0 && i != 0) {
      if (chr) strcat(plmsg, "'");
      strcat(plmsg, "\n -->");
      if (chr) strcat(plmsg, "'");
    }

    if ((uint8_t)payload[i] >= 32 && (uint8_t)payload[i] < 127) {
      if (!chr) strcat(plmsg, "'");
      sprintf(plmsg+strlen(plmsg), "%c", payload[i]);
      chr = true;
    } else {
      if (chr) strcat(plmsg, "'");
      sprintf(plmsg+strlen(plmsg), "~%02X", (uint8_t)payload[i]);
      chr = false;
    }
  }
  log_d("  payload:%s%s", plmsg, chr ? "'" : " ");

  Page_updateMQTT(topic, payload, len);
}   // onMqttMessage()
