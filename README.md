# Tunduque – Telematics for Heavy Machinery

GPS tracker, hour meter, water temperature and oil pressure monitor for machinery, developed at CEGA Electrónica.

Built by a team of two: I co-developed the firmware and hardware and know the whole system end to end.

## Features
- GPS position (NMEA parser) and tilt from an IMU
- Engine hour meter, water temperature (NTC) and oil pressure through the ADC
- Data sent over Wi-Fi with MQTT over TLS
- Over-the-air firmware updates (OTA)
- Configuration web page served by the device: Wi-Fi network and MQTT settings, stored in NVS
- Deep sleep to save power when the machine is off

## Hardware
Custom PCB around the ESP32-S3-WROOM-1. The schematic is in [`docs/schematic-v2.pdf`](docs/schematic-v2.pdf).

## Firmware
C on ESP-IDF, about 4,500 lines, split into one module per subsystem:

| Module | Role |
|---|---|
| `adc` | Analog sensors (temperature, pressure) |
| `nmea_parser` | GPS sentences |
| `imu` | Inclination |
| `hourmeter` | Engine hours, kept across power cycles |
| `mqtt` | Telemetry and remote commands |
| `wifi`, `dns`, `http` | Connection and configuration portal |
| `ota` | Remote firmware updates |
| `nvs` | Persistent settings |
| `sleep` | Power management |

Build with ESP-IDF: `idf.py set-target esp32s3 && idf.py build`. Broker address and credentials in this repo are placeholders (`CHANGE_ME`); set real ones through the configuration page.
