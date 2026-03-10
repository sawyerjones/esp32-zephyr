#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include "sensor.h"

LOG_MODULE_REGISTER(sensor, LOG_LEVEL_INF);

// 4 reading buffer if we drop connection
K_MSGQ_DEFINE(sensor_msgq, sizeof(struct sensor_reading), 4, 4);

/* https://docs.zephyrproject.org/latest/doxygen/html/group__device__model.html#gaadf3ffb63df544eb3de356ab2c5e9e3c 
 * what actually allows abstraction away from hardware
 */
const struct device *const bmp280 = DEVICE_DT_GET_ANY(bosch_bme280);

// producer/consumer setup like 480
static void sensor_timer_handler(struct k_timer *timer);
static void sensor_work_handler(struct k_work *work);

K_TIMER_DEFINE(sensor_timer, sensor_timer_handler, NULL);
K_WORK_DEFINE(sensor_work, sensor_work_handler);

static void sensor_timer_handler(struct k_timer *timer) {
    k_work_submit(&sensor_work);
}

static void sensor_work_handler(struct k_work *work) {
    struct sensor_value temp, press;
    struct sensor_reading reading;

    if (sensor_sample_fetch(bmp280) < 0) {
        LOG_ERR("[SENSOR] failed to sample sensor");
        return;
    }

    sensor_channel_get(bmp280, SENSOR_CHAN_AMBIENT_TEMP, &temp);
    sensor_channel_get(bmp280, SENSOR_CHAN_PRESS, &press);

    reading.temperature = sensor_value_to_double(&temp);
    reading.pressure = sensor_value_to_double(&press);

    if (k_msgq_put(&sensor_msgq, &reading, K_NO_WAIT) != 0) {
        LOG_WRN("[SENSOR/MQTT] Sensor queue full, dropping reading");
    }
}

void sensor_thread_entry(void *p1, void *p2, void *p3) {
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

    if (!device_is_ready(bmp280)) {
        LOG_ERR("BMP280 not ready");
        return;
    }

    /* samples every 5 seconds */
    k_timer_start(&sensor_timer, K_SECONDS(5), K_SECONDS(5));

    /* work queue does sampling, but keep thread alive */
    while (1) {
        k_sleep(K_FOREVER);
    }
}

/* lower priority than mqtt thread */
K_THREAD_DEFINE(sensor_tid, 2048, sensor_thread_entry, NULL, NULL, NULL, 7, 0, 0);