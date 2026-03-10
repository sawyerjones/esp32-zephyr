#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/logging/log.h>
#include "wifi.h"
#include "mqtt.h"

/* https://docs.zephyrproject.org/latest/services/logging/index.html#logging-in-a-module */
LOG_MODULE_REGISTER(bmp280_app, LOG_LEVEL_INF);

/* watchdog will restart everything if a thread hangs */
static const struct device *wdt = DEVICE_DT_GET(DT_NODELABEL(wdt0));
static int wdt_channel;

static void wdt_init(void) {
    struct wdt_timeout_cfg cfg = {
        .window = { .min = 0, .max = 10000 }, // 10 sec timeout
        .callback = NULL, // force reset on expiry
        .flags = WDT_FLAG_RESET_SOC,
    };

    if (!device_is_ready(wdt)) {
        LOG_ERR("[WDT] WDT not ready");
        return;
    }

    wdt_channel = wdt_install_timeout(wdt, &cfg);
    wdt_setup(wdt, WDT_OPT_PAUSE_HALTED_BY_DBG); // don't reset when debugging
}


int main(void) {
    printk("[WDT] starting setup...");
    wdt_init();

    printk("[ESP32] starting setup...\n");

    if (wifi_connect()) {
        LOG_ERR("[WIFI] ERROR: wifi_connect failed\n");
        return -1;
    }

    k_sleep(K_SECONDS(1));

    if (mqtt_app_connect()) {
        LOG_ERR("[MQTT] ERROR: mqtt_app_connect failed\n");
        return -1;
    }

    while (1) {
        wdt_feed(wdt, wdt_channel);
        k_sleep(K_SECONDS(5));
    }                
    return 0;
}