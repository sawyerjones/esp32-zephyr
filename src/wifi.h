#ifndef WIFI_H
#define WIFI_H

/**
 * @brief connect to the WiFi network defined in secrets.h
 * 
 * registers net_mgmt event callbacks for WiFi association and
 * IPv4 address assignment, then issues a connect request.
 * blocks on two semaphores — one for WiFi association, one for
 * DHCP — each with a 20 second timeout.
 * 
 * @returns 0 on success, -ETIMEDOUT if either stage times out,
 * - errno on connection request failure
 */
int wifi_connect(void);

#endif