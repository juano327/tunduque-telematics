#ifndef dns_control_H_
#define dns_control_H_

#include <sys/param.h>

#include "esp_log.h"
#include "esp_system.h"
#include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "common.h"

/**
 * start DNS server task
 */
void start_dns_server(void);

/**
 * Stop DNS server task
 * ESP_OK on success; any other value indicates an error
 */
esp_err_t stop_dns_server(void);

#endif // _dns_server_h_