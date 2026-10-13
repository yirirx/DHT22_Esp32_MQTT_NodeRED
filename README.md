# ESP32 DHT22 IoT Telemetry and Reliability Monitor

An ESP32 + DHT22 temperature/humidity monitoring project that publishes sensor data over MQTT and visualizes it with Node-RED. This repository is a **modified fork** of [Hariharnath-Paduchuru/DHT22_Esp32_MQTT_NodeRED](https://github.com/Hariharnath-Paduchuru/DHT22_Esp32_MQTT_NodeRED), extended with reliability-oriented telemetry and a test plan.

> **Project status:** firmware and documentation have been updated, but end-to-end hardware/broker/dashboard testing has not yet been documented. Do not treat planned tests as completed results.

## What the system does

- Reads temperature and relative humidity from a DHT22 sensor.
- Publishes legacy MQTT topics to remain compatible with the original dashboard:
  - `iotfrontier/temperature`
  - `iotfrontier/humidity`
- Publishes structured JSON telemetry to `iotfrontier/telemetry`, including a sequence number, uptime, Wi-Fi RSSI, and valid/invalid reading counters.
- Publishes device-health messages to `iotfrontier/health`.
- Publishes retained online/offline status to `iotfrontier/status` using MQTT Last Will.
- Uses scheduled Wi-Fi/MQTT reconnect attempts rather than an indefinite blocking reconnect loop.
- Validates sensor readings before publishing them.

## Architecture

```text
DHT22 sensor
    |
    v
ESP32 firmware ---- Wi-Fi ---- MQTT broker
                                  |
                  +---------------+----------------+
                  |               |                |
                  v               v                v
             temperature       humidity        JSON telemetry
                  |               |                |
                  +---------------+----------------+
                                  |
                                  v
                           Node-RED dashboard
```

See [docs/architecture.md](docs/architecture.md) and the original project's wiring/flow images included in this repository.

## Hardware and simulation

- ESP32 development board (or a compatible Wokwi ESP32 simulation)
- DHT22 sensor
- Jumper wires and breadboard for physical testing

The firmware currently uses **GPIO 15** for the DHT22 data line. Match the wiring to `DHT_PIN` in the firmware before running it. GPIO availability differs by ESP32 board; check your board's pinout.

## Software dependencies

- Arduino IDE or Arduino CLI with an ESP32 board package
- `DHTesp` library
- `PubSubClient` library
- ESP32 Arduino core (`WiFi.h` is provided by the core)
- Node-RED and a dashboard UI package compatible with your imported flow

## Run it

1. Open `Wokwi_ESP32_DHT22_code.ino` in Arduino IDE or a compatible Wokwi project.
2. Set the correct board and verify `DHT_PIN` matches your sensor wiring.
3. Configure `WIFI_SSID`, `WIFI_PASSWORD`, `MQTT_HOST`, and `MQTT_PORT` in the `Config` section.
4. Install the libraries listed above and compile/upload the firmware.
5. Import the Node-RED flow from [node-red/flows.json](node-red/flows.json) if present, then deploy it.
6. Open the Node-RED dashboard and confirm the temperature/humidity widgets update.
7. Subscribe to `iotfrontier/telemetry`, `iotfrontier/health`, and `iotfrontier/status` to inspect diagnostic messages.

### MQTT topic reference

| Topic | Payload | Purpose |
|---|---|---|
| `iotfrontier/temperature` | Numeric string, °C | Backward compatibility |
| `iotfrontier/humidity` | Numeric string, %RH | Backward compatibility |
| `iotfrontier/telemetry` | JSON | Timestamp-free structured telemetry and counters |
| `iotfrontier/health` | JSON | Sensor and device health summary |
| `iotfrontier/status` | `online` / `offline` | Retained MQTT presence status |

## Reliability evaluation

The goal of the extension is to evaluate observable behavior rather than claim reliability without evidence. Use the procedure in [docs/test-plan.md](docs/test-plan.md) and record actual results in [docs/test-results.md](docs/test-results.md).

Suggested metrics:
- valid samples / total samples
- invalid sensor readings detected
- MQTT reconnect attempts
- time from broker/network restoration to the next successfully received telemetry message
- sequence gaps observed by the subscriber

No performance numbers are claimed until tests are run and recorded.

## Security and privacy note

The default public broker configuration is for demonstrations only. Public MQTT brokers are unencrypted and topics may be visible to other users. Use a private broker with authentication and TLS for anything sensitive. Never commit Wi-Fi passwords, tokens, or private broker credentials.

## Attribution

This repository is derived from the original project by [Hariharnath Paduchuru](https://github.com/Hariharnath-Paduchuru/DHT22_Esp32_MQTT_NodeRED). The upstream repository includes an Apache License 2.0; the license file is retained. Modifications in this fork focus on input validation, reconnect scheduling, structured telemetry, device-health reporting, and evaluation documentation. Original assets and the original dashboard concept remain credited to the upstream author.

## Next steps

- [ ] Verify the firmware compiles for the selected ESP32 board.
- [ ] Verify GPIO wiring in Wokwi or on hardware.
- [ ] Run normal-operation and disconnect/reconnect tests.
- [ ] Capture a real dashboard screenshot from this modified version.
- [ ] Record measurements in the test-results file.
