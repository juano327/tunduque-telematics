#include <common.h>

char ota_url[256];


//OTA
void ota_task(void *pvParameter)
{
    

// 1. Primero, quitamos la tarea MQTT del registro del Watchdog
    if (mqtt_task_handle != NULL) {
        esp_task_wdt_delete(mqtt_task_handle);
        
        // 2. Ahora sí, eliminamos la tarea de FreeRTOS de forma segura
        vTaskDelete(mqtt_task_handle);
        mqtt_task_handle = NULL;
    }

    esp_http_client_config_t config = {
    .url = ota_url,  //url de la ota que se manda por mqtt, de aca descargo firm
    .skip_cert_common_name_check = true,
    .timeout_ms = 30000, //tiempo de espera de la solicitud http
    .keep_alive_enable = true, //dejo abierta conexion tcp, me evito handshakes y demoras
    .crt_bundle_attach = NULL,
};

esp_https_ota_config_t ota_config = {
    .http_config = &config, //implemento configuraciones anteriores
};

esp_err_t ret = esp_https_ota(&ota_config); //abre conexion http/https

    if (ret == ESP_OK) {
        printf("OTA realizada correctamente, se reinicia el esp32\n");

   /*         esp_mqtt_client_publish(client, mqtt_topic_pub_alert, "{"
"\"alert\":\"ota_finished\""   
    "}", 0, 1, 0);*/
// Obtienes los segundos actuales usando la función pública del horómetro
uint32_t segundos_actuales = hourmeter_get_total_seconds();

// Llamas a tu función de guardado con la clave y el valor exacto
nvs_save_u32("hm_sec", segundos_actuales);

// Pausa breve opcional para asegurar commit en Flash y reinicias
vTaskDelay(pdMS_TO_TICKS(50));
esp_restart();
    } else {
        printf("OTA ha fallado\n"); //si falla sigue funcionando el programa
        xTaskCreate(mqtt_publish_task, "mqtt_pub", 6144, client, 5, &mqtt_task_handle);
    }

    vTaskDelete(NULL); //elimino tarea
}
