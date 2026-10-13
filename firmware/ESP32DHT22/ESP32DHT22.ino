/*
 * Reliability-focused extension of the original ESP32 + DHT22 + MQTT demo.
 *
 * Upstream project: https://github.com/Hariharnath-Paduchuru/DHT22_Esp32_MQTT_NodeRED
 * Original code is distributed under Apache-2.0. This file contains modifications
 * for telemetry, input validation, reconnect scheduling, and device-health reporting.
 *
 * IMPORTANT:
 * - Change DHT_PIN to match your wiring (GPIO 15 is used as a practical default).
 * - Configure Wi-Fi/MQTT settings for your environment.
 * - Public MQTT brokers are for demos only; use authenticated TLS brokers for real deployments.
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <DHTesp.h>
#include <math.h>

namespace Config {
  constexpr uint8_t DHT_PIN = 15;
  constexpr uint32_t SAMPLE_INTERVAL_MS = 2000;
  constexpr uint32_t WIFI_RETRY_INTERVAL_MS = 10000;
  constexpr uint32_t MQTT_RETRY_INTERVAL_MS = 5000;
  constexpr uint16_t MQTT_PORT = 1883;
  constexpr size_t MQTT_BUFFER_SIZE = 384;

  const char* WIFI_SSID = "Wokwi-GUEST";
  const char* WIFI_PASSWORD = "";
  const char* MQTT_HOST = "broker.hivemq.com";

  // Keep the original topics so the original Node-RED dashboard remains compatible.
  const char* TEMPERATURE_TOPIC = "iotfrontier/temperature";
  const char* HUMIDITY_TOPIC = "iotfrontier/humidity";
  const char* TELEMETRY_TOPIC = "iotfrontier/telemetry";
  const char* HEALTH_TOPIC = "iotfrontier/health";
  const char* STATUS_TOPIC = "iotfrontier/status";
  const char* COMMAND_TOPIC = "iotfrontier/mqtt";
}

DHTesp dhtSensor;
WiFiClient networkClient;
PubSubClient mqtt(networkClient);

uint32_t lastSampleMs = 0;
uint32_t lastWiFiAttemptMs = 0;
uint32_t lastMqttAttemptMs = 0;
uint32_t sequenceNumber = 0;
uint32_t validReadings = 0;
uint32_t invalidReadings = 0;
uint32_t mqttReconnectAttempts = 0;
bool previousWiFiConnected = false;
bool previousMqttConnected = false;

bool isValidReading(float temperature, float humidity) {
  return isfinite(temperature) && isfinite(humidity) &&
         temperature >= -40.0f && temperature <= 80.0f &&
         humidity >= 0.0f && humidity <= 100.0f;
}

void startWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(Config::WIFI_SSID, Config::WIFI_PASSWORD);
  lastWiFiAttemptMs = millis();
  Serial.printf("[WiFi] Starting connection to %s\n", Config::WIFI_SSID);
}

void maintainWiFi() {
  const uint32_t now = millis();
  const bool connected = WiFi.status() == WL_CONNECTED;

  if (connected && !previousWiFiConnected) {
    Serial.printf("[WiFi] Connected; IP=%s RSSI=%d dBm\n",
                  WiFi.localIP().toString().c_str(), WiFi.RSSI());
  } else if (!connected && previousWiFiConnected) {
    Serial.println("[WiFi] Connection lost");
  }
  previousWiFiConnected = connected;

  if (!connected && now - lastWiFiAttemptMs >= Config::WIFI_RETRY_INTERVAL_MS) {
    lastWiFiAttemptMs = now;
    Serial.println("[WiFi] Retrying connection...");
    WiFi.disconnect();
    WiFi.begin(Config::WIFI_SSID, Config::WIFI_PASSWORD);
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.printf("[MQTT] Message on %s (%u bytes)\n", topic, length);
  // Preserve the original demo's command subscription for compatibility.
  // Commands are logged only; no hardware action is performed by this extension.
}

bool publishText(const char* topic, const char* payload, bool retained = false) {
  if (!mqtt.connected()) return false;
  const bool ok = mqtt.publish(topic, payload, retained);
  if (!ok) Serial.printf("[MQTT] Publish failed: %s\n", topic);
  return ok;
}

void maintainMQTT() {
  if (WiFi.status() != WL_CONNECTED) return;

  const uint32_t now = millis();
  if (mqtt.connected()) {
    if (!previousMqttConnected) {
      Serial.println("[MQTT] Connected");
      publishText(Config::STATUS_TOPIC, "online", true);
    }
    previousMqttConnected = true;
    return;
  }

  if (previousMqttConnected) {
    Serial.println("[MQTT] Connection lost");
    previousMqttConnected = false;
  }

  if (now - lastMqttAttemptMs < Config::MQTT_RETRY_INTERVAL_MS) return;
  lastMqttAttemptMs = now;
  mqttReconnectAttempts++;

  char clientId[40];
  snprintf(clientId, sizeof(clientId), "esp32-dht22-%08lx",
           static_cast<unsigned long>(ESP.getEfuseMac() & 0xFFFFFFFF));

  Serial.printf("[MQTT] Connecting to %s:%u (attempt %lu)\n",
                Config::MQTT_HOST, Config::MQTT_PORT,
                static_cast<unsigned long>(mqttReconnectAttempts));

  if (mqtt.connect(clientId, Config::STATUS_TOPIC, 1, true, "offline")) {
    previousMqttConnected = true;
    Serial.println("[MQTT] Connected");
    publishText(Config::STATUS_TOPIC, "online", true);
    mqtt.subscribe(Config::COMMAND_TOPIC);
  } else {
    Serial.printf("[MQTT] Connect failed, state=%d\n", mqtt.state());
  }
}

void publishReading(float temperature, float humidity) {
  sequenceNumber++;
  validReadings++;

  char temperatureText[16];
  char humidityText[16];
  snprintf(temperatureText, sizeof(temperatureText), "%.2f", temperature);
  snprintf(humidityText, sizeof(humidityText), "%.1f", humidity);

  // Backward-compatible topics consumed by the original Node-RED dashboard.
  publishText(Config::TEMPERATURE_TOPIC, temperatureText);
  publishText(Config::HUMIDITY_TOPIC, humidityText);

  char json[Config::MQTT_BUFFER_SIZE];
  const long rssi = WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0;
  const unsigned long uptimeSeconds = millis() / 1000UL;
  snprintf(json, sizeof(json),
           "{\"sequence\":%lu,\"temperature_c\":%.2f,\"humidity_pct\":%.1f,"
           "\"uptime_s\":%lu,\"wifi_rssi_dbm\":%ld,"
           "\"valid_readings\":%lu,\"invalid_readings\":%lu}",
           static_cast<unsigned long>(sequenceNumber), temperature, humidity,
           uptimeSeconds, rssi, static_cast<unsigned long>(validReadings),
           static_cast<unsigned long>(invalidReadings));

  publishText(Config::TELEMETRY_TOPIC, json);

  char health[Config::MQTT_BUFFER_SIZE];
  snprintf(health, sizeof(health),
           "{\"status\":\"ok\",\"sequence\":%lu,\"valid_readings\":%lu,"
           "\"invalid_readings\":%lu,\"mqtt_reconnect_attempts\":%lu}",
           static_cast<unsigned long>(sequenceNumber),
           static_cast<unsigned long>(validReadings),
           static_cast<unsigned long>(invalidReadings),
           static_cast<unsigned long>(mqttReconnectAttempts));
  publishText(Config::HEALTH_TOPIC, health);

  Serial.printf("[Sample %lu] T=%.2f C, RH=%.1f%%, RSSI=%ld dBm\n",
                static_cast<unsigned long>(sequenceNumber), temperature,
                humidity, rssi);
}

void setup() {
  Serial.begin(115200);
  delay(100);

  dhtSensor.setup(Config::DHT_PIN, DHTesp::DHT22);
  mqtt.setServer(Config::MQTT_HOST, Config::MQTT_PORT);
  mqtt.setCallback(mqttCallback);
  mqtt.setBufferSize(Config::MQTT_BUFFER_SIZE);
  mqtt.setKeepAlive(20);
  mqtt.setSocketTimeout(3);

  startWiFi();
  Serial.println("[System] ESP32 DHT22 reliability telemetry starting");
}

void loop() {
  maintainWiFi();
  maintainMQTT();

  if (mqtt.connected()) mqtt.loop();

  const uint32_t now = millis();
  if (now - lastSampleMs >= Config::SAMPLE_INTERVAL_MS) {
    lastSampleMs = now;

    const TempAndHumidity reading = dhtSensor.getTempAndHumidity();
    if (!isValidReading(reading.temperature, reading.humidity)) {
      invalidReadings++;
      Serial.printf("[Sensor] Invalid reading; total invalid=%lu\n",
                    static_cast<unsigned long>(invalidReadings));
      char health[Config::MQTT_BUFFER_SIZE];
      snprintf(health, sizeof(health),
               "{\"status\":\"sensor_error\",\"valid_readings\":%lu,"
               "\"invalid_readings\":%lu}",
               static_cast<unsigned long>(validReadings),
               static_cast<unsigned long>(invalidReadings));
      publishText(Config::HEALTH_TOPIC, health);
      return;
    }

    publishReading(reading.temperature, reading.humidity);
  }

  delay(5); // Yield to the ESP32 scheduler without blocking for reconnect delays.
}
