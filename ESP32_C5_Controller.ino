/*
╔══════════════════════════════════════════════════════════════╗
║                                                              ║
║                 DEAD PACKET SOCIETY                          ║
║                                                              ║
║                    NIGHTSHADE                                ║
║                                                              ║
║              GENERIC ESP32-C5 CONTROLLER                     ║
║                                                              ║
║  Coded by Nightshade — Dead Packet Society                   ║
║                                                              ║
║  Part of the DPS field communications system.                ║
║  Android BLE → ESP32-C5 → Wi-Fi TCP → DPS Nodes              ║
║                                                              ║
║  Built for the field.                                        ║
║  Built for the signal.                                       ║
║  Built by the Society.                                       ║
║                                                              ║
║                    SYSTEM ONLINE                             ║
║                                                              ║
╚══════════════════════════════════════════════════════════════╝
*/


#include <WiFi.h>
#include <ArduinoJson.h>
#include <NimBLEDevice.h>




const char* AP_SSID     = "WD-Hub";
const char* AP_PASSWORD = "wardrive123";

const uint16_t TCP_PORT = 5000;


const char* BLE_NAME = AP_SSID;
bool SCAN = false;
#define BLE_SERVICE_UUID \
  "12345678-1234-1234-1234-1234567890AB"

#define BLE_COMMAND_UUID \
  "12345678-1234-1234-1234-1234567890AC"

#define BLE_STATUS_UUID \
  "12345678-1234-1234-1234-1234567890AD"

#define BLE_CCCD_UUID \
  "00002902-0000-1000-8000-00805f9b34fb"

const size_t BLE_NOTIFY_CHUNK_SIZE = 20;


#define LCD_H_RES        172
#define LCD_V_RES        320
#define LCD_SPI_FREQ_HZ  (40 * 1000 * 1000)

#define LCD_SPI_SCLK     7
#define LCD_SPI_MOSI     6
#define LCD_SPI_MISO     GFX_NOT_DEFINED
#define LCD_SPI_CS       23
#define LCD_SPI_DC       24
#define LCD_SPI_RST      26
#define LCD_BACKLIGHT    10

#define LCD_X_GAP        34
#define LCD_Y_GAP        0

#define LCD_ROTATION     0

#define BACKLIGHT_LEDC_CH       0
#define BACKLIGHT_LEDC_FREQ_HZ  5000
#define BACKLIGHT_LEDC_BITS     8

#if __has_include(<esp_arduino_version.h>)
#include <esp_arduino_version.h>
#else
#define ESP_ARDUINO_VERSION_MAJOR 2
#endif

#define MAX_NODES 10
#define AP_MAX_CONNECTIONS 10


#define NODE_STATUS_BATCH_COUNT 10
#define STATUS_TIME_FALLBACK_ENABLED 1
#define STATUS_WINDOW_MS 3000


#define BLE_VERBOSE_LOG 0

struct NodeInfo {
  WiFiClient client;
  String nodeId;
  bool active;
  unsigned long lastHeartbeatMs;
  String lineBuf;
  String wifiMac;
  String bleMac;
};

NodeInfo nodes[MAX_NODES];


bool seenNodeNumber[MAX_NODES + 1] = { false };

unsigned long lastStatusSentMs = 0;

int nodeStatusMessagesSinceSend = 0;



WiFiServer nodeServer(TCP_PORT);


NimBLEServer* bleServer = nullptr;
NimBLECharacteristic* commandCharacteristic = nullptr;
NimBLECharacteristic* statusCharacteristic = nullptr;
NimBLEAdvertising* bleAdvertising = nullptr;

bool bleConnected = false;
unsigned long lastBleAdvertisingCheck = 0;

unsigned long lastDisplayUpdate = 0;

uint32_t wifiObservationCount = 0;
uint32_t bleObservationCount = 0;


void setBacklight(uint8_t percent) {
  if (percent > 100) {
    percent = 100;
  }

  const uint32_t duty =
      (percent * ((1 << BACKLIGHT_LEDC_BITS) - 1)) / 100;

#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(LCD_BACKLIGHT, duty);
#else
  ledcWrite(BACKLIGHT_LEDC_CH, duty);
#endif
}

void initBacklight(uint8_t percent) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(
    LCD_BACKLIGHT,
    BACKLIGHT_LEDC_FREQ_HZ,
    BACKLIGHT_LEDC_BITS
  );
#else
  ledcSetup(
    BACKLIGHT_LEDC_CH,
    BACKLIGHT_LEDC_FREQ_HZ,
    BACKLIGHT_LEDC_BITS
  );

  ledcAttachPin(
    LCD_BACKLIGHT,
    BACKLIGHT_LEDC_CH
  );
#endif

  setBacklight(percent);
}




int getConnectedNodeCount() {
  int count = 0;

  for (int i = 0; i < MAX_NODES; i++) {
    if (
      nodes[i].client.connected()// &&
      
    ) {
      count++;
    }
  }
  
  return count;
}




void clearNode(int index) {
  if (index < 0 || index >= MAX_NODES) {
    return;
  }

  nodes[index].client.stop();

  nodes[index].nodeId = "";
  nodes[index].active = false;
  nodes[index].lastHeartbeatMs = 0;
  nodes[index].lineBuf = "";
  nodes[index].wifiMac = "";
  nodes[index].bleMac = "";
}


int findNodeById(const String& nodeId) {
  for (int i = 0; i < MAX_NODES; i++) {
    if (nodes[i].nodeId == nodeId) {
      return i;
    }
  }

  return -1;
}


int findFreeNodeSlot() {
  for (int i = 0; i < MAX_NODES; i++) {
    if (!nodes[i].client.connected()) {
      return i;
    }
  }

  return -1;
}


void updateNodeActivity(int index) {
  if (index < 0 || index >= MAX_NODES) {
    return;
  }

  nodes[index].active = true;
  nodes[index].lastHeartbeatMs = millis();
}


String getControllerBssid() {
  return WiFi.softAPmacAddress();
}


String makeDefaultNodeId(int number) {
  if (number >= 1 && number < 100) {
    char buf[8];
    snprintf(buf, sizeof(buf), "DPS-%02d", number);
    return String(buf);
  }

  return "DPS-" + String(number);
}



int nodeNumberOfSlot(int slot) {
  const String& id = nodes[slot].nodeId;

  int end = id.length();
  int start = end;

  while (start > 0 && isDigit(id[start - 1])) {
    start--;
  }

  if (start == end) {
    return slot + 1;
  }

  return id.substring(start).toInt();
}



void sendBlePayload(const String& text) {
  if (
    !bleConnected ||
    statusCharacteristic == nullptr
  ) {
    return;
  }

  const size_t totalBytes = text.length();

  if (totalBytes == 0) {
    return;
  }

  size_t offset = 0;
  int chunkNumber = 0;

  const int totalChunks =
      (totalBytes +
       BLE_NOTIFY_CHUNK_SIZE -
       1) /
      BLE_NOTIFY_CHUNK_SIZE;

  while (offset < totalBytes) {
    const size_t remaining =
        totalBytes - offset;

    const size_t chunkLength =
        remaining > BLE_NOTIFY_CHUNK_SIZE
        ? BLE_NOTIFY_CHUNK_SIZE
        : remaining;

    chunkNumber++;

    bool sent =
        statusCharacteristic->notify(
          (const uint8_t*)text.c_str() + offset,
          chunkLength
        );

#if BLE_VERBOSE_LOG
    Serial.print("[BLE] NOTIFY CHUNK ");
    Serial.print(chunkNumber);
    Serial.print("/");
    Serial.print(totalChunks);
    Serial.print(" BYTES: ");
    Serial.print(chunkLength);
    Serial.print(" RESULT: ");
    Serial.println(
      sent ? "SUCCESS" : "FAILED"
    );
#endif

    if (!sent) {
      Serial.println(
        "[BLE] Notification chunk failed"
      );
      break;
    }

    offset += chunkLength;

    delay(2);
  }

#if BLE_VERBOSE_LOG
  Serial.println(
    "[BLE] Notification transmission complete"
  );
#endif
}



void noteNodeStatusReceived() {
  nodeStatusMessagesSinceSend++;

  if (nodeStatusMessagesSinceSend >= NODE_STATUS_BATCH_COUNT) {
    sendStatus();
  }
}



void sendStatus() {
  if (
    !bleConnected ||
    statusCharacteristic == nullptr
  ) {
    return;
  }

  lastStatusSentMs = millis();


  nodeStatusMessagesSinceSend = 0;

 
  int slotForNumber[MAX_NODES + 1];

  for (int n = 0; n <= MAX_NODES; n++) {
    slotForNumber[n] = -1;
  }

  for (int i = 0; i < MAX_NODES; i++) {
    if (nodes[i].nodeId.length() == 0) {
      continue;
    }

    const int n = nodeNumberOfSlot(i);

    if (n < 1 || n > MAX_NODES) {
      continue;
    }

    seenNodeNumber[n] = true;

    const int existing =
        slotForNumber[n];

  
    if (
      existing < 0 ||
      nodes[i].lastHeartbeatMs >
      nodes[existing].lastHeartbeatMs
    ) {
      slotForNumber[n] = i;
    }
  }

  
  const size_t STATUS_JSON_CAPACITY =
      JSON_OBJECT_SIZE(6) +
      JSON_ARRAY_SIZE(MAX_NODES) +
      (size_t)MAX_NODES *
      (JSON_OBJECT_SIZE(8) + 96);

  DynamicJsonDocument doc(
      STATUS_JSON_CAPACITY
  );

  doc["type"] =
      "status";

  doc["controller"] =
      "WAVESHARE_C5";

  doc["system"] =
      "SYSTEM ONLINE";

  doc["ble"] =
      bleConnected;

  doc["controller_bssid"] =
      getControllerBssid();

  doc["controller_ssid"] =
      AP_SSID;

  JsonArray nodeArray =
      doc.createNestedArray(
          "nodes"
      );

  
  for (
    int n = 1;
    n <= MAX_NODES;
    n++
  ) {
    JsonObject node =
        nodeArray.createNestedObject();

    const int slot =
        slotForNumber[n];

    String id =
        makeDefaultNodeId(n);

    bool online =
        false;

    unsigned long lastBeat =
        0;

    if (slot >= 0) {

      if (
        nodes[slot]
            .nodeId.length() > 0
      ) {
        id =
            nodes[slot]
                .nodeId;
      }

      online =
          nodes[slot]
              .client.connected() &&
          nodes[slot]
              .active;

      lastBeat =
          nodes[slot]
              .lastHeartbeatMs;
    }

   
    node["id"] =
        id;

    node["node"] =
        id;

    node["node_id"] =
        id;

    
    node["slot"] =
        n;

    node["online"] =
        online;

    node["last_heartbeat"] =
        lastBeat;

    
    if (
      slot >= 0 &&
      nodes[slot]
          .wifiMac.length() > 0
    ) {
      node["wifi_mac"] =
          nodes[slot]
              .wifiMac;
    }

    if (
      slot >= 0 &&
      nodes[slot]
          .bleMac.length() > 0
    ) {
      node["ble_mac"] =
          nodes[slot]
              .bleMac;
    }
  }

  String output;

  serializeJson(
      doc,
      output
  );

  if (
    doc.overflowed()
  ) {
    Serial.println();
    Serial.println(
      "[BLE] WARNING: status JSON buffer overflowed — "
      "increase STATUS_JSON_CAPACITY sizing in sendStatus()"
    );
  }

  Serial.print(
    "[BLE] STATUS -> APP ("
  );

  Serial.print(
    output.length()
  );

  Serial.println(
    " bytes)"
  );

 
  Serial.print(
    "[BLE] NODES: "
  );

  for (
    int n = 1;
    n <= MAX_NODES;
    n++
  ) {
    const int slot =
        slotForNumber[n];

    bool online =
        false;

    if (slot >= 0) {
      online =
          nodes[slot]
              .client.connected() &&
          nodes[slot]
              .active;
    }

    Serial.print(
      "DPS-"
    );

    if (n < 10) {
      Serial.print(
        "0"
      );
    }

    Serial.print(
      n
    );

    Serial.print(
      "="
    );

    Serial.print(
      online
        ? "ONLINE"
        : "OFFLINE"
    );

    if (
      n < MAX_NODES
    ) {
      Serial.print(
        " | "
      );
    }
  }

  Serial.println();

  sendBlePayload(
      output
  );
}



void sendStatusIfDue() {
#if STATUS_TIME_FALLBACK_ENABLED
  if (!bleConnected) {
    return;
  }

  if (millis() - lastStatusSentMs < STATUS_WINDOW_MS) {
    return;
  }

  sendStatus();
#endif
}

void sendToApp(const String& text) {
  if (
    !bleConnected ||
    statusCharacteristic == nullptr
  ) {
    return;
  }

  sendBlePayload(text);
}


void sendToNode(
  int index,
  const String& command
) {
  if (
    index < 0 ||
    index >= MAX_NODES
  ) {
    return;
  }

  if (!nodes[index].client.connected()) {
    return;
  }

  String cmd = command;
  cmd.trim();

  if (cmd.length() == 0) {
    return;
  }

  Serial.print("[TCP] WAVESHARE -> ");

  if (nodes[index].nodeId.length() > 0) {
    Serial.print(nodes[index].nodeId);
  } else {
    Serial.print("NODE");
    Serial.print(index + 1);
  }

  Serial.print(": ");
  Serial.println(cmd);

  nodes[index].client.println(cmd);
  nodes[index].client.flush();
}


void forwardToActiveNodes(const String& command) {
  for (int i = 0; i < MAX_NODES; i++) {
    if (
      nodes[i].client.connected() &&
      nodes[i].active
    ) {
      sendToNode(i, command);
    }
  }
}



void processCommand(const String& command) {
  String cmd = command;
  cmd.trim();

  Serial.println();
  Serial.println("==========================================");
  Serial.print("[CMD] APP -> CONTROLLER: ");
  Serial.println(cmd);
  Serial.println("==========================================");

  if (cmd == "STATUS") {
    sendStatus();
    return;
  }

  if (cmd == "NODES") {
    sendStatus();
    return;
  }

  if (cmd == "START_ALL") {
    StaticJsonDocument<128> doc;
    doc["cmd"] = "start";

    String json;
    serializeJson(doc, json);

    Serial.print("[CMD] FORWARD START_ALL -> ");
    Serial.println(json);

    forwardToActiveNodes(json);
    SCAN = 1;
    
    return;
  }

  if (cmd == "START_DEF") {
    StaticJsonDocument<128> doc;
    doc["cmd"] = "start_def";

    String json;
    serializeJson(doc, json);

    Serial.print("[CMD] FORWARD START_DEF -> ");
    Serial.println(json);

    forwardToActiveNodes(json);
    SCAN = 1;
    
    return;
  }

  if (cmd == "STOP") {
    StaticJsonDocument<128> doc;
    doc["cmd"] = "stop";

    String json;
    serializeJson(doc, json);

    Serial.print("[CMD] FORWARD STOP -> ");
    Serial.println(json);

    forwardToActiveNodes(json);
    SCAN = 0;
   
    return;
  }

  if (cmd == "CLEAR") {
    Serial.println(
      "[CMD] CLEAR ignored - not supported by C5"
    );
    return;
  }

  if (cmd == "TEST") {
    Serial.println(
      "[CMD] TEST ignored - not supported by C5"
    );
    return;
  }

  if (cmd.startsWith("NODE ")) {
    String remainder =
        cmd.substring(5);

    remainder.trim();

    int separator =
        remainder.indexOf(' ');

    if (separator < 0) {
      Serial.println(
        "[CMD] Invalid NODE command"
      );
      return;
    }

    String nodeId =
        remainder.substring(
          0,
          separator
        );

    String nodeCommand =
        remainder.substring(
          separator + 1
        );

    nodeId.trim();
    nodeCommand.trim();

    int index =
        findNodeById(nodeId);

    if (index < 0) {
      Serial.print(
        "[CMD] Node not found: "
      );
      Serial.println(nodeId);
      return;
    }

    sendToNode(
      index,
      nodeCommand
    );

    return;
  }

  Serial.print("[CMD] Unknown command: ");
  Serial.println(cmd);
}


void processNodeJson(
  int index,
  const String& line
) {
  StaticJsonDocument<4096> doc;

  DeserializationError err =
      deserializeJson(doc, line);

  if (err) {
    Serial.print(
      "[JSON] Parse error from node: "
    );
    Serial.println(err.c_str());
    return;
  }

  int nodeNumber =
      doc["node"] | 0;

  const char* type =
      doc["type"] | "";





  if (String(type) == "heartbeat") {
    if (
      nodeNumber >= 1 &&
      nodeNumber <= MAX_NODES
    ) {
      String expectedId =
          makeDefaultNodeId(nodeNumber);

      nodes[index].nodeId =
          expectedId;

      seenNodeNumber[nodeNumber] = true;

      updateNodeActivity(index);

      noteNodeStatusReceived();
    }

    return;
  }


  if (
    String(type) == "identify" ||
    String(type) == "hello" ||
    String(type) == "node"
  ) {
    const char* id =
        doc["id"] |
        doc["node_id"] |
        doc["name"] |
        "";

    if (
      id != nullptr &&
      strlen(id) > 0
    ) {
      nodes[index].nodeId =
          String(id);
    }
    else if (
      nodeNumber >= 1 &&
      nodeNumber <= MAX_NODES
    ) {
      nodes[index].nodeId =
          makeDefaultNodeId(nodeNumber);
    }

    const char* wifiMac =
        doc["wifi_mac"] | "";

    const char* bleMac =
        doc["ble_mac"] | "";

    if (
      wifiMac != nullptr &&
      strlen(wifiMac) > 0
    ) {
      nodes[index].wifiMac =
          String(wifiMac);
    }

    if (
      bleMac != nullptr &&
      strlen(bleMac) > 0
    ) {
      nodes[index].bleMac =
          String(bleMac);
    }

    updateNodeActivity(index);

    noteNodeStatusReceived();

    return;
  }



  if (String(type) == "wifi") {
    wifiObservationCount++;

    String output;
    serializeJson(doc, output);

    sendToApp(output);

    return;
  }



  if (String(type) == "ble") {
    bleObservationCount++;

    String output;
    serializeJson(doc, output);

    sendToApp(output);
    
    return;
  }



  String output;
  serializeJson(doc, output);

  sendToApp(output);
}



void readNodeData(int index) {
  if (
    index < 0 ||
    index >= MAX_NODES
  ) {
    return;
  }

  if (!nodes[index].client.connected()) {
    return;
  }

  while (nodes[index].client.available()) {
    char c =
        nodes[index].client.read();

    if (c == '\n') {
      String line =
          nodes[index].lineBuf;

      nodes[index].lineBuf = "";

      line.trim();

      if (line.length() > 0) {
        processNodeJson(
          index,
          line
        );
      }
    }
    else if (c != '\r') {
      nodes[index].lineBuf += c;

      if (nodes[index].lineBuf.length() > 4096) {
        nodes[index].lineBuf = "";
      }
    }
  }
}



void acceptNodeConnections() {
  WiFiClient incoming =
      nodeServer.available();

  if (!incoming) {
    return;
  }

  int slot =
      findFreeNodeSlot();

  if (slot < 0) {
    Serial.println(
      "[TCP] No free node slots"
    );

    incoming.stop();
    return;
  }

  nodes[slot].client =
      incoming;

  nodes[slot].active =
      true;

  nodes[slot].lastHeartbeatMs =
      millis();

  nodes[slot].lineBuf = "";
  nodes[slot].wifiMac = "";
  nodes[slot].bleMac = "";

  String defaultId =
      makeDefaultNodeId(slot + 1);

  nodes[slot].nodeId =
      defaultId;

  Serial.print(
    "[TCP] NODE CONNECTED: "
  );

  Serial.print(
    nodes[slot].nodeId
  );

  Serial.print(
    " (slot "
  );

  Serial.print(
    slot + 1
  );

  Serial.println(")");
}




void checkNodeConnections() {
  for (int i = 0; i < MAX_NODES; i++) {
    if (
      nodes[i].nodeId.length() > 0 &&
      !nodes[i].client.connected()
    ) {
      if (nodes[i].active) {
        Serial.print(
          "[TCP] NODE DISCONNECTED: "
        );
        Serial.println(
          nodes[i].nodeId
        );
      }

      clearNode(i);
    }
  }
}



void checkHeartbeatTimeouts() {
  const unsigned long HEARTBEAT_TIMEOUT_MS =
      10000;

  const unsigned long now =
      millis();

  for (int i = 0; i < MAX_NODES; i++) {
    if (
      nodes[i].client.connected() &&
      nodes[i].active
    ) {
      if (
        now -
        nodes[i].lastHeartbeatMs >
        HEARTBEAT_TIMEOUT_MS
      ) {
        Serial.print(
          "[NODE] HEARTBEAT TIMEOUT: "
        );

        Serial.println(
          nodes[i].nodeId
        );

        nodes[i].active = false;
      }
    }
  }
}




class ServerCallbacks :
  public NimBLEServerCallbacks {

  void onConnect(
    NimBLEServer* server,
    NimBLEConnInfo& connInfo
  ) override {
    bleConnected = true;
    
    Serial.println();
    Serial.println(
      "[BLE] APP CONNECTED"
    );

    Serial.println(
      "[BLE] SYSTEM ONLINE"
    );

  
    sendStatus();
  }


  void onDisconnect(
    NimBLEServer* server,
    NimBLEConnInfo& connInfo,
    int reason
  ) override {
    bleConnected = false;
    /// STOP NODES
    if (bleConnected == false) {
    StaticJsonDocument<128> doc;
    doc["cmd"] = "stop";

    String json;
    serializeJson(doc, json);

    Serial.print("[CMD] FORWARD STOP -> ");
    Serial.println(json);

    forwardToActiveNodes(json);
    SCAN = 0;
   
    return;
  }
    Serial.println();
    Serial.println(
      "[BLE] APP DISCONNECTED"
    );

    Serial.print(
      "[BLE] Disconnect reason: "
    );

    Serial.println(reason);

    if (bleAdvertising != nullptr) {
      bool restarted =
          bleAdvertising->start();

      Serial.print(
        "[BLE] Advertising restart requested: "
      );

      Serial.println(
        restarted
          ? "SUCCESS"
          : "FAILED"
      );
    }
  }
};




class CommandCallbacks :
  public NimBLECharacteristicCallbacks {

  void onWrite(
    NimBLECharacteristic* characteristic,
    NimBLEConnInfo& connInfo
  ) override {
    std::string value =
        characteristic->getValue();

    if (value.length() == 0) {
      return;
    }

    String command =
        String(value.c_str());

    command.trim();

    Serial.println();
    Serial.println(
      "[BLE] APP -> WAVESHARE"
    );

    Serial.println(command);

    processCommand(command);
  }
};



void checkBleAdvertising() {
  if (bleAdvertising == nullptr) {
    return;
  }

  if (bleConnected) {
    return;
  }

  const unsigned long now =
      millis();

  if (
    now - lastBleAdvertisingCheck <
    5000
  ) {
    return;
  }

  lastBleAdvertisingCheck =
      now;

  bool active =
      bleAdvertising->isAdvertising();

  Serial.print(
    "[BLE] Advertising check: "
  );

  Serial.println(
    active
      ? "ACTIVE"
      : "NOT ACTIVE"
  );

  if (!active) {
    bool restarted =
        bleAdvertising->start();

    Serial.print(
      "[BLE] Advertising restart: "
    );

    Serial.println(
      restarted
        ? "SUCCESS"
        : "FAILED"
    );
  }
}




void setupBLE() {
  Serial.println();
  Serial.println(
    "[BLE] Initializing NimBLE..."
  );

  NimBLEDevice::init(
    BLE_NAME
  );



  NimBLEDevice::setSecurityAuth(
    true,
    false,
    true
  );

  NimBLEDevice::setSecurityIOCap(
    BLE_HS_IO_NO_INPUT_OUTPUT
  );

  bleServer =
      NimBLEDevice::createServer();

  bleServer->setCallbacks(
    new ServerCallbacks()
  );

  bleServer->advertiseOnDisconnect(true);

  NimBLEService* service =
      bleServer->createService(
        BLE_SERVICE_UUID
      );

  commandCharacteristic =
      service->createCharacteristic(
        BLE_COMMAND_UUID,
        NIMBLE_PROPERTY::WRITE |
        NIMBLE_PROPERTY::WRITE_NR
      );

  commandCharacteristic->setCallbacks(
    new CommandCallbacks()
  );

  statusCharacteristic =
      service->createCharacteristic(
        BLE_STATUS_UUID,
        NIMBLE_PROPERTY::READ |
        NIMBLE_PROPERTY::NOTIFY
      );

  service->start();

  bleAdvertising =
      NimBLEDevice::getAdvertising();

  if (bleAdvertising == nullptr) {
    Serial.println(
      "[BLE] ERROR: advertising object unavailable"
    );
    return;
  }

  bool serviceAdded =
      bleAdvertising->addServiceUUID(
        BLE_SERVICE_UUID
      );

  Serial.print(
    "[BLE] Service UUID added: "
  );

  Serial.println(
    serviceAdded
      ? "YES"
      : "NO"
  );

  bool nameSet =
      bleAdvertising->setName(
        BLE_NAME
      );

  Serial.print(
    "[BLE] Device name set: "
  );

  Serial.println(
    nameSet
      ? "YES"
      : "NO"
  );

  bleAdvertising->enableScanResponse(
    true
  );

  Serial.println(
    "[BLE] Scan response: ENABLED"
  );

  bool connectable =
      bleAdvertising->setConnectableMode(
        BLE_GAP_CONN_MODE_UND
      );

  Serial.print(
    "[BLE] Connectable mode: "
  );

  Serial.println(
    connectable
      ? "YES"
      : "NO"
  );

  bool discoverable =
      bleAdvertising->setDiscoverableMode(
        BLE_GAP_DISC_MODE_GEN
      );

  Serial.print(
    "[BLE] Discoverable mode: "
  );

  Serial.println(
    discoverable
      ? "YES"
      : "NO"
  );

  bool started =
      bleAdvertising->start();

  Serial.print(
    "[BLE] Advertising start result: "
  );

  Serial.println(
    started
      ? "SUCCESS"
      : "FAILED"
  );

  Serial.print(
    "[BLE] Advertising active: "
  );

  Serial.println(
    bleAdvertising->isAdvertising()
      ? "YES"
      : "NO"
  );

  Serial.print(
    "[BLE] Device name: "
  );

  Serial.println(
    BLE_NAME
  );

  Serial.print(
    "[BLE] Service UUID: "
  );

  Serial.println(
    BLE_SERVICE_UUID
  );

  Serial.println();
  Serial.println(
    "[BLE] WD-Hub advertising"
  );
}



void setup() {
  Serial.begin(115200);

  delay(1000);

  Serial.println();
  Serial.println();
  Serial.println(
    "=========================================="
  );
  Serial.println(
    " DEAD PACKET SOCIETY"
  );
  Serial.println(
    " WAVESHARE ESP32-C5 LCD CONTROLLER"
  );
  Serial.println(
    "=========================================="
  );

 

  for (int i = 0; i < MAX_NODES; i++) {
    clearNode(i);
  }

  

 
  Serial.println();
  Serial.println(
    "[WiFi] Starting WD-Hub AP..."
  );

  WiFi.mode(WIFI_AP);

  WiFi.softAP(
    AP_SSID,
    AP_PASSWORD,
    1,
    0,
    AP_MAX_CONNECTIONS
  );

  delay(500);

  Serial.print(
    "[WiFi] SSID: "
  );

  Serial.println(
    AP_SSID
  );

  Serial.print(
    "[WiFi] IP: "
  );

  Serial.println(
    WiFi.softAPIP()
  );

  Serial.print(
    "[WiFi] BSSID: "
  );

  Serial.println(
    getControllerBssid()
  );


  nodeServer.begin();

  nodeServer.setNoDelay(true);

  Serial.print(
    "[TCP] Server started on port "
  );

  Serial.println(
    TCP_PORT
  );



  setupBLE();

  Serial.println();
  Serial.println(
    "=========================================="
  );
  Serial.println(
    " SYSTEM ONLINE"
  );
  Serial.println(
    "=========================================="
  );

 
}




void loop() {
  

  acceptNodeConnections();



  for (int i = 0; i < MAX_NODES; i++) {
    if (nodes[i].client.connected()) {
      readNodeData(i);
    }
  }

 

  checkNodeConnections();


  checkHeartbeatTimeouts();



  sendStatusIfDue();


  checkBleAdvertising();


  

  delay(2);
}
