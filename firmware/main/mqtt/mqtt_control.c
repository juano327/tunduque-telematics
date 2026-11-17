#include <common.h>


extern nmea_parser_handle_t gps_handle;

static bool boot_msg_sent = false;

////
extern int bandera_wifi;

bool reinicio = false; //para indicar una sola vez esp_restarted
TaskHandle_t mqtt_task_handle = NULL;
bool mqtt_connected = false;
char version_firmware[100];
int tiempo_gps_ms = 1000;

char mqtt_topic_pub[80];
char mqtt_topic_sub[80];
char mqtt_topic_pub_on_off [80];

esp_mqtt_client_handle_t client;
bool ota_requested = false;

gps_data_t gps_data = {0};


//handler del MQTT
// Handler del MQTT con todo el parseo de JSON y comandos directamente adentro
void mqtt_event_handler(void *handler_args,
                        esp_event_base_t base,
                        int32_t event_id,
                        void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;

    switch(event_id)
    {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI("MQTT", "Conectado al servidor MQTT");
            mqtt_connected = true;
            esp_mqtt_client_subscribe(event->client, mqtt_topic_sub, 1);
        
            // --- ENVÍO ÚNICO DEL JSON DE ON OFF AL INICIAR ---
            if (!boot_msg_sent) {
                char *json_boot = crear_json_on_off();
                if (json_boot != NULL) {
                    // Puedes cambiar "mqtt_topic_pub" por otro tópico específico si lo requieres, 
                    // o usar el mismo si se publica en el general.
                    int msg_id = esp_mqtt_client_publish(event->client, mqtt_topic_pub_on_off, json_boot, 0, 1, 0);
                    


                    
                    if (msg_id != -1) {
                        ESP_LOGI("MQTT", "JSON de inicio (boot) publicado exitosamente");
                        boot_msg_sent = true; // Evita que se vuelva a mandar en reconexiones
                    } else {
                        ESP_LOGE("MQTT", "Error al publicar el JSON de inicio");
                    }
                    cJSON_free(json_boot);
                }
            }
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI("MQTT", "MQTT desconectado");
            mqtt_connected = false;
            break;

        case MQTT_EVENT_DATA:
            {
                ESP_LOGI("MQTT", "Mensaje recibido:");
                printf("TOPIC: %.*s\n", event->topic_len, event->topic);
                printf("DATA : %.*s\n", event->data_len, event->data);

                // Copiamos los datos de forma segura a un buffer local terminado en '\0'
                char msg_buffer[300] = {0};
                int len = event->data_len;
                if (len >= sizeof(msg_buffer)) len = sizeof(msg_buffer) - 1;

                memcpy(msg_buffer, event->data, len);
                msg_buffer[len] = '\0';

                ESP_LOGI("MQTT", "RX: %s", msg_buffer);

                // Parseamos el JSON directamente aquí
                cJSON *root = cJSON_Parse(msg_buffer);
                if (!root)
                {
                    ESP_LOGE("MQTT CMD", "JSON parsing failed");
                    break;
                }

                const cJSON *event_json = cJSON_GetObjectItem(root, "event");
                if (event_json && event_json->valuestring)
                {
                    ESP_LOGI("MQTT CMD", "event: %s", event_json->valuestring);

                    // --- 1. COMANDO OTA ---
                    if (strstr(event_json->valuestring, "ota") != NULL)
                    {
                        ESP_LOGI("MQTT CMD", "Ota recibida");
                        //nmea_parser_deinit(gps_handle);
                        const cJSON *otaLink = cJSON_GetObjectItem(root, "otaLink");
                        if (otaLink && otaLink->valuestring)
                        {
                            ESP_LOGI("MQTT CMD", "otaLink: %s", otaLink->valuestring);
                            // Asegúrate de usar tu variable global (link_ota o ota_url)
                 strlcpy(ota_url, otaLink->valuestring, sizeof(ota_url));
                            if (!ota_requested) { ota_requested = true; xTaskCreate(&ota_task, "ota_task", 8192, NULL, 5, NULL); }
                        }
                    }

                    // --- 2. COMANDO GPS TIME ---
                    else if (strstr(event_json->valuestring, "gpstime") != NULL)
                    {
                        const cJSON *gpstime = cJSON_GetObjectItem(root, "time");
                        if (gpstime && gpstime->valuestring)
                        {
                            ESP_LOGW("MQTT", "Nuevo tiempo de gps recibido: %s", gpstime->valuestring);
                            save_gps_time(gpstime->valuestring);

                            ESP_LOGW("MQTT", "Reiniciando para aplicar nuevo tiempo de gps...");
                           
                            // Obtienes los segundos actuales usando la función pública del horómetro
                    uint32_t segundos_actuales = hourmeter_get_total_seconds();

                    // Llamas a tu función de guardado con la clave y el valor exacto
                    nvs_save_u32("hm_sec", segundos_actuales);

                    // Pausa breve opcional para asegurar commit en Flash y reinicias
                    vTaskDelay(pdMS_TO_TICKS(50));
                    esp_restart();
                        }
                    }

                    // --- 3. COMANDO SET MINUTES ---
                    else if (strstr(event_json->valuestring, "setminutes") != NULL || strstr(event_json->valuestring, "loadminutes") != NULL)
                    {
                        const cJSON *minutes_item = cJSON_GetObjectItem(root, "minutes");
                        if (minutes_item && cJSON_IsNumber(minutes_item))
                        {
                            uint32_t total_minutes = (uint32_t)minutes_item->valuedouble;
                            //ESP_LOGW("MQTT CMD", "Estableciendo minutos del hourmeter: %u", total_minutes);
                            hourmeter_set_total_mins(total_minutes);
                           // ESP_LOGI("MQTT CMD", "hourmeter cargado a %u minutos", total_minutes);
                        }
                        else
                        {
                            ESP_LOGE("MQTT CMD", "JSON sin campo minutes válido para setminutes");
                        }
                        

                        // Pausa breve opcional para asegurar commit en Flash y reinicias
                        vTaskDelay(pdMS_TO_TICKS(200));
                        esp_restart();
                    }

                    // --- 4. COMANDO RESET ---
                    else if (strstr(event_json->valuestring, "reset") != NULL || strstr(event_json->valuestring, "restart") != NULL)
                    {
                        ESP_LOGW("MQTT", "Reiniciando esp32 por comando MQTT");
                        // gpio_set_level(autoPWROFF, 1); // Descomenta si usas este pin
                        
                        // Obtienes los segundos actuales usando la función pública del horómetro
uint32_t segundos_actuales = hourmeter_get_total_seconds();

// Llamas a tu función de guardado con la clave y el valor exacto
nvs_save_u32("hm_sec", segundos_actuales);

// Pausa breve opcional para asegurar commit en Flash y reinicias
vTaskDelay(pdMS_TO_TICKS(50));
esp_restart();
                    }

                    // --- 5. COMANDO RELAY (ENCENDER / APAGAR GPIO 7) ---
                    else if (strstr(event_json->valuestring, "relay") != NULL)
                    {
                        const cJSON *state_item = cJSON_GetObjectItem(root, "state"); // O puedes usar "value" según prefieras
                        if (state_item && cJSON_IsNumber(state_item))
                        {
                            int pin_state = state_item->valueint; // 1 para encender, 0 para apagar
                            
                            // Control directo del GPIO_NUM_7
                            gpio_set_level(GPIO_NUM_7, pin_state);
                            
                            ESP_LOGI("MQTT CMD", "Relay (GPIO 7) cambiado a estado: %d", pin_state);
                        }
                        else
                        {
                            ESP_LOGE("MQTT CMD", "JSON sin campo 'state' válido para el relay");
                        }
                    }

                    // Puedes agregar aquí el resto de tus comandos ("changebroker", "report", etc.) de la misma manera
                }

                // Liberamos la memoria del objeto cJSON
                cJSON_Delete(root);
            }
            break;

        default:
            break;
    }
}






//Inicializacion de MQTT
esp_mqtt_client_handle_t mqtt_init(void)
{
    char mqtt_host[100] = {0};
    int mqtt_port = 0;
    char mqtt_uri[128] = {0};
    char mqtt_user[64] = {0};
    char mqtt_pass[64] = {0};

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &nvs_handle);


    
    if (err == ESP_OK) {
        size_t required_size = sizeof(mqtt_host);
        nvs_get_str(nvs_handle, "mqtt_host", mqtt_host, &required_size);

        nvs_get_i32(nvs_handle, "mqtt_port", (int32_t*)&mqtt_port);

        required_size = sizeof(mqtt_user);
        nvs_get_str(nvs_handle, "mqtt_user", mqtt_user, &required_size);

        required_size = sizeof(mqtt_pass);
        nvs_get_str(nvs_handle, "mqtt_pass", mqtt_pass, &required_size);

        nvs_close(nvs_handle);
    }

    // Si el host no está guardado, usamos el valor por defecto (sin el prefijo en la NVS)
    if (strlen(mqtt_host) == 0) {
        strcpy(mqtt_host, "mqtt.example.com");
        mqtt_port = 8883;
        ESP_LOGW("MQTT", "Usando host por defecto: %s:%d", mqtt_host, mqtt_port);
    } else {
        if (mqtt_port == 0) mqtt_port = 8883;
        ESP_LOGI("MQTT", "Host cargado desde NVS: %s:%d", mqtt_host, mqtt_port);
    }

    // Agregamos "mqtts://" automáticamente al principio de la URI final
    snprintf(mqtt_uri, sizeof(mqtt_uri), "mqtts://%s:%d", mqtt_host, mqtt_port);

    // Si el usuario no está guardado en NVS, usamos el valor por defecto
    if (strlen(mqtt_user) == 0) {
        strcpy(mqtt_user, "device_01");
    }

    // Si la contraseña no está guardada en NVS, usamos el valor por defecto
    if (strlen(mqtt_pass) == 0) {
        strcpy(mqtt_pass, "CHANGE_ME");
    }

char *json_lwt = crear_json_lwt();
    // ESP_LOGE("MQTT_LWT", "JSON generado correctamente: %s", json_lwt);


    esp_mqtt_client_config_t mqtt_cfg = {
    .broker = {
        .address.uri = mqtt_uri,
        .verification = {
            .skip_cert_common_name_check = true  //lo que dijo que agregara el Juan (TLS)
        }
    },
    .credentials = {
        .username = mqtt_user,
        .authentication.password = mqtt_pass,
        .client_id = id_mqtt,
    },
    .session = {
        .keepalive = 60, 
        .last_will = {
            .topic = mqtt_topic_pub_on_off,
            .msg = json_lwt,
            .msg_len = strlen(json_lwt),
            .qos = 1,
            .retain = 1
        }
    },
    .task = {
        .stack_size = 8192   // stack ampliado: TLS (mqtts) + cJSON_Parse dentro del event handler
    },
    .network = {
        .reconnect_timeout_ms = 5000
    }
};

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);

    esp_mqtt_client_register_event(
        client,
        ESP_EVENT_ANY_ID,
        mqtt_event_handler,
        NULL
    );

    return client;
}

//tarea de publicacion de datos MQTT

// Variable global para guardar la referencia a la tarea que debe ser despertada
static TaskHandle_t xTaskToNotify = NULL;

// Callback que ejecuta el timer cuando se cumple el tiempo
void vTimerCallback(TimerHandle_t pxTimer)
{
    if (xTaskToNotify != NULL) {
        // Despierta a la tarea de forma segura desde la interrupción/timer
        xTaskNotifyGive(xTaskToNotify);
    }
}

//extern TimerHandle_t retry_timer_handle;
//TimerHandle_t retry_timer_handle = NULL;

// Tarea de publicación de datos MQTT
void mqtt_publish_task(void *arg)
{

    ESP_ERROR_CHECK(esp_task_wdt_add(NULL));

    esp_mqtt_client_handle_t client = (esp_mqtt_client_handle_t)arg;
    int count = 0;

    // Registramos la tarea actual para que el timer sepa a quién despertar
    xTaskToNotify = xTaskGetCurrentTaskHandle();

    //asignacion de memoria para la IMU
    imu_datos_procesados_t *datos_imu = malloc(sizeof(imu_datos_procesados_t));
    if (datos_imu == NULL) {
        ESP_LOGE("MQTT", "No se pudo asignar memoria para datos_imu");
        // Manejo de error: podrías suspender la tarea o reiniciar
    }
    

    float vin = 0;
    int wifi_signal = 0;
    int vin_v = 0;
    int water_temp = 0;
    int oil_pressure = 0;
    char lat_str[32];
    char long_str[32];
    char alt_str[32];
    char speed_str[32];
    char time_str[16];
    float on_off = 0;
    char rms_x_str[16];
    char rms_y_str[16];
    char rms_z_str[16];
    char freq_x_str[16];
    char freq_y_str[16];
    char freq_z_str[16];

  
    // Crear y arrancar el temporizador de FreeRTOS con el tiempo configurado
    TimerHandle_t xTimer = xTimerCreate(
        "mqtt_timer",               
        pdMS_TO_TICKS(tiempo_gps_ms), 
        pdTRUE,                     
        (void *)0,                  
        vTimerCallback              
    );

    if (xTimer != NULL) {
        xTimerStart(xTimer, 0);
    }

    while (1)
    {


        // La tarea duerme indefinidamente y se bloquea (consume 0% CPU) 
        // hasta que el Timer la despierte exactamente una vez por ciclo.
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        ESP_ERROR_CHECK(esp_task_wdt_reset());

        // 1. VERIFICACIÓN TEMPRANA: Si no hay red o MQTT, nos ahorramos armar el JSON
        if (wifi_ok != true || mqtt_connected != true)
        {
            ESP_LOGW("MQTT", "Sin conexión. Omitiendo ciclo de envío...");
            continue; 
        }

        // 2. Leer sensores y estados actuales
        wifi_signal = get_wifi_rssi();
        vin = leer_5v();
        //vin_v = (int)(vin * 1000.0f);
        water_temp = leer_temperatura();
        oil_pressure = leer_aceite();
        on_off = leer_ON_OFF();
        vin_v = (int)(on_off );

        //leer datos de la IMU 
        bool imu_leida_correctamente = false;
        if (datos_imu != NULL) {
            // Esta función te trae el RMS, la frecuencia y todo el arreglo de una sola vez
            imu_leida_correctamente = imu_get_datos_procesados(datos_imu);
        }


        snprintf(lat_str, sizeof(lat_str), "%.10f", gps_data.lat);
        snprintf(long_str, sizeof(long_str), "%.10f", gps_data.lon);
        snprintf(alt_str, sizeof(alt_str), "%.2f", gps_data.alt);
        snprintf(speed_str, sizeof(speed_str), "%.2f", gps_data.speed);
        snprintf(time_str, sizeof(time_str), "%s", "a");



        // 3. CREAR EL OBJETO RAÍZ
        cJSON *root = cJSON_CreateObject();
        
        cJSON_AddNumberToObject(root, "db", -150);
        cJSON_AddNumberToObject(root, "net", 0);
        cJSON_AddNumberToObject(root, "w_db", wifi_signal);
        cJSON_AddNumberToObject(root, "vin", vin_v);
        cJSON_AddNumberToObject(root, "n", count);
        cJSON_AddNumberToObject(root, "r_t", tiempo_gps_ms/1000);
        
        // 4. Crear el objeto interno "data"
        cJSON *data = cJSON_CreateObject();
        cJSON_AddNumberToObject(data, "h_m", hourmeter_get_total_minutes());
        cJSON_AddNumberToObject(data, "w_t", water_temp);
        cJSON_AddNumberToObject(data, "o_p", oil_pressure);

        if (imu_leida_correctamente) {
            // Solo enviamos los datos mas relevantes de la IMU (RMS y frecuencia dominante) para no saturar el JSON
            snprintf(rms_x_str, sizeof(rms_x_str), "%.4f", datos_imu->x.rms);
            snprintf(rms_y_str, sizeof(rms_y_str), "%.4f", datos_imu->y.rms);
            snprintf(rms_z_str, sizeof(rms_z_str), "%.4f", datos_imu->z.rms);

            snprintf(freq_x_str, sizeof(freq_x_str), "%.4f",datos_imu->x.freq_dominante);
            snprintf(freq_y_str, sizeof(freq_y_str), "%.4f",datos_imu->y.freq_dominante);
            snprintf(freq_z_str, sizeof(freq_z_str), "%.4f",datos_imu->z.freq_dominante);

            
            cJSON_AddRawToObject(data, "rms_x", rms_x_str);
            cJSON_AddRawToObject(data, "rms_y", rms_y_str);
            cJSON_AddRawToObject(data, "rms_z", rms_z_str);
            
            // Opcional: También enviar la frecuencia que más está vibrando
            cJSON_AddRawToObject(data, "freq_x", freq_x_str);
            cJSON_AddRawToObject(data, "freq_y", freq_y_str);
            cJSON_AddRawToObject(data, "freq_z", freq_z_str);
            
            
        }

        // 5. Si hay buen GPS, rellenamos el objeto "data"
        if (gps_data.fix == 3)
        {
            cJSON_AddRawToObject(data, "lat", lat_str);
            cJSON_AddRawToObject(data, "lon", long_str);
            cJSON_AddRawToObject(data, "alt", alt_str);
            cJSON_AddRawToObject(data, "speed", speed_str);
            cJSON_AddNumberToObject(data, "fix", gps_data.fix);
            cJSON_AddNumberToObject(data, "timestamp", gps_data.timestamp);
        }
        else
        {
            cJSON_AddNumberToObject(data, "lat", 0);
            cJSON_AddNumberToObject(data, "lon", 0);
            cJSON_AddNumberToObject(data, "alt", 0);
            cJSON_AddNumberToObject(data, "speed", 0);
            cJSON_AddNumberToObject(data, "fix", gps_data.fix); 
            
            cJSON_AddNumberToObject(data, "timestamp", 0); 
        }

        cJSON_AddItemToObject(root, "data", data);

        // 6. Publicar mensaje
        char *mensaje_json = cJSON_Print(root);

        if (mensaje_json != NULL)
        {

            ESP_LOGI("MQTT_DEBUG", "Topic destino: %s", mqtt_topic_pub);
    ESP_LOGI("MQTT_DEBUG", "Payload JSON: %s", mensaje_json);

            int msg_id = esp_mqtt_client_publish(client, mqtt_topic_pub, mensaje_json, 0, 1, 0);

            if (msg_id != -1) {
                ESP_LOGI("MQTT", "Mensaje publicado por timer");
                count++; 
            } else {
                ESP_LOGE("MQTT", "Error al publicar mensaje");
            }

            cJSON_free(mensaje_json);

            // Indicador visual rápido
            gpio_set_level(LED_GPIO, 1); 
            vTaskDelay(pdMS_TO_TICKS(100)); 
            gpio_set_level(LED_GPIO, 0); 
        }

        // 7. Destruir el árbol JSON
        cJSON_Delete(root);
    }
}
        





//JSON LWT
 
char* crear_json_lwt(void) {
    char *string_json = NULL;

    // Obtener la descripción de la aplicación de ESP-IDF (que incluye la versión)
    const esp_app_desc_t *app_desc = esp_app_get_description();

    // Crear el objeto raíz
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        goto end;
    }


    // Agregar campos principales
    cJSON_AddStringToObject(root, "hw_ver", HW_VER);
    
    // Usamos la versión obtenida dinámicamente del firmware
    // (Si prefieres usar tu variable global version_firmware, puedes colocarla aquí directamente)
    cJSON_AddStringToObject(root, "fw_ver", app_desc->version);

    cJSON_AddNumberToObject(root, "sleep", 0);
    
    cJSON_AddStringToObject(root, "reset_reason", "LWT_OFF");
    cJSON_AddNumberToObject(root, "boot_time_ms", 0);

    // Crear el objeto anidado "wifi"
    cJSON *wifi = cJSON_CreateObject();
    if (wifi == NULL) {
        goto end;
    }
    cJSON_AddStringToObject(wifi, "ssid", "0");
    cJSON_AddStringToObject(wifi, "bssid", "0");
    cJSON_AddStringToObject(wifi, "ip", "0");
    cJSON_AddItemToObject(root, "wifi", wifi);

    // Crear el objeto anidado "modem"
    cJSON *modem = cJSON_CreateObject();
    if (modem == NULL) {
        goto end;
    }
    cJSON_AddNumberToObject(modem, "imei", 0);
    cJSON_AddNumberToObject(modem, "iccid", 0);
    cJSON_AddNumberToObject(modem, "prov", 0);
    cJSON_AddNumberToObject(modem, "band", 0);
    cJSON_AddItemToObject(root, "modem", modem);

    // Convertir a string con formato
    string_json = cJSON_Print(root);
    if (string_json == NULL) {
        fprintf(stderr, "Fallo al imprimir cJSON.\n");
    }

end:
    cJSON_Delete(root);
    return string_json;
}


char* crear_json_on_off_sleep(void) {
    char *string_json = NULL;

    // Obtener la descripción de la aplicación de ESP-IDF (que incluye la versión)
    const esp_app_desc_t *app_desc = esp_app_get_description();

    // Crear el objeto raíz
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        goto end;
    }


   // Buffers temporales para almacenar los datos de red reales
    char ssid_str[33] = {0};
    char bssid_str[18] = {0};
    char ip_str[16] = "0.0.0.0";

    // --- OBTENER DATOS DE WIFI (SSID y BSSID) ---
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        // Copiar el SSID de forma segura
        snprintf(ssid_str, sizeof(ssid_str), "%s", (char *)ap_info.ssid);

        // Formatear la MAC/BSSID a texto hexadecimal estándar (XX:XX:XX:XX:XX:XX)
        snprintf(bssid_str, sizeof(bssid_str), "%02x:%02x:%02x:%02x:%02x:%02x",
                 ap_info.bssid[0], ap_info.bssid[1], ap_info.bssid[2],
                 ap_info.bssid[3], ap_info.bssid[4], ap_info.bssid[5]);
    } else {
        snprintf(ssid_str, sizeof(ssid_str), "Desconocido");
        snprintf(bssid_str, sizeof(bssid_str), "00:00:00:00:00:00");
    }

    // --- OBTENER DIRECCIÓN IP ASIGNADA ---
    // Usamos el nombre por defecto estándar de la interfaz de estación en ESP-IDF
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    
    if (netif != NULL) {
        esp_netif_ip_info_t ip_info;
        if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
            // Convertir la IP de formato entero a string punteado ("192.168.x.x")
            esp_ip4addr_ntoa(&ip_info.ip, ip_str, sizeof(ip_str));
        }
    }


    uint32_t boot_time_ms = (uint32_t)(esp_timer_get_time() / 1000ULL);

    // Agregar campos principales
    cJSON_AddStringToObject(root, "hw_ver", HW_VER);
    
    // Usamos la versión obtenida dinámicamente del firmware
    // (Si prefieres usar tu variable global version_firmware, puedes colocarla aquí directamente)
    cJSON_AddStringToObject(root, "fw_ver", app_desc->version);

    cJSON_AddNumberToObject(root, "sleep", 1);
    
    cJSON_AddStringToObject(root, "reset_reason", obtener_reset_reason_str());
    cJSON_AddNumberToObject(root, "boot_time_ms", boot_time_ms);

    // Crear el objeto anidado "wifi"
    cJSON *wifi = cJSON_CreateObject();
    if (wifi == NULL) {
        goto end;
    }
    cJSON_AddStringToObject(wifi, "ssid", ssid_str);
    cJSON_AddStringToObject(wifi, "bssid", bssid_str);
    cJSON_AddStringToObject(wifi, "ip", ip_str);
    cJSON_AddItemToObject(root, "wifi", wifi);

    // Crear el objeto anidado "modem"
    cJSON *modem = cJSON_CreateObject();
    if (modem == NULL) {
        goto end;
    }
    cJSON_AddNumberToObject(modem, "imei", 0);
    cJSON_AddNumberToObject(modem, "iccid", 0);
    cJSON_AddNumberToObject(modem, "prov", 0);
    cJSON_AddNumberToObject(modem, "band", 0);
    cJSON_AddItemToObject(root, "modem", modem);

    // Convertir a string con formato
    string_json = cJSON_Print(root);
    if (string_json == NULL) {
        fprintf(stderr, "Fallo al imprimir cJSON.\n");
    }

end:
    cJSON_Delete(root);
    return string_json;
}