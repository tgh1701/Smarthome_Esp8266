#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266HTTPClient.h>

// PC
// const char * ssid = "Hien Truong";
// const char * password = "0926886199";
// const char * mqtt_server = "192.168.0.119";

// MAC
const char * ssid = "HienTruong";
const char * password = "0926886199";
const char * mqtt_server = "172.20.10.2";

WiFiClient espClient;
PubSubClient client(espClient);
#define MSG_BUFFER_SIZE 50
char msg[MSG_BUFFER_SIZE];

const size_t JSON_BUFFER_SIZE = 512;
char jsonString[JSON_BUFFER_SIZE];

void setup_wifi() {
  delay(10);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  randomSeed(micros());
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char * topic, byte * payload, unsigned int length) {
  payload[length] = '\0';
  String payloadString = "";
  for (int i = 0; length > 0; i++, length--) {
    payloadString += (char) payload[i];
  }
  if (payloadString.length() > 0) {
    if (strncmp(topic, "buttonsState/", 13) == 0) {
      handleButtonsState(topic, payloadString);
    } else if (strcmp(topic, "autoMode/water/status") == 0) {
      handleAutoModeWaterStatus(payloadString);
    } else if (strcmp(topic, "autoMode/water/setTarget") == 0) {
      handleAutoModeWaterSetTarget(payloadString);
    } else if (strcmp(topic, "autoMode/rain/status") == 0) {
      handleAutoModeRainStatus(payloadString);
    } else if (strcmp(topic, "finger/fingerControl") == 0) {
      handleFingerControl(payloadString);
    } else if (strcmp(topic, "finger/fingerId") == 0) {
      handleFingerId(payloadString);
    } else if (strcmp(topic, "finger/fingerName") == 0) {
      handleFingerName(payloadString);
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe("buttonsState/#");
      client.subscribe("autoMode/#");
      client.subscribe("finger/#");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  client.subscribe("buttonsState/#");
  client.subscribe("autoMode/#");
  client.subscribe("finger/#");
}

void handleButtonsState(char * topic, String payloadString) {
  DynamicJsonDocument jsonDoc(JSON_BUFFER_SIZE);
  JsonObject response = jsonDoc.createNestedObject("buttons");
  const char * room = strchr(topic, '/') + 1;
  int currentButtonState = payloadString.toInt();
  response[room] = currentButtonState;
  serializeJson(jsonDoc, jsonString);
  Serial.println(jsonString);
}

void handleAutoModeWaterStatus(const String & payloadString) {
  DynamicJsonDocument jsonDoc(JSON_BUFFER_SIZE);
  JsonObject response = jsonDoc.createNestedObject("autoMode");
  JsonObject water = response.createNestedObject("water");
  water["status"] = payloadString.toInt();
  serializeJson(jsonDoc, jsonString);
  Serial.println(jsonString);
}

void handleAutoModeWaterSetTarget(const String & payloadString) {
  DynamicJsonDocument jsonDoc(JSON_BUFFER_SIZE);
  JsonObject response = jsonDoc.createNestedObject("autoMode");
  JsonObject water = response.createNestedObject("water");
  water["setTarget"] = payloadString.toInt();
  serializeJson(jsonDoc, jsonString);
  Serial.println(jsonString);
}

void handleAutoModeRainStatus(const String & payloadString) {
  DynamicJsonDocument jsonDoc(JSON_BUFFER_SIZE);
  JsonObject response = jsonDoc.createNestedObject("autoMode");
  JsonObject rain = response.createNestedObject("rain");
  rain["status"] = payloadString.toInt();
  serializeJson(jsonDoc, jsonString);
  Serial.println(jsonString);
}

void handleFingerControl(const String & payloadString) {
  int fingerControlValue = payloadString.toInt();
  DynamicJsonDocument jsonDoc(JSON_BUFFER_SIZE);
  JsonObject response = jsonDoc.to < JsonObject > ();
  response["fingerControl"] = fingerControlValue;
  serializeJson(response, jsonString);
  Serial.println(jsonString);
}

void handleFingerId(const String & payloadString) {
  int fingerIdValue = payloadString.toInt();
  DynamicJsonDocument jsonDoc(JSON_BUFFER_SIZE);
  JsonObject response = jsonDoc.to < JsonObject > ();
  response["fingerId"] = fingerIdValue;
  serializeJson(response, jsonString);
  Serial.println(jsonString);
}

void handleFingerName(const String & payloadString) {
  DynamicJsonDocument jsonDoc(JSON_BUFFER_SIZE);
  JsonObject response = jsonDoc.to < JsonObject > ();
  response["fingerName"] = payloadString;
  String jsonString;
  serializeJson(response, jsonString);
  Serial.println(jsonString);
}

void handleSensorsData(JsonObject & sensorData) {
  float mq2Value = sensorData["MQ2"];
  float mq5Value = sensorData["MQ5"];
  float soilMoisture = sensorData["Soil"];
  float temperature = sensorData["Temp"];
  float humidity = sensorData["Hum"];
  int rainDigital = sensorData["Rain"];
  int fireDigital = sensorData["Fire"];
  String mqttPayload = "{\"MQ2\":" + String(mq2Value) +
    ",\"MQ5\":" + String(mq5Value) +
    ",\"Soil\":" + String(soilMoisture) +
    ",\"Temp\":" + String(temperature) +
    ",\"Hum\":" + String(humidity) +
    ",\"Rain\":" + String(rainDigital) +
    ",\"Fire\":" + String(fireDigital) + "}";
  client.publish("sensorsData", mqttPayload.c_str());
}

void handleButtonsData(JsonObject & buttonsData) {
  for (JsonPair button: buttonsData) {
    String buttonName = button.key().c_str();
    int buttonState = button.value().as < int > ();
    String topic = "buttonsState/" + buttonName;
    client.publish(topic.c_str(), String(buttonState).c_str());
  }
}

void handleFingerScanData(JsonObject & fingerScanData) {
  int id = fingerScanData["fingerScan"][0]["id"].as < int > ();
  String mqttPayload = "{\"id\":" + String(id) + "}";
  client.publish("finger/fingerScan", mqttPayload.c_str());
}

void handleFingerEnrollData(JsonObject & fingerEnrollData) {
  int id = fingerEnrollData["fingerEnroll"][0]["id"].as < int > ();
  String name = fingerEnrollData["fingerEnroll"][0]["fingerName"].as < String > ();
  String data = fingerEnrollData["fingerEnroll"][0]["data"].as < String > ();
  String mqttPayload = "{\"id\":" + String(id) + ",\"fingerName\":\"" + name + "\",\"data\":\"" + data + "\"}";
  client.publish("finger/fingerEnroll", mqttPayload.c_str());
}

void handleFingerDeleteData(JsonObject & fingerDeleteData) {
  int id = fingerDeleteData["fingerDelete"][0]["id"].as < int > ();
  String mqttPayload = "{\"id\":" + String(id) + "}";
  client.publish("finger/fingerDelete", mqttPayload.c_str());
}

void readDataFromMega() {
  if (Serial.available()) {
    Serial.readBytesUntil('\n', jsonString, JSON_BUFFER_SIZE);
    DynamicJsonDocument jsonDoc(JSON_BUFFER_SIZE);
    DeserializationError error = deserializeJson(jsonDoc, jsonString);
    if (error) {} else if (jsonDoc.containsKey("sensors")) {
      JsonObject sensorData = jsonDoc["sensors"];
      handleSensorsData(sensorData);
    } else if (jsonDoc.containsKey("buttons")) {
      JsonObject buttonsData = jsonDoc["buttons"];
      handleButtonsData(buttonsData);
    } else if (jsonDoc.containsKey("finger")) {
      JsonObject fingerData = jsonDoc["finger"];
      if (fingerData.containsKey("fingerScan")) {
        handleFingerScanData(fingerData);
      } else if (fingerData.containsKey("fingerEnroll")) {
        handleFingerEnrollData(fingerData);
      } else if (fingerData.containsKey("fingerDelete")) {
        handleFingerDeleteData(fingerData);
      }
    }
  }
}

void loop() {
  readDataFromMega();
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}