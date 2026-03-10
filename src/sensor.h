#ifndef SENSOR_H
#define SENSOR_H

#include <zephyr/kernel.h>

/**
 * @brief sensor reading passed between the sensor and MQTT threads.
 */
struct sensor_reading {
    double temperature; /* degrees Celsius */
    double pressure;    /* hPa */
};

/**
 * @brief message queue populated by the sensor thread.
 */
extern struct k_msgq sensor_msgq;

/**
 * @brief sensor thread entry point.
 *
 * defined via K_THREAD_DEFINE in sensor.c, DO NOT CALL
 * inits BMP280, starts the periodic k_timer, and
 * blocks indefinitely while the system work queue handles sampling
 */
void sensor_thread_entry(void *p1, void *p2, void *p3);

#endif /* SENSOR_H */