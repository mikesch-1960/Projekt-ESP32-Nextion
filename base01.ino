/* x=nicht mehr verfolgt; v=implementiert und verifiziert; ?=implementiert aber nicht genug getestet; !=wichtige oder umfassende Änderung

  In dieser Version: -----
  v Kommunikation zwischen ESP und Display
  X Mit WifiManager
  v AsyncMQTT_Generic Bibliothek verwendet statt WifiManager - hab mich für eine andere AsyncMQTT library entschieden.
  v NTP server
  v Die im 'page init event' gemeldeten _timeXYZ Komponenten werden automatisch beim Minuten-/oder Sekundenwechsel aktualisiert, abhängig davon ob im Format in Sekunden Bezeichner enthalten ist.
  v Die im 'page init event' gemeldeten _wifiXYZ Komponenten werden automatisch aktualisiert
  v Format Makros: 'Program.s' kann im 'config event' bis zu 4 Formate (fmtTime0-3) für _time Komponenten definieren. Diese werden in der 'page init' Meldung mit #0-#3 addressiert. Die Angabe muss die einzige im param sein. Z.B. `_timeM1&#1;` // use second timeformat macro
  v Baudrate zum Nextion Display wurde auf die höchst mögliche gesetzt (921600), dass muss in der HMI Datei im 'Program.s' Skript auch so eingestellt sein!
  v Serial.printf ersetzt durch log_e/w/i/d/v. https://thingpulse.com/esp32-logging/
  v Bei den Parametern zu den _wifi Komponenten eine 'echte' Formatangabe wie '%R dBa' machen können.
  v Bei den Parametern zu den _wifi Komponenten kann ein boolean format '_wifiBool.txt&%P[isTrue:isFalse]' for %C,%P verwendet werden.

  Todos: -----
  x LongTouch und swipe im ESP steuern
  v WifiManager entfernen und https://github.com/marvinroger/async-mqtt-client verwenden. Kein blockierender code, saubere Ereignisse, ...!
  v Das dimmen in der Startseite des Nextion funktioniert nicht
  v implement M mac address to _wifi format
  v Häftiges testen der Konventionen zu den vom Nextion gesendeten Komponenten, das durch ein #define ein und aus geschaltet werden kann.
  v Manche MQTT payloads kommen in mehreren Teilen an!
  - MQTT Verbindung aus 'program.s' config lesen und dann setzen oder überschreiben.
  - NTP Verbindung aus 'program.s' config lesen und dann setzen oder überschreiben.
  - Datentypen für die Anzeige von MQTT IP(i)/Host(H), Port(P) und Verbindungsstatus(B)roker,
    sowie ClientId(c)
  - Aus 'program.s' heraus eine Liste mit aktualisierbaren globalen Variablen(int) und/oder globalen Komponenten in Formularen senden, wie beim 'page init' Ereignis.
    Ideen dazu: comp_t hat zwei Zähler. Einen für die globalen, immer zu aktualisierenden Elemente und einen zweiten für die in der Seite zu aktualisierenden Komponenten. Dabei muss auf doppelte geachtet werden!

  Probleme: -----


  Ideen: -----
  x Im Screen sendxy aktivieren und hier die dadurch gesendeten touch Ereignisse für longPress und swipe auswerten.
    Hier wird dann ein Kommando abgesetzt, das entsprechende globale Variablen im Screen setzt, die dort dann beim release ausgewertet werden können.
    Problem: sendxy sendet immer nur die 'pressed' Koordinate!
    !Ich gebe auf und bleibe dabei das Wischen in den einzelnen Seiten zu machen, da (zumindest bei meinem Nextion) kein Ereignis die Koordinaten beim loslassen sendet.
  v Im Display eine Fehlerseite einbauen, die der ESP verwenden kann um Fehler (z.B. beim 'page int' Ereignis) zu melden.
  - En-/Disable Buttons by setting Textcolor and checking that color on press/release event
  v Das ganze System umstellen: Die Komponentennamen sind egal. Beim 'page init' muss der Komponentenname oder die Komponentenid(#23) angegeben werden. Danach der data-specifier (zusätzlich zu denen vom jetzigen wifi %d=Datum %t=Zeit %m=mqtt), dann der jetzige param.
      Vorteile: kürzere Komponentennamen möglich, standard Komponentennamen möglich, Unterscheidung von Zeit und Datum.
      t0.txt=d/%A %d.%m.%Y^p0.pic=Q[2,5]^n0.val=R/%d dBa^#12=t/%H:%M^ODTmp.txt=m.topicid/%1.1f °C^
  - range auch für mqtt adn T-ime Komponenten ermögliche. Idee kam, weil über AccuWeather eine Icon Nummer gesetzt wird und diese in einer Picture Komponete dargestellt werden kann. Dazu muss allerdings die Zahl der vorgerigen resourcen in den Bildern hinzu addiert werden.
  - Weitere spezielle Fomate außer boolean (Berechnung, Bereich, WortZuWertOderText)

  Interessante Infos
  . zu WiFI:
      connect to wifi (absolutly nonblocking) via FreeRTOS
      https://www.youtube.com/watch?v=YSGPcm-qxDA

  . zu wifimanager:
      mit zusätzlichem html https://github.com/tzapu/WiFiManager/blob/master/examples/Super/OnDemandConfigPortal/OnDemandConfigPortal.ino
        const char* z = "<H1>TEST</H1>";
        WiFiManagerParameter custom_text(z);
        wifiManager.addParameter(&custom_text);
      wifiManager.setCustomHeadElement("<style>body {background-color: powderblue;}</style>");

  . zu MQTT:
      https://stackoverflow.com/questions/35049248/esp8266-wifimanager-pubsubclient
      https://pubsubclient.knolleary.net/api
      https://github.com/knolleary/pubsubclient
      https://arduinojson.org/v6/how-to/use-arduinojson-with-pubsubclient/

  . zum Nextion Display:
      Official Nextion page:  https://nextion.tech/instruction-set/
      Unofficial Nextion/TJC User Forum:  https://unofficialnextion.com/
      Tips, Tricks, and Traps for Nextion Devices:    https://github.com/krizkontrolz/Home-Assistant-nextion_handler/tree/main/Tips_and_Tricks
      A Full Instruction Set (with hidden ones):    https://github.com/UNUF/nxt-doc/blob/main/Protocols/Full%20Instruction%20Set.md#readme

  . zum NSPanel von Sonoff
      https://blakadder.com/nspanel-teardown/   Pins for relays NTC and buzzer and much more
      NSPanel specific IO-Pins: https://www.kickstarter.com/projects/sonoffnspanel/sonoff-nspanel-smart-scene-wall-switch/posts/3328417
        Pin   IO              Corresponding function
         7    SENSOR_CAPN     NTC, Sonoff confirmed it is an NTC thermistor probe model MF52A R = 10K±1% B = 3950K±1%.
        16    GPIO27          KEY2
        17    MTMS            KEY1
        23    GPIO0           for flashing firmware
        41    U0TXD           ESP_TX, connect to ESP
        40    U0RXD           ESP_RX
        24    GPIO4           T_RST, screen reset, high valid
        25    GPIO16          NEX_TX, connect to screen
        27    GPIO17          NEX_RX, connect to screen
        38    GPIO19          RELAY2, secound relay of the NSPanel
        39    GPIO22          RELAY1, first relay of the NSPanel
        42    GPIO24          BUZZER, control passive buzzer
*/


// #define CORE_DEBUG_LEVEL      ARDUHAL_LOG_LEVEL_VERBOSE      // doesn't seem to work!
// in vscode press F1: 'Arduino: Board Config' to change core debug level

#define strHas(str, what)     (strstr(str, what)!=nullptr)

// ------------------------------------------------------------------------------------------------
#pragma region  USER_SETTINGS

// uncomment this to define the credentials inside the else statement
#define USE_CREDENTIALS_FILE

#ifdef USE_CREDENTIALS_FILE
  #include "credentials.h"  // is not included in my github repository!
#else
  #define WIFI_SSID         "YourSSID"
  #define WIFI_PWD          "YourPassword"

  #define MQTT_HOST         IPAddress(192, 168, 8, 99)   // or hostname like "your.mqtt.hostname"
  #define MQTT_PORT         1883
  // If your broker requires authentication, uncomment the two lines below
  // #define MQTT_USER       ""
  // #define MQTT_PWD        ""
  #define MQTT_CLIENTID     "MyNextion"
#endif

// https://github.com/SensorsIot/NTP-time-for-ESP8266-and-ESP32
// https://remotemonitoringsystems.ca/time-zone-abbreviations.php
#define NTP_SERVER          "europe.pool.ntp.org"
#define NTP_TIMEZONE        "CET-1CEST,M3.5.0/02,M10.5.0/03"

#define MQTT_RECONNECT_TIME 5000
#define WIFI_RECONNECT_TIME 5000

#pragma endregion USER_SETTINGS
// ------------------------------------------------------------------------------------------------

#include <WiFi.h>

extern "C"
{
  #include "freertos/FreeRTOS.h"
  #include "freertos/timers.h"
}

TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;

// https://github.com/khoih-prog/AsyncMQTT_Generic
// API reference: https://github.com/khoih-prog/AsyncMQTT_Generic/blob/main/docs/2.-API-reference.md
//#### #include <AsyncMqtt_Generic.h>

// https://github.com/marvinroger/async-mqtt-client
#include <AsyncMqttClient.h>

AsyncMqttClient mqttClient;

#include "ntpGlobals.h"
#include "ntp.h"
#include "nexGlobals.h"
#include "nex.h"
#include "mqtt.h"


void WiFiEvent(WiFiEvent_t event)
{
  switch (event)
  {
#if USING_CORE_ESP32_CORE_V200_PLUS
    case ARDUINO_EVENT_WIFI_READY:
      Serial.println("WiFi ready");
      break;
    case ARDUINO_EVENT_WIFI_STA_START:
      Serial.println("WiFi STA starting");
      break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      Serial.println("WiFi STA connected");
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.println("WiFi connected");
      Serial.print("IP address: "); Serial.println(WiFi.localIP());
      connectToMqtt();
      break;
    case ARDUINO_EVENT_WIFI_STA_LOST_IP:
      Serial.println("WiFi lost IP");
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      Serial.println("WiFi lost connection");
      xTimerStop(mqttReconnectTimer, 0); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
      xTimerStart(wifiReconnectTimer, 0);
      break;
#else
    case SYSTEM_EVENT_STA_GOT_IP:
      log_i("  WiFi connected! IP address: %s", WiFi.localIP().toString().c_str());
      NTP_begin(NTP_SERVER, NTP_TIMEZONE);  // Connecting to NTP server
      NTP_updTimeinfo();
      connectToMqtt();
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("WiFi lost connection!");
      xTimerStop(mqttReconnectTimer, 0); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
      xTimerStart(wifiReconnectTimer, 0);
      break;
#endif
    default:
      break;
  }
}

void connectToWifi()
{
  Serial.println("Connecting to Wi-Fi...");
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  WiFi.setSleep(false);
  WiFi.begin(WIFI_SSID, WIFI_PWD);
}


void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 5000);

  log_i("\nStarting Nextion driver on '%s'", ARDUINO_BOARD);

  NEX_begin(921600);    // use the fastest possible value! Must be set in the Nextion Editor in Program.s with same baudrate!

  mqttReconnectTimer =
    xTimerCreate("mqttTimer", pdMS_TO_TICKS(5000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
  wifiReconnectTimer =
    xTimerCreate("wifiTimer", pdMS_TO_TICKS(5000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));

  WiFi.onEvent(WiFiEvent);
  connectToWifi();

  mqttClient.setCleanSession(true);
  mqttClient.setMaxTopicLength(200);

  mqttClient.onConnect(onMqttConnected);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onPublish(onMqttPublish);

  mqttClient.setServer(MQTT_HOST, MQTT_PORT);

  log_d("[ESP] Setup Done! Entering loop...");
}   // setup()


uint8_t payload[MAXLEN_COMPONENT_LIST+1+3+3] = {0}; // plus space for '\0', message-head and message-tail

void loop() {
  if (NEX_readData(payload, MAXLEN_COMPONENT_LIST, 10)) {
    NEX_handleMsg(payload);
  }   // Nextion Serial available


  if (Serial.available()) {
    Serial.setTimeout(200);

    int len = Serial.readBytes((char *)payload, sizeof(payload)-1);
    log_d("[ESP] got message: '%s' len=%d", payload, len);

    if (len > 2) {   // send ESP message as command to nextion
      payload[len] = '\0';
      while (len > 0 && (uint8_t)payload[len] <= 0x20)     payload[len--] = '\0';   // remove linebreaks and spaces from the end
      NEX_sendCommand((const char *)payload);
    } else
    {
      // on any input that is only one character do
      if ((char)payload[0]=='B')   {ESP.restart(); delay(1000);}      // restart ESP
      else
      if ((char)payload[0]=='R')   NEX_sendCommand("rest");    // reset Screen
    }
  }   // ESP Serial available


  if (PG_upd.time.next()) {
    NTP_updTimeinfo();
    Page_updateTime();      // update time components on current page
    PG_upd.time.storeCurrent();
  }

  if (PG_upd.wifi.next()) {
    Page_updateWifi();
    PG_upd.wifi.storeCurrent();
  }
}   // loop()