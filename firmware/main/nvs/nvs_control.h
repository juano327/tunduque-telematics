#ifndef nvs_control_H_
#define nvs_control_H_

#define DEVICE_ID_MAX_LEN 32

typedef struct {
    char ssid[32];
    char pass[64];    //estructura de guardado de datos de la red
} wifi_creds_t;

typedef struct {
    char ip[16];
    char gw[16];            //bufferes para guardar datos 
    char mask[16];
    char dns[16];    //estructura de guardado de datos de la ip fija
} ip_fija_t;

void save_device_id(const char *id);
esp_err_t load_device_id(char *out);
void save_gps_time(const char *gps_time);
esp_err_t load_gps_time(char *out);
void spiffs_init(void);
void guardar_gps_csv(float lat, float lon, float alt, float speed, int fix, int hour, int min, int seg);
esp_err_t load_wifi_creds(wifi_creds_t *out);
void save_wifi_creds(const char *ssid, const char *pass);
void save_static_ip(const char *ip, const char *gw, const char *mask, const char *dns);
esp_err_t load_static_ip(ip_fija_t *out);
void clear_static_ip(void);
esp_err_t save_mqtt_creds(const char *host, int port, const char *user, const char *pass);
esp_err_t nvs_save_u32(const char *key, uint32_t value);
esp_err_t nvs_load_u32(const char *key, uint32_t *value);


#endif