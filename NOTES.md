**Board:** esp32_devkitc/esp32/procpu
**Sensor** BMP280

## Development Environment
Need to activate Zephyr before any development

run `cd ~` and `source activate-zephyr` before any development

## Hardware

### [Board](espressif.com/documentation/esp32_datasheet_en.pdf#cd-peri-pin-config)
SCL/SDA can be mapped to any GPIO pins (21, 22, 23)

SCL --> 22

SDA --> 21

CSB --> 3.3v

SDD --> GND

### [Sensor](https://cdn.shopify.com/s/files/1/0837/4340/8415/files/BMP280-Datasheet.pdf?v=1768632341)
From top to bottom: VCC, GND, SCL, SDA, CSB, SDD
