cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE}) # requires Zephyr venv
project(my_app)
target_sources(app PRIVATE 
    src/main.c
    src/wifi.c
    src/mqtt.c
    src/sensor.c
)