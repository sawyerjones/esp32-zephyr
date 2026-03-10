#ifndef MQTT_APP_H
#define MQTT_APP_H

#include <zephyr/net/mqtt.h>

/**
 * @brief connect to the MQTT broker defined in secrets.h
 * 
 * inits the mqtt_client struct, sets broker address/port,
 * and calls mqtt_connect(). Blocks until CONNACK is received.
 * must be called after wifi_connect()
 * 
 * @returns 0 on success, - errno on failure
 */
int mqtt_app_connect(void);

/**
 * @brief publish a sensor reading to sensor/esp32
 * 
 * serialises temp and pressure into a JSON payload and publishes
 * at QoS 0. Guards the mqtt_client with mqtt_sem internally.
 * 
 * @returns 0 on success, - errno on fail
 */
int mqtt_publish_sensor(double temp, double pressure);

/**
 * @brief MQTT thread entry point.
 *
 * defined via K_THREAD_DEFINE in mqtt.c, DO NOT CALL DIRECTLY.
 * polls the MQTT socket for incoming traffic, drives the keepalive
 * cycle via mqtt_live(), and drains sensor_msgq to publish readings.
 */
void mqtt_thread_entry(void *p1, void *p2, void *p3);

/**
 * @brief MQTT event callback (registered internally, exposed for reference)
 * 
 * handles CONNACK, DISCONNECT, and PUBACK events.
 * DO NOT CALL DIRECTLY.
 */
void mqtt_event_handler(struct mqtt_client *c, const struct mqtt_evt *evt);

#endif 