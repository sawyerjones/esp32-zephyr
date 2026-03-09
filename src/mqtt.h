#ifndef MQTT_APP_H
#define MQTT_APP_H

#include <zephyr/net/mqtt.h>

/**
 * @brief connect to the MQTT broker defined in secrets.h
 * 
 * inits the mqtt_client struct, sets broker address/port,
 * and calls mqtt_connect(). Blocks until CONNACK is received.
 * 
 * @returns 0 on success, - errno on failure
 */
int mqtt_app_connect(void);

/**
 * @brief publish a sensor reading to sensor/node1/data
 * 
 * serialises temp and pressure into a JSON payload and publishes
 * at QoS 0. Requires mqtt_app_connect() to have succeeded first.
 * 
 * @returns 0 on success, - errno on fail
 */
int mqtt_publish_sensor(float temp, float pressure);

/**
 * @brief MQTT poll loop — blocking, never returns
 * 
 * polls the MQTT socket for incoming data and services the
 * keep-alive PING cycle via mqtt_live(). Called from main()
 * after mqtt_app_connect() succeeds.
 */
void mqtt_poll_loop(void);

/**
 * @brief MQTT event callback (registered internally, exposed for reference)
 * 
 * handles CONNACK, DISCONNECT, and PUBACK events.
 * DO NOT CALL DIRECTLY.
 */
void mqtt_event_handler(struct mqtt_client *c, const struct mqtt_evt *evt);

#endif 