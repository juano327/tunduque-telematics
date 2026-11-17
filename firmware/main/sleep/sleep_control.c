#include <common.h>

//extern led_strip_handle_t led_strip;
//extern esp_mqtt_client_handle_t client;

void wifi_timeout_callback(void *arg)
{

    if(wifi_ok == false){
    ESP_LOGW("Timer", "Timer finalizado, entrando en deep sleep");

    gpio_set_level(LED_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(200));
        gpio_set_level(LED_GPIO, 0);
        vTaskDelay(pdMS_TO_TICKS(200));
        gpio_set_level(LED_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(200));
        gpio_set_level(LED_GPIO, 0);
        vTaskDelay(pdMS_TO_TICKS(200));
        gpio_set_level(LED_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(200));
        gpio_set_level(LED_GPIO, 0);
        vTaskDelay(pdMS_TO_TICKS(200));
        gpio_set_level(LED_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(200));


   // entrar_en_deep_sleep(60);
} //entra en deep sleep si no esta conectado al usb (60=5min)

    if (wifi_ok == true){

    ESP_LOGW("Timer", "WiFi ya reconectó, abortando sleep");
    return;

    }
    
}

void wifi_timeout_timer_init(void)
{
    esp_timer_create_args_t timer_args = {
        .callback = wifi_timeout_callback,
        .name = "wifi_timeout"
    };

    esp_timer_create(&timer_args, &wifi_timeout_timer);
}

//control de sleep

// 5 minutos en microsegundos (5 * 60 * 1,000,000)
#define SLEEP_DURATION_US (5ULL * 60ULL * 1000000ULL)

// Umbrales de tensión propios de la gestión de energía
#define VOLTAGE_THRESHOLD_START  25.0f  
#define VOLTAGE_THRESHOLD_STOP   24.2f  

static const char *TAG_POWER = "POWER_MGMT";

void entrar_en_suspension(uint32_t current_seconds) 
{

char *json_boot = crear_json_on_off_sleep();
                
                esp_mqtt_client_publish(client, mqtt_topic_pub_on_off, json_boot, 0, 1, 0);

                cJSON_free(json_boot);
                
                //vTaskDelay(pdMS_TO_TICKS(60000)); 

                gpio_set_level(GPIO_NUM_7, 0); // apago relay

    // 1. Guardar el horómetro antes de apagar
    nvs_save_u32("hm_sec", current_seconds);
    vTaskDelay(pdMS_TO_TICKS(50)); // Asegurar escritura en flash

    ESP_LOGI(TAG_POWER, "Tiempo de wifi terminado. Guardando horómetro (%lu s) y entrando en Deep Sleep por 5 min...", (unsigned long)current_seconds);

    // 2. Configurar temporizador RTC para despertar en 5 minutos
    esp_sleep_enable_timer_wakeup(SLEEP_DURATION_US);

    // 3. Entrar en modo Deep Sleep
     //gpio_set_level(GPIO_NUM_12, 1); //apago gps
    esp_deep_sleep_start();
}



void sleep_task(void *pvParameters) //NO SE USA ESTA
{
    (void)pvParameters;
    int segundos_apagado_continuo = 0;

    // Configuración y habilitación del wake-up por GPIO en el ESP32-C3
    const gpio_config_t wakeup_conf = {
        .pin_bit_mask = (1ULL << BOTON_AP_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_LOW_LEVEL
    };
    gpio_config(&wakeup_conf);

    // Habilitar el despertar por GPIO de forma correcta (con sus dos parámetros)
    esp_deep_sleep_enable_gpio_wakeup((1ULL << BOTON_AP_GPIO), ESP_GPIO_WAKEUP_GPIO_LOW);

    while (1) {
        // Leemos la tensión de la máquina independientemente
        float voltage = leer_ON_OFF();

        if (voltage >= VOLTAGE_THRESHOLD_START) {
            // Si la tensión es alta, hay energía: reiniciamos el contador de apagado
            segundos_apagado_continuo = 0;
        } 
        else if (voltage < VOLTAGE_THRESHOLD_STOP) {
            // La tensión está por debajo del límite de parada
            segundos_apagado_continuo++;
            
            ESP_LOGD(TAG_POWER, "Voltaje bajo detectado (%2.2fV). Acumulado apagado: %d/10s", voltage, segundos_apagado_continuo);

            // Si se mantiene apagado por 10 segundos continuos, disparamos el sleep
            if (segundos_apagado_continuo >= 10) {
                // Obtenemos el total de segundos directamente de la API pública del horómetro
                uint32_t current_sec = hourmeter_get_total_seconds();
                //led_strip_set_pixel(led_strip, 0, 30, 20, 10);
                //led_strip_refresh(led_strip);
                vTaskDelay(pdMS_TO_TICKS(100)); 

                char *json_boot = crear_json_on_off_sleep();
                
                esp_mqtt_client_publish(client, mqtt_topic_pub_on_off, json_boot, 0, 1, 0);

                cJSON_free(json_boot);
                
                vTaskDelay(pdMS_TO_TICKS(60000)); 

                gpio_set_level(GPIO_NUM_7, 0); // apago relay

                entrar_en_suspension(current_sec);
            }
        } 
        else {
            // Zona de histéresis: mantenemos el contador estático
            segundos_apagado_continuo = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(1000)); 
    }    
}

