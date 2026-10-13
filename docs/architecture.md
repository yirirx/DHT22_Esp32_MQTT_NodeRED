# System Architecture

## Data path

1. The DHT22 sensor provides temperature and relative-humidity measurements to the ESP32.
2. The firmware validates the readings for finite numeric values and plausible DHT22 operating ranges.
3. Valid samples are published to the two legacy numeric MQTT topics and a structured JSON telemetry topic.
4. Health messages report sample counters and MQTT reconnect attempts. A retained status topic reports online/offline presence through MQTT Last Will.
5. Node-RED subscribes to the MQTT topics and presents live gauges, charts, and diagnostic messages.

## Topics

| Topic | Payload | Notes |
|---|---|---|
| `iotfrontier/temperature` | Numeric string | Legacy dashboard compatibility; degrees Celsius |
| `iotfrontier/humidity` | Numeric string | Legacy dashboard compatibility; relative humidity percent |
| `iotfrontier/telemetry` | JSON object | Sequence, values, uptime, RSSI, reading counters |
| `iotfrontier/health` | JSON object | Sensor health and reconnect diagnostics |
| `iotfrontier/status` | `online` or `offline` | Retained; offline is the MQTT Last Will |

## Reliability design choices

- **Non-blocking retry scheduling:** Wi-Fi and MQTT reconnection attempts are scheduled instead of waiting indefinitely in a reconnect loop.
- **Input validation:** non-finite and out-of-range measurements are not published as valid samples.
- **Observability:** sequence numbers and counters make gaps and invalid readings easier to detect.
- **Backward compatibility:** existing temperature and humidity topics remain available for the original dashboard.
- **Liveness:** MQTT Last Will can notify subscribers when the client disconnects unexpectedly.

## Limitations

The current demo uses a public, unencrypted MQTT broker and static demonstration configuration. Do not send sensitive information through it. Reliability claims require repeated tests and recorded measurements; the design alone is not proof of improved reliability.
