#ifndef wifi_control_H_
#define wifi_control_H_

#include "esp_err.h"
#include "esp_http_server.h"
#include "driver/gpio.h"

extern bool wifi_ok;
extern esp_timer_handle_t wifi_timeout_timer;
extern char nombre_ap[32];   //variable de cambio de ID
extern uint8_t mac_para_ap[6];
extern bool ap;
extern bool ap_apretado;

void start_access_point(void);
void wifi_init(void);
void wifi_event_handler(void *arg, esp_event_base_t event_base,int32_t event_id, void *event_data);
int get_wifi_rssi(void);

extern int bandera_wifi;


#endif

