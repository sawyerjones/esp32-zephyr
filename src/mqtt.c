#include <zephyr/kernel.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <stdio.h>
#include "secrets.h"
#include "sensor.h"

LOG_MODULE_REGISTER(mqtt_pub, LOG_LEVEL_INF);


static uint8_t rx_buf[256];
static uint8_t tx_buf[256];

static struct mqtt_client client;
static struct sockaddr_in broker;
static struct mqtt_utf8 client_id;

static uint8_t pub_topic[] = "sensor/esp32";

#define CLIENT_ID "zephyr-node1"
#define KEEPALIVE_S  60

/* controls publish/polling */
K_SEM_DEFINE(mqtt_sem, 1, 1);

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

int mqtt_publish_sensor(double temp, double pressure) {
    char payload[128];
    snprintf(payload, sizeof(payload), "{\"temp\":%.2f,\"pressure\":%.2f}", temp, pressure);

    struct mqtt_publish_param param = {
        .message = {
            .topic = {
                .topic = { .utf8 = pub_topic,
                           .size = sizeof(pub_topic) - 1 },
                .qos = MQTT_QOS_0_AT_MOST_ONCE,
            },
            .payload = { .data = (uint8_t*)payload,
                         .len = strlen(payload) },
        },
        .message_id = sys_rand32_get(),
        .dup_flag = 0,
        .retain_flag = 0,
    };

    k_sem_take(&mqtt_sem, K_FOREVER);
    int rc = mqtt_publish(&client, &param);
    k_sem_give(&mqtt_sem);

    if (rc != 0) {
        LOG_ERR("[MQTT] mqtt_publish failed: %d", rc);
    } else {
        LOG_INF("[MQTT] published: %s", payload);
    }

    return rc;
}

void mqtt_thread_entry(void *p1, void *p2, void *p3) {
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

    struct zsock_pollfd fds = { 
        .fd = client.transport.tcp.sock,
        .events = ZSOCK_POLLIN,
    };

    struct sensor_reading reading;

    /* wait for WIFI + mqtt_app_connect (signaled from main) */
    while (1) {
        // check for mqtt traffic w/o blocking
        int rc = zsock_poll(&fds, 1, 100);
        if (rc > 0 && (fds.revents & ZSOCK_POLLIN)) {
            k_sem_take(&mqtt_sem, K_FOREVER);
            mqtt_input(&client);
            k_sem_give(&mqtt_sem);
        }

        k_sem_take(&mqtt_sem, K_FOREVER);
        mqtt_live(&client); // whole reason for non-blocking, has to keep getting called otherwise we drop connection
        k_sem_give(&mqtt_sem);

        // publish
        while (k_msgq_get(&sensor_msgq, &reading, K_NO_WAIT) == 0) {
            mqtt_publish_sensor(reading.temperature, reading.pressure);
        }
    }
}

/* higher priority then sensor thread */
K_THREAD_DEFINE(mqtt_tid, 4096, mqtt_thread_entry, NULL, NULL, NULL, 6, 0, 0);