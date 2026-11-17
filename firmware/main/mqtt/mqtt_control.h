#ifndef mqtt_control_H_
#define mqtt_control_H_

// mqtt config
#define MQTT_URI     "mqtts://mqtt.example.com:8883"
#define MQTT_USER      "CHANGE_ME"
#define MQTT_PASSW     "CHANGE_ME"

#define HW_VER     "1.0"

extern bool reinicio;
extern TaskHandle_t mqtt_task_handle;
extern bool mqtt_connected;
extern char mqtt_topic_pub[80];
extern char mqtt_topic_pub_on_off [80];
extern char mqtt_topic_sub[80];
extern char mqtt_topic_pub_state[80];
extern esp_mqtt_client_handle_t client;  //handler cliente mqtt
extern char version_firmware[100];
extern int tiempo_gps_ms;
extern bool ota_requested;

//datos del gps (estructura)
typedef struct {
    float lat;
    float lon;
    float alt;
    float speed;
    int fix;
    bool valid;
    int hour;
    int min;
    int seg;
    int sat;
    int32_t timestamp;
} gps_data_t;

extern gps_data_t gps_data;

void publicar_csv_mqtt(esp_mqtt_client_handle_t client);
void mqtt_event_handler(void *handler_args,esp_event_base_t base,int32_t event_id,void *event_data);
esp_mqtt_client_handle_t mqtt_init(void);
void mqtt_publish_task(void *arg);
const char* obtener_reset_reason_str(void);
char* crear_json_on_off(void);
char* crear_json_lwt(void);
char* crear_json_on_off_sleep(void);

#endif
