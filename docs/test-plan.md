# Reliability Test Plan

This plan defines repeatable checks for the modified firmware. Mark a test as passed only after observing and recording the result.

## Test setup

- ESP32 board or Wokwi ESP32 simulation
- DHT22 sensor model or physical sensor wired to the configured GPIO
- Node-RED with the flow in `node-red/flows.json`
- MQTT subscriber capable of recording topic, payload, and receive time
- A test log with firmware revision, board/core version, library versions, and test date

## Cases

| ID | Scenario | Procedure | Evidence to record |
|---|---|---|---|
| T01 | Normal sampling | Run for at least 10 minutes with a stable sensor input | Total received samples, missing sequence numbers, dashboard updates |
| T02 | Invalid sensor input | Disconnect or simulate an invalid DHT22 reading where supported | Serial log and health messages; confirm invalid count increases and sample is not published as valid |
| T03 | MQTT outage | Stop or block broker access, wait, then restore it | Disconnect log, reconnect attempts, time to first telemetry after restoration |
| T04 | Wi-Fi outage | Disable the simulation/network connection, then restore it | Wi-Fi retry logs and time to resumed telemetry |
| T05 | Unexpected device disconnect | Stop the device without a graceful MQTT disconnect | Whether subscribers receive retained `offline` status via Last Will |
| T06 | Backward compatibility | Subscribe to the original temperature and humidity topics | Confirm numeric-string payloads continue to arrive |
| T07 | Long-run operation | Run for at least 1 hour | Samples received, invalid readings, reconnect counts, sequence gaps |

## Metrics

- Valid sample ratio = valid samples / expected sample opportunities.
- Invalid-reading detection count.
- MQTT reconnect attempts.
- Recovery time = elapsed time between restoring network/broker access and receiving the next valid telemetry message.
- Sequence gaps = missing sequence numbers observed by the subscriber.

Use the same board, sensor, sampling interval, broker, and test duration when comparing a baseline build with the modified build. Record any differences in conditions. Do not claim statistically significant improvement from a single run.
