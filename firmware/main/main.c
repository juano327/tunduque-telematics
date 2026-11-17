#include <common.h>

nmea_parser_handle_t gps_handle = NULL; // Definición real

char device_id[DEVICE_ID_MAX_LEN] = {0};   //variable de cambio de ID
char tiempo_gps[32] = {0};   //variable de cambio tiempo de gps
char id_mqtt[128]={0};

extern void vManageSleepTask(void *pvParameters);

//MAIN//////
void app_main(void)
{

esp_task_wdt_delete(NULL);
//ESP_LOGE("system", "TUNDUQUE");

init_adc();       //inicio ADC 
 nvs_flash_init();
    hourmeter_init();
    start_gpio_config(); //inicializacion gpio

   // gpio_set_level(GPIO_NUM_12, 0); //enciendo gps (pnp)
    
 ESP_LOGI("system", "TUNDUQUE");
//xTaskCreate(sleep_task, "sleep_task", 4096, NULL, 5, NULL);

   // gpio_set_level(LED_GPIO, 1); //prendo led setup

//init_boton_on_off(); //inicializo boton de apagado

esp_efuse_mac_get_default(mac_para_ap); //obtengo mac para el nombre del ap

// causa por la cual se despierta el esp32
esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

// Manejo de wakeup desde deep sleep para ESP32-C3
if (cause == ESP_SLEEP_WAKEUP_GPIO) {
    uint64_t wakeup_pins = esp_sleep_get_gpio_wakeup_status();

    if (wakeup_pins & (1ULL << BOTON_AP_GPIO)) {
        ESP_LOGI("system", "Boton AP apretado para sacar de deep sleep, abriendo ap");
        ap = true;
    }
}

if (cause == ESP_SLEEP_WAKEUP_TIMER) {
    ESP_LOGI("system", "Despertado por timer");
}

const esp_app_desc_t *desc = esp_app_get_description();  //guardar version del firmware (git)
printf("Version Tunduque: %s\n", desc->version);

strcpy(version_firmware,  desc->version);

 wifi_timeout_timer_init(); //inicializo timer de wifi (bajo consumo)


#if !NO_REG_EN_HW
gpio_set_direction(PIN_EN_REG, GPIO_MODE_OUTPUT);
gpio_set_level(PIN_EN_REG, 1); //habilito pin del regulador
#endif




    vTaskDelay(pdMS_TO_TICKS(10)); 
 const esp_partition_t *running = esp_ota_get_running_partition(); //particion desde donde se esta ejecutando el codigo ahora
    const esp_partition_t *boot = esp_ota_get_boot_partition(); //con cual particion deberia arrancar

    if (running != boot) { //si el boot es diferente al running (particion)
        ESP_LOGW("OTA", "Validacion de ota");
        esp_err_t err = esp_ota_mark_app_valid_cancel_rollback(); //se valida y no se vuelve a la otra particion
        if (err == ESP_OK) {
            ESP_LOGW("OTA", "ota validada");
        } else {
            ESP_LOGE("OTA", "error en validacion de ota: %s", esp_err_to_name(err));
        }
    }
   

//spiffs_init();  //csv (guardado de datos caso desconexion wifi)
    
    wifi_init();    //conexion a wifi

if (ap){  //si no hay red configurada

start_access_point(); //inicio AP

xTaskCreate(led_blink_task, "led_blink", 1024, NULL, 1, NULL); //blinkeo led

}

xTaskCreate(vManageSleepTask, "manage_sleep_task", 4096, NULL, 5, NULL);
//xTaskCreate(power_beagle, "power_beagle_task", 2048, NULL, 5, NULL);
 //xTaskCreate(power_control, "power_control",4096, NULL, 10, NULL);  //sleep en baja bateria (sin hardware en este PCB)
  //  xTaskCreate(power_task, "power_task", 4096, NULL, 10, NULL); //control boton off

if (load_device_id(device_id) == ESP_OK)   //id del dispositivo
{
    ESP_LOGI("ID", "Device ID cargado: %s", device_id);
}
else
{ //en caso de no tener una id guardada, se usa la MAC
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    

    snprintf(device_id, sizeof(device_id),
             "%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2],
             mac[3], mac[4], mac[5]);

    ESP_LOGI("ID", "Usando MAC como ID: %s", device_id);
}

snprintf(id_mqtt, sizeof(id_mqtt), "tunduque-%s", device_id);

if (load_gps_time(tiempo_gps) == ESP_OK)  //tiempo de envio de datos por mqtt (cada cuanto)
{
    
    tiempo_gps_ms = atoi(tiempo_gps);
    ESP_LOGI("Time", "Tiempo de envio de gps: %d", tiempo_gps_ms); 
}
else
{ //por defecto 10 seg
    tiempo_gps_ms=10000;

    ESP_LOGI("Time", "Tiempo de envio de gps por defecto: %d", tiempo_gps_ms);
}



// Armado de topic MQTT dinámico

snprintf(mqtt_topic_pub, sizeof(mqtt_topic_pub),
         "tunduque/def/%s/report", device_id);

snprintf(mqtt_topic_pub_on_off, sizeof(mqtt_topic_pub_on_off),
         "tunduque/def/%s/onoff", device_id);

           //Armo topico de MQTT en base a la MAC -

snprintf(mqtt_topic_sub, sizeof(mqtt_topic_sub),
         "tunduque/def/%s/cmd", device_id);


        reinicio = true; //flag mqtt


    //  MQTT
    client = mqtt_init();  //Inicio MQTT cliente
    

    //CONFIG GPS 
    nmea_parser_config_t config = NMEA_PARSER_CONFIG_DEFAULT();  //config parser GPS y UART1

    config.uart.uart_port  = UART_NUM_1;
    config.uart.rx_pin     = GPIO_NUM_18;  // U1RXD en ESP32-S3
    config.uart.baud_rate  = 115200;
    config.uart.event_queue_size = 64;
    



vTaskDelay(pdMS_TO_TICKS(10));

    // Inicializacion del parser de gps
    nmea_parser_handle_t nmea_hdl = nmea_parser_init(&config);
    nmea_parser_add_handler(nmea_hdl, gps_event_handler, NULL);

// Activo GPS + BEIDOU (multiconstelacion)
uart_write_bytes(UART_NUM_1, "$PCAS04,3*1A\r\n", strlen("$PCAS04,3*1A\r\n"));
    vTaskDelay(pdMS_TO_TICKS(10));

    //Creacion de tarea MQTT
   xTaskCreate(mqtt_publish_task, "mqtt_pub", 6144, client, 5, &mqtt_task_handle);

gpio_install_isr_service(0);  // necesario antes de registrar ISR handlers
//init_boton_ap();
//xTaskCreate(boton_ap_task, "boton_ap", 6144, NULL, 5, NULL);


vTaskDelay(pdMS_TO_TICKS(10));
gpio_set_level(LED_GPIO, 0); //apago led setup, fin config

//vTaskDelete(NULL);

}




//MSJ ON OFF

// Función auxiliar opcional para obtener el texto del reset reason
const char* obtener_reset_reason_str(void) {
    esp_reset_reason_t razon = esp_reset_reason();
    
    switch (razon) {
        case ESP_RST_UNKNOWN:   return "UNKNOWN";    
        case ESP_RST_POWERON:   return "POWER_ON";
        case ESP_RST_EXT:       return "EXT_PIN";    
        case ESP_RST_PANIC:     return "PANIC";
        case ESP_RST_INT_WDT:   return "INT_WDT";
        case ESP_RST_TASK_WDT:  return "TASK_WDT";
        case ESP_RST_WDT:       return "WDT";
        case ESP_RST_DEEPSLEEP: return "DEEPSLEEP";  
        case ESP_RST_BROWNOUT:  return "BROWNOUT";
        case ESP_RST_SDIO:      return "SDIO";
        case ESP_RST_SW:        return "SOFT_RST";
        case ESP_RST_USB:       return "USB_RST";
        case ESP_RST_PWR_GLITCH: return "PWR_GLITCH";
        default:                return "UNSUPPORTED";
    }
}

//funcion json para msg on off

char* crear_json_on_off(void) {
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

    cJSON_AddNumberToObject(root, "sleep", 0);
    
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