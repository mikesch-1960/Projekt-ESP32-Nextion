/*
  In dieser Version: -----
  - Kommunikation zwischen ESP und Display
  - Mit WifiManager
  - NTP server
  - Die im 'page init event' gemeldeten _timeXYZ Komponenten werden automatisch beim Minuten-/oder Sekundenwechsel aktualisiert, abhängig davon ob im Format ein Sekunden Bezeichner enthalten ist.
  - Die im 'page init event' gemeldeten _wifiXYZ Komponenten werden automatisch aktualisiert
  - Format Makros: 'Program.s' kann im 'config event' bis zu 4 Formate (fmtTime0-3) für _time Komponenten definieren. Diese werden in der 'page init' Meldung mit #0-#3 addressiert. Die Angabe muss die einzige im param sein. Z.B. `_timeM1&#1;` // use second timeformat macro
  - Baudrate zum Nextion Display wurde auf die höchst mögliche gesetzt (921600), dass muss in der HMI Datei im 'Program.s' Skript auch so eingestellt sein!
  - Serial.printf ersetzt durch log_e/w/i/d/v. https://thingpulse.com/esp32-logging/
  - WifiManger um Eingabefelder für MQTT Verbindung erweitert
  - Reconnect to wifi after disconnect, if connection via wifi manager was successfull before.

  Todos: -----
  x LongTouch und swipe im ESP steuern
  ? Wenn Wifi ausfällt: Reconnect ohne Config Portal wenn schon mal ok war. Wann Config Portal?
  - Die zusätzlichen Parameter des WiFiMgr im LittleFS speichern.
  - Das dimmen in der Startseite des Nextion funktioniert nicht
  - implement %M mac address to _wifi format
  - implement %P captive portal active?  to _wifi format
  - Häftiges testen der Konventionen zu den vom Nextion gesendeten componenten, das durch ein #define ein und aus geschaltet werden kann.
  - Aus 'program.s' heraus eine Liste mit aktualisierbaren globalen Variablen(int) und/oder globalen Komponenten in Formularen senden, wie beim 'page init' Ereignis.
    Ideen dazu: comp_t hat zwei Zähler. Einen für die globalen, immer zu aktualisierenden Elemente und einen zweiten für die in der Seite zu aktualisierenden Komponenten. Dabei muss auf doppelte geachtet werden!

  Ideen: -----
  x Im Screen sendxy aktivieren und hier die dadurch gesendeten touch Ereignisse für longPress und swipe auswerten.
    Hier wird dann ein Kommando abgesetzt, das entsprechende globale Variablen im Screen setzt, die dort dann beim release ausgewertet werden können.
    Problem: sendxy sendet immer nur die 'pressed' Koordinate!
    !Ich gebe auf und bleibe dabei das Wischen in den einzelnen Seiten zu machen, da (zumindest bei meinem Nextion) kein Ereignis die Koordinaten beim loslassen sendet.
  v Bei den Parametern zu den _wifi Komponenten eine 'echte' Formatangabe wie '%R dBa' machen können.
  - Bei den Parametern zu den _wifi Komponenten Möglichkeit für bool: _wifiBool.txt&%P[isTrue:isFalse];
  - En-/Disable Buttons by setting Textcolor and checking that color on press/release event

  Interessante Infos
  . zu WiFI:
      connect to wifi (absolutly nonblocking) via FreeRTOS
      https://www.youtube.com/watch?v=YSGPcm-qxDA

  . zu MQTT:
      https://pubsubclient.knolleary.net/api
      https://github.com/knolleary/pubsubclient
      https://arduinojson.org/v6/how-to/use-arduinojson-with-pubsubclient/

  . zum Nextion Display:
      Official Nextion page:  https://nextion.tech/instruction-set/
      Unofficial Nextion/TJC User Forum:  https://unofficialnextion.com/
      Tips, Tricks, and Traps for Nextion Devices:    https://github.com/krizkontrolz/Home-Assistant-nextion_handler/tree/main/Tips_and_Tricks
      A Full Instruction Set (with hidden ones):    https://github.com/UNUF/nxt-doc/blob/main/Protocols/Full%20Instruction%20Set.md#readme
*/

// forward functions ###
// bool wifiConnect();
// bool wifiDisconnect();


#define CORE_DEBUG_LEVEL      ARDUHAL_LOG_LEVEL_VERBOSE      // doesn't seem to work!
// in vscode press F1: 'Arduino: Board Config' to change core debig level

#define strHas(str, what)     (strstr(str, what)!=nullptr)
#define strStart(str, with)     (strstr(str, with)==str)

#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
WiFiManager wifiMgr;

//### LittleFS U:\Quelltexte\Arduino\libraries\LittleFS_esp32\examples\LITTLEFS_test\LittleFS_test.ino
#include "mqttGlobals.h"

bool shouldSaveConfig = false;
bool ConnectionDataOk = false;    // was successfully connected via wifiManager before

// NTP settings
WiFiManagerParameter wmcp_ntp_server("ntpserver", "NTP server:", "0.europe.pool.ntp.org", 51);
WiFiManagerParameter wmcp_ntp_tz("ntptimezone", "NTP timezone config:", "CET-1CEST,M3.5.0/02,M10.5.0/03", 61);
// MQTT settings
WiFiManagerParameter custom_mqtt_server("server", "MQTT Server",  "rpiiobroker", 40);    // 192, 168, 2, 136
WiFiManagerParameter custom_mqtt_port("port", "MQTT Port",  "1883", 6);
WiFiManagerParameter custom_mqtt_username("username", "mqtt username",  "", 40);
WiFiManagerParameter custom_mqtt_password("password", "mqtt password",  "", 40);
// WiFiManagerParameter wmcp_nex_dimHi("nexDimHigh:", "Nextion:\nDim norm.:", "60", 4);//###
// wmcp_nex_dimHi.setValue("80", 4);//### how to change the value in case config from Nextion


/*
  NSPanel specific IO-Pins: https://www.kickstarter.com/projects/sonoffnspanel/sonoff-nspanel-smart-scene-wall-switch/posts/3328417
    Pin   IO              Corresponding function
     7    SENSOR_CAPN     NTC, temperature sensing
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

#include "ntpGlobals.h"
#include "ntp.h"

#include "nexGlobals.h"
#include "nex.h"


void saveParamsCallback () { //###
  log_d("Get Params: %s : %s", wmcp_ntp_server.getID(), wmcp_ntp_server.getValue());
}

bool wifiConnect() {
  log_i("connected to WiFi %s (%s) %ddBa (%d%%)...",
      wifiMgr.getWiFiSSID().c_str(), WiFi.localIP().toString().c_str(),
      WiFi.RSSI(), wifiMgr.getRSSIasQuality(WiFi.RSSI())
  );

  //read updated parameters
  strncpy(NTP_store.ntpServername, wmcp_ntp_server.getValue(), 50);
  strncpy(NTP_store.ntpTimezone, wmcp_ntp_tz.getValue(), 50);

  strncpy(mqtt.server, custom_mqtt_server.getValue(), 40);
  strncpy(mqtt.port, custom_mqtt_port.getValue(), 6);
  strncpy(mqtt.username, custom_mqtt_username.getValue(), 40);
  strncpy(mqtt.password, custom_mqtt_password.getValue(), 40);

  //save the custom parameters to LittleFS
  if (shouldSaveConfig) {
    //### LittleFS to save these settings
    log_i("Saving config...");
  }

  NTP_begin((char*)wmcp_ntp_server.getValue(), (char*)wmcp_ntp_tz.getValue());

  //### set up mqtt like in WiFiMQTTManager.cpp at line 150

  log_i("Wifi connected!");
  return true;
}   // wifiConnect()

bool wifiDisconnect(int test) {
  log_i("Wifi disconnected! ### %d", test);
  return true;
}   // wifiDisconnect()

void setup() {
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP

  Serial.begin(115200);
  Serial.println();  Serial.println();

  NEX_begin(921600 /*### 0=detect */);    // use the fastest possible value! Must be set in Program.s with baud=92160

  /* https://github.com/tzapu/WiFiManager/blob/master/examples/Super/OnDemandConfigPortal/OnDemandConfigPortal.ino */

  // wifiMgr.resetSettings();     //#### reset settings - wipe credentials for testing

  wifiMgr.setDebugOutput(false);
  wifiMgr.setConfigPortalBlocking(false);

  wifiMgr.addParameter(&wmcp_ntp_server);
  wifiMgr.addParameter(&wmcp_ntp_tz);

  wifiMgr.addParameter(&custom_mqtt_server);
  wifiMgr.addParameter(&custom_mqtt_port);
  wifiMgr.addParameter(&custom_mqtt_username);
  wifiMgr.addParameter(&custom_mqtt_password);

  wifiMgr.setSaveParamsCallback(saveParamsCallback);///###

  wifiMgr.setSaveConfigCallback([&]() {
    log_i("### Should save config...");
    shouldSaveConfig = true;
  });

  wifiMgr.setAPCallback([&](WiFiManager *myWiFiManager) {
    log_i("Entering config mode...");
    log_i("  %s", WiFi.softAPIP().toString().c_str());
    log_i("  Connect your device to WiFi SSID %s to configure WiFi and MQTT...", myWiFiManager->getConfigPortalSSID().c_str());
    ConnectionDataOk = false;
    //### Inform Nextion
  });

  // set dark mode via invert class
  wifiMgr.setDarkMode(true);
  wifiMgr.setWiFiAutoReconnect(true);
  wifiMgr.setConnectTimeout(10); // seconds
  wifiMgr.setHostname("NextionNS");

  // automatically connect using saved credentials if they exist.
  // If connection fails it starts an access point with the specified name
  //### if (wifiMgr.autoConnect("Nextion_AP", "michi")) {
  if (wifiMgr.autoConnect("Nextion_AP")) {
    ConnectionDataOk = true;
    wifiConnect();
  }
  else {
    log_i("[WM] Configportal running... %s", wifiMgr.getConfigPortalSSID().c_str());  //### show AP and IP
    ConnectionDataOk = false;
    // was not connected before, so no need to call wifiDisconnect()
    //### Inform Nextion
  }

  log_i("[ESP] Setup Done! Entering loop...");
}


uint8_t payload[MAXLEN_COMPONENT_LIST+1+3+3] = {0}; // plus space for '\0', message-head and message-tail

void loop() {
  wifiMgr.process();


  if (NEX_readPayload(payload, MAXLEN_COMPONENT_LIST, 10)) {
        NEX_handleMsg(payload);
  }   // Nextion Serial available


  if (Serial.available()) {
    Serial.setTimeout(200);

    int len = Serial.readBytes((char *)payload, sizeof(payload)-1);
    log_d("[ESP] got message: '%s' len=%d", payload, len);

    if (len > 2) {   // send ESP message as command to nextion
      payload[len] = '\0';
      while (len > 0 && (uint8_t)payload[len] <= 0x20)     payload[len--] = '\0';   // remove linebreaks and spaces from the end
      NEX_sendCommand((const char *)payload, 200);
    } else
    {
      // on any input that is only one character do
      if ((char)payload[0]=='B')   wifiMgr.reboot();      // reboot ESP
      else
      if ((char)payload[0]=='R')   NEX_sendCommand("rest", false);    // reboot Screen
/* ### only for testing NEX_getInt()
       else
      if ((char)payload[0]=='T')   {
        int32_t x = NEX_getInt("tch2");
        log_d("### getInt returns %d", x);
      }
 */
    }
  }   // ESP Serial available


  if (PG_upd.time.next()) {
    NTP_updTimeinfo();      // update NTP time structure to get current time
    Page_updateTime();   // update time components on current page
    PG_upd.time.storeCurrent();
  }

  // Wifi status changed to connected and was successfully connected before
  if (WiFi.status() == WL_CONNECTED && !PG_upd.wifi.curr.conn && ConnectionDataOk) {
    wifiConnect();
    PG_upd.wifi.curr.conn = 1;
  }

  // Wifi status changed from connected to disconnected
  if (WiFi.status() != WL_CONNECTED && PG_upd.wifi.curr.conn) {
    wifiDisconnect(1);
    PG_upd.wifi.curr.conn = 0;
  }

  if (PG_upd.wifi.next()) {
    Page_updateWifi();
    PG_upd.wifi.storeCurrent();
  }
}   // loop()