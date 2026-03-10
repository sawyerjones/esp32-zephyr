**Board:** esp32_devkitc/esp32/procpu
**Sensor** BMP280

## Development Environment
Need to activate Zephyr before any development

run `cd ~` and `source activate-zephyr` before any development

Build cmd: `west build -p always -b esp32_devkitc/esp32/procpu`

i2c wiring check through monitor: `i2c scan i2c@3ff53000`, make sure `CONFIG_SHELL` configured in prj.conf

rpi: activate venv, run:
- `mosquitto_sub -h localhost -t "#" -v` for json data
- `python3 subscriber.py` for SQLite updates



## Hardware

### [Board](espressif.com/documentation/esp32_datasheet_en.pdf#cd-peri-pin-config)
SCL/SDA can be mapped to any GPIO pins (21, 22, 23)

SCL --> 22

SDA --> 21

CSB --> 3.3v

SDD --> GND

**MAC**: 20:E7:C8:EC:D4:D0

### [Sensor](https://cdn.shopify.com/s/files/1/0837/4340/8415/files/BMP280-Datasheet.pdf?v=1768632341)
From top to bottom: VCC, GND, SCL, SDA, CSB, SDD


## Software

### Relationships
ESP -> JSON (over wifi)-> MQTT -> Dash


### [Mosquitto](https://docs.zephyrproject.org/latest/connectivity/networking/api/mqtt.html)