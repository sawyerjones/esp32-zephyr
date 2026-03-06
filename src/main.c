#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

/* https://docs.zephyrproject.org/latest/services/logging/index.html#logging-in-a-module */
LOG_MODULE_REGISTER(bmp280_app, LOG_LEVEL_INF);
/* https://docs.zephyrproject.org/latest/doxygen/html/group__device__model.html#gaadf3ffb63df544eb3de356ab2c5e9e3c 
 * what actually allows abstraction away from hardware
 */
const struct device *const bmp280_dev = DEVICE_DT_GET_ANY(bosch_bmp280);

int main(void) {
    if (!device_is_ready(bmp280_dev)) {
        LOG_ERR("BMP280 not ready (check wiring)");
        return -1;
    }

    while(1) {
        int ret = sensor_sample_fetch(bmp280_dev); // directly from register defined in ../boards/esp...
        if (ret < 0) {
            LOG_ERR("Failed to fetch sample: %d", ret);
            k_sleep(K_SECONDS(5));
            continue;
        }
        /* struct {
         *      int val1 = natural num
         *      int val2 = following decimal
         */
        struct sensor_value temp, press;
        sensor_channel_get(bmp280_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp); //C
        sensor_channel_get(bmp280_dev, SENSOR_CHAN_PRESS, &press); // kPa

        LOG_INF("Temp: %.2f °C", sensor_value_to_double(&temp));
        LOG_INF("Pressure: %.2f kPa", sensor_value_to_double(&press));

        k_sleep(K_SECONDS(2));
    }


}