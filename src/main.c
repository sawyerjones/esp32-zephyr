#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include "wifi.h"
#include "mqtt.h"

/* https://docs.zephyrproject.org/latest/services/logging/index.html#logging-in-a-module */
LOG_MODULE_REGISTER(bmp280_app, LOG_LEVEL_INF);
/* https://docs.zephyrproject.org/latest/doxygen/html/group__device__model.html#gaadf3ffb63df544eb3de356ab2c5e9e3c 
 * what actually allows abstraction away from hardware
 */
const struct device *const bmp280_dev = DEVICE_DT_GET_ANY(bosch_bme280);

#define STACK_SIZE 2048
#define PRIORITY 5
#define SAMPLE_MS 5000

K_THREAD_STACK_DEFINE(pub_stack, STACK_SIZE);
static struct k_thread pub_thread;

static void publish_thread(void *p1, void *p2, void *p3) {
    /* struct {
    *      int val1 = natural num
    *      int val2 = following decimal as natural num
    */
    struct sensor_value temp, press;

    while (1) {
        sensor_sample_fetch(bmp280_dev); // directly from register defined in ../boards/esp...
        sensor_channel_get(bmp280_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp); //C
        sensor_channel_get(bmp280_dev, SENSOR_CHAN_PRESS, &press); // kPa

        float t = sensor_value_to_double(&temp);
        float p = sensor_value_to_float(&press);

        printk("[SENSOR] T=%.2f°C  P=%.1fhPa\n", (double)t, (double)p);
        mqtt_publish_sensor(t, p);

        k_sleep(K_MSEC(SAMPLE_MS));
    }
}

int main(void) {
    printk("[ESP32] starting setup...\n");
    if (!device_is_ready(bmp280_dev)) {
        LOG_ERR("[ESP32] ERROR: BMP280 not ready (check wiring)\n");
        return -1;
    }

    if (wifi_connect()) {
        printk("[WIFI] ERROR: wifi_connect failed\n");
        return -1;
    }

    k_sleep(K_SECONDS(1));

    if (mqtt_app_connect()) {
        printk("[MQTT] ERROR: mqtt_app_connect failed\n");
        return -1;
    }

    k_thread_create(&pub_thread, pub_stack, STACK_SIZE,
                    publish_thread, NULL, NULL, NULL,
                    PRIORITY, 0, K_NO_WAIT);

    mqtt_poll_loop();
    return 0;
}