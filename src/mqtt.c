#include <zephyr/kernel.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_ip.h>
#include <string.h>
#include <stdio.h>
#include "secrets.h"

static uint8_t rx_buf[256];
static uint8_t tx_buf[256];

static struct mqtt_client client;
static struct sockaddr_in broker;
static struct mqtt_utf8 client_id;

/* http://www.steves-internet-guide.com/understanding-mqtt-qos-levels-part-1/ */
void mqtt_event_handler(struct mqtt_client *c, const struct mqtt_evt *evt) {
    switch (evt->type) {
        case MQTT_EVT_CONNACK:
            if (evt->result == 0) {
                printk("[MQTT] connected to broker\n");
            }
            break;
        case: MQTT_EVT_DISCONNECT:
            printk("[MQTT] disconnected from broker\n");
            break;
        case MQTT_EVT_PUBACK:
            printk("[MQTT] recieved PUBACK");
            break;
        default:
            break;
    }
}

int mqqt_app_connect(void) {
    broker.sin_family = AF_INET;
    broker.sin_port = htons(BROKER_PORT);
    net_addr_pton(AF_INET, BROKER_ADDR, &broker.sin_addr);

    return -1;
}

int mqtt_publish_sensor(float temp, float pressure) {
    char payload[128];
   

}