#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_event.h>
#include "wifi.h"
#include "secrets.h"

#define WIFI_TIMEOUT K_SECONDS(20)

/* k_sem_give = up
 * k_sem_take = down */
static K_SEM_DEFINE(wifi_connected, 0, 1);
static K_SEM_DEFINE(ipv4_obtained, 0, 1);

// bound to wifi/ipv4 event handler
#define WIFI_EVENTS (NET_EVENT_WIFI_CONNECT_RESULT | \
                      NET_EVENT_WIFI_DISCONNECT_RESULT)
#define IPV4_EVENTS NET_EVENT_IPV4_ADDR_ADD

static struct net_mgmt_event_callback wifi_cb;
static struct net_mgmt_event_callback ipv4_cb;

// iface required by net_mgmt_event_callback
static void wifi_event_handler(struct net_mgmt_event_callback *cb, 
                                uint64_t mgmt_event, struct net_if *iface) {
    if (mgmt_event == NET_EVENT_WIFI_CONNECT_RESULT) {
        struct wifi_status *status = (struct wifi_status *)cb->info;
        if (status->conn_status == WIFI_STATUS_CONN_SUCCESS) {
            printk("[WIFI] connected\n");
            k_sem_give(&wifi_connected);
        } else {
            printk("[WIFI] connection failed (status %d)\n", status->conn_status);
        }
    }
}

static void ipv4_event_handler(struct net_mgmt_event_callback *ipv4_cb,
                                uint64_t mgmt_event, struct net_if *iface) {
    if (mgmt_event == NET_EVENT_IPV4_ADDR_ADD) {
        printk("[WIFI] IP assigned");
        k_sem_give(&ipv4_obtained);
    } else {
        printk("[WIFI] IP assignment failed");
    }
}

int wifi_connect(void) {
    struct net_if *iface = net_if_get_default();
    struct wifi_connect_req_params params = {
        .ssid = WIFI_SSID,
        .ssid_length = strlen(WIFI_SSID),
        .psk = WIFI_PW,
        .psk_length = strlen(WIFI_PW),
        .security = WIFI_SECURITY_TYPE_PSK,
        .channel = WIFI_CHANNEL_ANY,
        .band = WIFI_FREQ_BAND_2_4_GHZ,
        .mfp = WIFI_MFP_OPTIONAL
    };

    net_mgmt_init_event_callback(&wifi_cb, wifi_event_handler, WIFI_EVENTS);
    net_mgmt_add_event_callback(&wifi_cb);
    net_mgmt_init_event_callback(&ipv4_cb, ipv4_event_handler, IPV4_EVENTS);
    net_mgmt_add_event_callback(&ipv4_cb);

    printk("[WIFI] connecting to: %s\n", WIFI_SSID);
    int ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &params, sizeof(params));
    if (ret) {
        printk("[WIFI] connection request failed: %d\n", ret);
        return ret;
    }

    if (k_sem_take(&wifi_connected, WIFI_TIMEOUT)) {
        printk("[wifi] Timeout waiting for WiFi\n");
        return -ETIMEDOUT;
    }

    struct net_linkaddr *mac = net_if_get_link_addr(iface);
    printk("[WIFI] MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
        mac->addr[0], mac->addr[1], mac->addr[2],
        mac->addr[3], mac->addr[4], mac->addr[5]);

    // move off of 1.0.0.0
    net_dhcpv4_start(iface);
    printk("[WIFI] DHCP started\n");

    if (k_sem_take(&ipv4_obtained, WIFI_TIMEOUT)) {
        printk("[wifi] Timeout waiting for IP\n");
        return -ETIMEDOUT;
    }

    struct net_in_addr *if_addr = net_if_ipv4_get_global_addr(iface, NET_ADDR_PREFERRED);
    if (if_addr) {
        char buf[NET_IPV4_ADDR_LEN];
        net_addr_ntop(AF_INET, if_addr, buf, sizeof(buf));
        printk("[WIFI] ESP32 IP: %s\n", buf);
    } else {
        printk("[WIFI] no preferred IPv4 address found\n");
    }

    printk("[WIFI] Ready!\n");
    return 0;
}