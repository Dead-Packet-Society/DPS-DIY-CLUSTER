#include <WiFi.h>
#include <ArduinoJson.h>
#include <NimBLEDevice.h>


#define NODE_ID 4


const char* WIFI_SSID = "WD-Hub";
const char* WIFI_PASS = "wardrive123";

IPAddress HUB_IP(192, 168, 4, 1);
const uint16_t HUB_PORT = 5000;


const unsigned long WIFI_RETRY_MS         = 5000;
const unsigned long TCP_RETRY_MS          = 3000;
const unsigned long WIFI_SCAN_PER_CHAN_MS = 300;
const unsigned long BLE_SCAN_BURST_S      = 1;
const unsigned long HEARTBEAT_INTERVAL_MS = 2000;


#define ROLE_WIFI_24  1  
#define ROLE_WIFI_5   2  
#define ROLE_BLE      3

int nodeRole = ROLE_WIFI_24;


int wifiChannels[64];
int wifiChannelCount = 0;


WiFiClient tcpClient;

bool scanningEnabled = false;

unsigned long lastWiFiAttempt = 0;
unsigned long lastTCPAttempt = 0;
unsigned long lastHeartbeat = 0;

String tcpLineBuffer;

void loadDefaultConfig() {

  wifiChannelCount = 0;

  if (NODE_ID == 1) {

    nodeRole = ROLE_WIFI_24;

    for (int ch = 1; ch <= 7; ch++) {
      wifiChannels[wifiChannelCount++] = ch;
    }  
   

  } else if (NODE_ID == 2) {

    nodeRole = ROLE_WIFI_24;

    for (int ch = 8; ch <= 13; ch++) {
      wifiChannels[wifiChannelCount++] = ch;
    }
  

  } else if (NODE_ID == 3) {

    nodeRole = ROLE_WIFI_5;

    int channels5[] = {
      36, 40, 44, 48,
      52, 56, 60, 64,
      100, 104, 108, 112,
      116, 120, 124, 128,
      132, 136, 140, 144,
      149, 153, 157, 161, 165
    };

    wifiChannelCount =
        sizeof(channels5) / sizeof(channels5[0]);

    for (int i = 0; i < wifiChannelCount; i++) {
      wifiChannels[i] = channels5[i];
    }

  } else if (NODE_ID == 4) {

    nodeRole = ROLE_BLE;
  
  } else if (NODE_ID == 5) {

    nodeRole = ROLE_BLE;
  
  } else if (NODE_ID == 6) {

    nodeRole = ROLE_BLE;
  
  } else if (NODE_ID == 7) {

    nodeRole = ROLE_BLE;
  
  } else if (NODE_ID == 8) {

    nodeRole = ROLE_BLE;
  
  } else if (NODE_ID == 9) {

    nodeRole = ROLE_BLE;
  
  } else if (NODE_ID == 10) {

    nodeRole = ROLE_BLE;
  }
}


int channelToFrequency(int channel) {

  // 2.4 GHz channels 1-13
  if (channel >= 1 && channel <= 13) {
    return 2407 + (channel * 5);
  }

  // 2.4 GHz channel 14
  if (channel == 14) {
    return 2484;
  }

  // 5 GHz
  if (channel >= 32 && channel <= 177) {
    return 5000 + (channel * 5);
  }

  return 0;
}


String encryptionToString(wifi_auth_mode_t enc) {

  switch (enc) {

    case WIFI_AUTH_OPEN:
      return "OPEN";

    case WIFI_AUTH_WEP:
      return "WEP";

    case WIFI_AUTH_WPA_PSK:
      return "WPA_PSK";

    case WIFI_AUTH_WPA2_PSK:
      return "WPA2_PSK";

    case WIFI_AUTH_WPA_WPA2_PSK:
      return "WPA_WPA2_PSK";

    case WIFI_AUTH_WPA2_ENTERPRISE:
      return "WPA2_ENTERPRISE";

    case WIFI_AUTH_WPA3_PSK:
      return "WPA3_PSK";

    case WIFI_AUTH_WPA2_WPA3_PSK:
      return "WPA2_WPA3_PSK";

    default:
      return "UNKNOWN";
  }
}


void sendLine(JsonDocument& doc) {

  if (!tcpClient.connected()) {
    return;
  }

  String output;

  serializeJson(doc, output);

  tcpClient.println(output);

  Serial.println("[TCP] NODE -> HELTEC");
  Serial.println(output);
}


void sendHeartbeat() {

  if (!tcpClient.connected()) {
    return;
  }

  StaticJsonDocument<128> doc;

  doc["node"] = NODE_ID;
  doc["type"] = "heartbeat";
  doc["ts"] = millis();

  sendLine(doc);
}


void ensureWiFi() {

  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  unsigned long now = millis();

  if (now - lastWiFiAttempt < WIFI_RETRY_MS) {
    return;
  }

  lastWiFiAttempt = now;

  Serial.println();
  Serial.println("[WiFi] Connecting to WD-Hub...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long start = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - start < 4000) {

    delay(100);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {

    Serial.println("[WiFi] Connected");

    Serial.print("[WiFi] IP: ");
    Serial.println(WiFi.localIP());

    Serial.print("[WiFi] RSSI: ");
    Serial.println(WiFi.RSSI());

  } else {

    Serial.println("[WiFi] Connection failed");
  }
}


void ensureTCP() {

  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  if (tcpClient.connected()) {
    return;
  }

  unsigned long now = millis();

  if (now - lastTCPAttempt < TCP_RETRY_MS) {
    return;
  }

  lastTCPAttempt = now;

  Serial.println();
  Serial.println("[TCP] Connecting to Heltec...");

  if (tcpClient.connect(HUB_IP, HUB_PORT)) {

    Serial.println("[TCP] Connected to Heltec");

    tcpClient.setNoDelay(true);

    lastHeartbeat = millis();

  } else {

    Serial.println("[TCP] Connection failed");
  }
}


void readTCPCommands() {

  if (!tcpClient.connected()) {
    return;
  }

  while (tcpClient.available()) {

    char c = tcpClient.read();

    if (c == '\n') {

      String line = tcpLineBuffer;

      tcpLineBuffer = "";

      line.trim();

      if (line.length() > 0) {

        Serial.println();
        Serial.println("[TCP] HELTEC -> NODE");
        Serial.println(line);

        processCommand(line);
      }

    } else if (c != '\r') {

      tcpLineBuffer += c;

      if (tcpLineBuffer.length() > 2048) {
        tcpLineBuffer = "";
      }
    }
  }
}


void processCommand(const String& line) {

  StaticJsonDocument<1024> doc;

  DeserializationError err =
      deserializeJson(doc, line);

  if (err) {

    Serial.print("[CMD] JSON ERROR: ");
    Serial.println(err.c_str());

    return;
  }

  const char* cmd = doc["cmd"];

  if (!cmd) {
    return;
  }

  String command = String(cmd);


  if (command == "start") {

    scanningEnabled = true;

    Serial.println("[CMD] START");

    return;
  }


  if (command == "stop") {

    scanningEnabled = false;

    Serial.println("[CMD] STOP");

    return;
  }


  if (command == "start_def") {

    loadDefaultConfig();

    scanningEnabled = true;

    Serial.println("[CMD] START_DEF");

    return;
  }


  if (command == "set_node_config") {

    int targetNode = doc["node"] | -1;

    if (targetNode != NODE_ID) {
      return;
    }

    Serial.println("[CMD] SET_NODE_CONFIG");

    const char* role =
        doc["role"] | "";

    if (String(role) == "wifi24") {

      nodeRole = ROLE_WIFI_24;

    } else if (String(role) == "wifi5") {

      nodeRole = ROLE_WIFI_5;

    } else if (String(role) == "ble") {

      nodeRole = ROLE_BLE;
    }

    if (doc["channels"].is<JsonArray>()) {

      JsonArray channels =
          doc["channels"].as<JsonArray>();

      wifiChannelCount = 0;

      for (JsonVariant v : channels) {

        if (wifiChannelCount >= 64) {
          break;
        }

        wifiChannels[wifiChannelCount++] =
            v.as<int>();
      }
    }

    Serial.print("[CONFIG] Role: ");
    Serial.println(nodeRole);

    Serial.print("[CONFIG] Channels: ");

    for (int i = 0; i < wifiChannelCount; i++) {

      Serial.print(wifiChannels[i]);

      if (i < wifiChannelCount - 1) {
        Serial.print(",");
      }
    }

    Serial.println();

    return;
  }


  if (command == "status") {

    StaticJsonDocument<512> status;

    status["node"] = NODE_ID;
    status["type"] = "status";
    status["scanning"] = scanningEnabled;
    status["wifi"] =
        WiFi.status() == WL_CONNECTED;
    status["tcp"] =
        tcpClient.connected();
    status["role"] = nodeRole;

    JsonArray channels =
        status.createNestedArray("channels");

    for (int i = 0; i < wifiChannelCount; i++) {
      channels.add(wifiChannels[i]);
    }

    sendLine(status);

    return;
  }

  Serial.print("[CMD] Unknown command: ");
  Serial.println(command);
}


void performWiFiScan() {

  if (!tcpClient.connected()) {
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  if (wifiChannelCount == 0) {
    return;
  }

  Serial.println();
  Serial.println("[SCAN] Starting Wi-Fi scan");

  for (int chIndex = 0;
       chIndex < wifiChannelCount;
       chIndex++) {

    if (!scanningEnabled) {
      break;
    }

    int channel =
        wifiChannels[chIndex];

    Serial.print("[SCAN] Channel ");
    Serial.println(channel);


    int count =
        WiFi.scanNetworks(
            false,
            true,
            false,
            WIFI_SCAN_PER_CHAN_MS,
            channel
        );

    if (count <= 0) {

      WiFi.scanDelete();

      continue;
    }

    Serial.print("[SCAN] Found ");
    Serial.print(count);
    Serial.println(" networks");

    for (int i = 0; i < count; i++) {

      String bssid =
          WiFi.BSSIDstr(i);

      String ssid =
          WiFi.SSID(i);

      int rssi =
          WiFi.RSSI(i);

      int actualChannel =
          WiFi.channel(i);

      if (actualChannel <= 0) {
        actualChannel = channel;
      }

      int frequency =
          channelToFrequency(actualChannel);

      wifi_auth_mode_t enc =
          WiFi.encryptionType(i);

      String encryption =
          encryptionToString(enc);

      StaticJsonDocument<768> doc;

      doc["node"] = NODE_ID;
      doc["type"] = "wifi";


      doc["bssid"] = bssid;
      doc["ssid"] = ssid;
      doc["rssi"] = rssi;
      doc["ch"] = actualChannel;
      doc["frequency"] = frequency;
      doc["enc"] = encryption;

      doc["ts"] = millis();

      sendLine(doc);
    }

    WiFi.scanDelete();

    delay(20);
  }

  Serial.println("[SCAN] Wi-Fi scan complete");
}


NimBLEScan* pBleScan = nullptr;

class AdvertisedCallback : public NimBLEScanCallbacks {

  void onResult(
      const NimBLEAdvertisedDevice* dev) override {

    if (!tcpClient.connected()) {
      return;
    }

    if (!scanningEnabled) {
      return;
    }

    String mac =
        dev->getAddress().toString().c_str();

    int rssi =
        dev->getRSSI();

    String name = "";

    if (dev->haveName()) {

      name =
          String(dev->getName().c_str());
    }

    StaticJsonDocument<512> doc;

    doc["node"] = NODE_ID;
    doc["type"] = "ble";
    doc["mac"] = mac;
    doc["rssi"] = rssi;

    if (name.length() > 0) {
      doc["name"] = name;
    }

    doc["ts"] = millis();

    sendLine(doc);
  }
};


void performBLEScan() {

  if (!tcpClient.connected()) {
    return;
  }

  if (!scanningEnabled) {
    return;
  }

  if (!pBleScan) {

    pBleScan =
        NimBLEDevice::getScan();

    pBleScan->setScanCallbacks(
        new AdvertisedCallback(),
        true
    );

    pBleScan->setActiveScan(true);
    pBleScan->setInterval(100);
    pBleScan->setWindow(99);
  }

  Serial.println();
  Serial.println("[SCAN] BLE burst");

  pBleScan->start(
      BLE_SCAN_BURST_S,
      false
  );

  pBleScan->clearResults();

  Serial.println("[SCAN] BLE complete");
}


void performScan() {

  if (!scanningEnabled) {
    return;
  }

  if (!tcpClient.connected()) {
    return;
  }

  if (nodeRole == ROLE_WIFI_24 ||
      nodeRole == ROLE_WIFI_5) {

    performWiFiScan();

  } else if (nodeRole == ROLE_BLE) {

    performBLEScan();
  }
}


void setup() {

  Serial.begin(115200);

  delay(1000);

  Serial.println();
  Serial.println();
  Serial.println("==========================================");
  Serial.println(" DEAD PACKET SOCIETY");
  Serial.println(" XIAO ESP32-C5 SCANNER");
  Serial.println("==========================================");

  Serial.print("NODE ID: DPS-");

  if (NODE_ID < 10) {
    Serial.print("0");
  }

  Serial.println(NODE_ID);

  loadDefaultConfig();

  Serial.print("[CONFIG] Role: ");
  Serial.println(nodeRole);

  Serial.print("[CONFIG] Channels: ");

  for (int i = 0; i < wifiChannelCount; i++) {

    Serial.print(wifiChannels[i]);

    if (i < wifiChannelCount - 1) {
      Serial.print(",");
    }
  }

  Serial.println();


  String bleName =
      "DPS-" + String(NODE_ID);

  NimBLEDevice::init(
      bleName.c_str()
  );


  WiFi.mode(WIFI_STA);

  WiFi.setSleep(false);

  ensureWiFi();


  ensureTCP();

  Serial.println();
  Serial.println("==========================================");
  Serial.println(" NODE ONLINE");
  Serial.println("==========================================");
}


void loop() {

  ensureWiFi();

  ensureTCP();

  readTCPCommands();


  if (tcpClient.connected()) {

    unsigned long now = millis();

    if (now - lastHeartbeat >=
        HEARTBEAT_INTERVAL_MS) {

      lastHeartbeat = now;

      sendHeartbeat();
    }
  }


  if (scanningEnabled &&
      tcpClient.connected()) {

    performScan();
  }

  delay(10);
}