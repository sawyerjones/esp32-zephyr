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

#define CLIENT_ID "zephyr-node1"
#define KEEPALIVE_S  60

/* http://www.steves-internet-guide.com/understanding-mqtt-qos-levels-part-1/ */
void mqtt_event_handler(struct mqtt_client *c, const struct mqtt_evt *evt) {
    switch (evt->type) {
        case MQTT_EVT_CONNACK:
            if (evt->result == 0) {
                printk("[MQTT] connected to broker\n");
            }
            break;
        case MQTT_EVT_DISCONNECT:
            printk("[MQTT] disconnected from broker\n");
            break;
        case MQTT_EVT_PUBACK:
            printk("[MQTT] recieved PUBACK");
            break;
        default:
            break;
    }
}

int mqtt_app_connect(void) {
    broker.sin_family = AF_INET;
    broker.sin_port = htons(BROKER_PORT);
    net_addr_pton(AF_INET, BROKER_ADDR, &broker.sin_addr);

    client_id.utf8 = (uint8_t *)CLIENT_ID;
    client_id.size = strlen(CLIENT_ID);

    mqtt_client_init(&client);
    client.broker = (struct sockaddr *)&broker;
    client.evt_cb = mqtt_event_handler;
    client.client_id = client_id;
    client.protocol_version = MQTT_VERSION_3_1_1;
    client.keepalive = KEEPALIVE_S;
    client.rx_buf = rx_buf;
    client.rx_buf_size = sizeof(rx_buf);
    client.tx_buf = tx_buf;
    client.tx_buf_size = sizeof(tx_buf);

    client.transport.type = MQTT_TRANSPORT_NON_SECURE;

    int ret = mqtt_connect(&client);
    if (ret) {
        printk("[MQTT] mqtt_connect failed: %d\n", ret);
    }
    return ret;
}

int mqtt_publish_sensor(float temp, float pressure) {
    char payload[128];
    snprintf(payload, sizeof(payload), "{\"temp\":%.2f,\"pressure\":%.2f}", (double)temp, (double)pressure);

    struct mqtt_publish_param param = {
        .message = {
            .topic = {
                .topic = { .utf8 = (uint8_t*)"sensor/node1/data",
                           .size = strlen("sensor/node1/data") },
                .qos = MQTT_QOS_0_AT_MOST_ONCE,
            },
            .payload = { .data = (uint8_t*)payload,
                         .len = strlen(payload) },
        },
        .message_id = 0,
        .dup_flag = 0,
        .retain_flag = 0,
    };

    return mqtt_publish(&client, &param);
}

void mqtt_poll_loop(void) {
    // which socket is monitored
    struct zsock_pollfd fds = {
        .fd = client.transport.tcp.sock,
        .events = ZSOCK_POLLIN,
    };

    while (1) {
        // poll socket
        int rc = zsock_poll(&fds, 1, mqtt_keepalive_time_left(&client));
        if (rc > 0 && (fds.revents & ZSOCK_POLLIN)) {
            // process response
            mqtt_input(&client);
        }
        mqtt_live(&client); // sends PINGREQ when keepalivetimer dies
    }
}