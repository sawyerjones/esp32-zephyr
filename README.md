# ESP32 Environment Monitor (Zephyr RTOS)

A multi-threaded IoT sensor node built on Zephyr RTOS. Reads temperature and pressure from a BMP280 over I²C, publishes readings to a Raspberry Pi via WiFi/MQTT, and logs data to SQLite with a web dashboard.

---

## Architecture

```
┌──────────────────────────────┐        MQTT / WiFi        ┌─────────────────────────┐
│    ESP32-DevKitC (Zephyr)    │                           │      Raspberry Pi       │
│                              │  publish: sensor/esp32 →  │                         │
│  sensor thread (priority 7)  │                           │  Mosquitto MQTT broker  │
│    k_timer → k_work          │                           │  subscriber.py → SQLite │
│    BMP280 via I²C            │                           │  Flask dashboard        │
│       ↓ k_msgq               │                           │                         │
│  mqtt thread  (priority 6)   │                           │                         │
│    zsock_poll + mqtt_live    │                           │                         │
│    mqtt_publish_sensor       │                           │                         │
│                              │                           │                         │
│  main (watchdog feed)        │                           │                         │
└──────────────────────────────┘                           └─────────────────────────┘
```

---

## Hardware

| Component | Details |
|-----------|---------|
| MCU | ESP32-DevKitC (WROOM-32E) |
| Sensor | BMP280 breakout board |
| Interface | I²C — SDA → GPIO21, SCL → GPIO22 |
| Backend | Raspberry Pi (any model) |

### BMP280 Wiring

```
BMP280 Pin    ESP32 Pin
----------    ---------
VCC           3.3V
GND           GND
SCL           GPIO22
SDA           GPIO21
CSB           3.3V      ← must be pulled high to force I²C mode
SDD           GND       ← sets I²C address to 0x76
```

> CSB must be tied to 3.3V. Leaving it floating causes the sensor to not respond on I²C.

---

## Firmware

### Project structure

```
esp32-zephyr/
├── CMakeLists.txt
├── prj.conf                              # Kconfig: WiFi, MQTT, sensor, watchdog
├── boards/
│   └── esp32_devkitc_esp32_procpu.overlay  # Devicetree: I²C bus + BMP280 node
└── src/
    ├── main.c        # watchdog init, wifi + mqtt connect, main feed loop
    ├── sensor.c      # BMP280 sampling thread (k_timer + k_work + k_msgq)
    ├── sensor.h
    ├── mqtt.c        # MQTT publish thread (zsock_poll + mqtt_live + k_msgq drain)
    ├── mqtt.h
    ├── wifi.c        # WiFi connect with semaphore-gated DHCP wait
    ├── wifi.h
    └── secrets.h     # WiFi SSID/password, broker IP/port (not committed)
```

### Threading model

| Thread | Priority | Role |
|--------|----------|------|
| `sensor_tid` | 7 | Starts `k_timer`, sleeps forever — work queue does sampling |
| `mqtt_tid` | 6 | Polls MQTT socket, drives keepalive, drains `sensor_msgq` |
| `main` | default | Feeds watchdog every 5 seconds |

Data flows from the sensor thread to the MQTT thread via `K_MSGQ_DEFINE(sensor_msgq, ...)` — a 4-slot message queue. If the MQTT thread is backed up, readings are dropped rather than blocking the work queue.

`K_SEM_DEFINE(mqtt_ready, 0, 1)` prevents the MQTT thread from initializing its `zsock_pollfd` before `mqtt_app_connect()` has run and the socket fd is valid.

`K_SEM_DEFINE(mqtt_sem, 1, 1)` guards concurrent access to the `mqtt_client` handle between `mqtt_publish_sensor()` and the poll/keepalive loop.

### Watchdog

A 10-second hardware watchdog is configured via Zephyr's WDT driver (`wdt0`). `main()` feeds it every 5 seconds. If any subsystem deadlocks and stalls `main`, the ESP32 resets automatically. `WDT_OPT_PAUSE_HALTED_BY_DBG` prevents resets during debug sessions.

### Devicetree overlay

The BMP280 is declared at I²C address `0x76` using `compatible = "bosch,bme280"`. Zephyr's BMP280 driver is gated behind `DT_HAS_BOSCH_BME280_ENABLED` — the `bosch,bme280` compatible string and `CONFIG_BME280=y` are required regardless of whether the physical chip is a BMP280 or BME280.

---

## Build & Flash

### Prerequisites

- [Zephyr SDK](https://docs.zephyrproject.org/latest/develop/getting_started/index.html) installed
- `west` meta-tool installed
- Zephyr environment activated

### Setup

```bash
cd ~
# activate the zephyr venv
cd ~/workspace/esp32-zephyr
```

### Build

```bash
west build -p always -b esp32_devkitc/esp32/procpu
```

### Flash

```bash
west flash
```

### Monitor serial output

```bash
west espressif monitor
```

Expected output after successful boot:
```
[WDT] starting setup...
[ESP32] starting setup...
[WIFI] connecting to: <SSID>
[WIFI] connected
[WIFI] DHCP started
[WIFI] ESP32 IP: 192.168.x.x
[MQTT] connected to broker
[MQTT] published: {"temp":23.45,"pressure":1013.25}
```

### I²C debug

To verify the sensor is wired correctly before flashing the full app, enable the shell in `prj.conf`:

```kconfig
CONFIG_SHELL=y
CONFIG_I2C_SHELL=y
```

Then in the monitor:
```
i2c scan i2c@3ff53000
```

The BMP280 should appear at address `0x76`.

---

## Raspberry Pi Setup

### Mosquitto MQTT broker

```bash
sudo apt install mosquitto mosquitto-clients
sudo systemctl enable mosquitto
sudo systemctl start mosquitto
```

Monitor all incoming MQTT traffic:
```bash
mosquitto_sub -h localhost -t "#" -v
```

### Python subscriber (MQTT → SQLite)

```bash
cd ~/esp32-zephyr/pi
source ../bin/activate   # activate virtualenv
pip install paho-mqtt
python3 subscriber.py
```

`subscriber.py` creates `sensor_data.db` on first run and inserts a row for each incoming MQTT message.

### Dashboard (Flask)

```bash
cd ~/esp32-zephyr/dashboard
pip install flask flask-cors
python3 app.py
```

Access at `http://<pi-ip>:5000` from any device on the local network.

---

## Configuration

Create `src/secrets.h` and fill in your values before building:

```c
#define WIFI_SSID   "your_ssid"
#define WIFI_PW     "your_password"
#define BROKER_ADDR "192.168.x.x"   // Raspberry Pi LAN IP
#define BROKER_PORT 1883
```

> `secrets.h` is gitignored. Never commit credentials.

---