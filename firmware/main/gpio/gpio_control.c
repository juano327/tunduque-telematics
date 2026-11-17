#include <common.h>


//BOTON AP

volatile bool boton_ap_irq = false;
static const char *TAGL = "Tunduque_LED";
//led_strip_handle_t led_strip = NULL;

void start_gpio_config(void){

    /*
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,              //config GPIO led
    };

    gpio_config(&io_conf);*/

    gpio_config_t io_conf22 = {
        .pin_bit_mask = (1ULL << BLINK_GPIO),
        .mode = GPIO_MODE_OUTPUT,              //config GPIO led
    };
    gpio_config(&io_conf22);

    /*
    gpio_config_t gps_reset_conf = {
        .pin_bit_mask = (1ULL << GPS_RESET),
        .mode = GPIO_MODE_OUTPUT,              //config GPIO reset gps
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&gps_reset_conf);
    gpio_set_level(GPS_RESET, 0); //reseteo gps
    vTaskDelay(pdMS_TO_TICKS(500));
    gpio_set_level(GPS_RESET, 1); */

    // 1. Configurar la GPIO 11 como salida (on off gps)
    
    /*
    gpio_config_t io_conf11 = {
        .pin_bit_mask = (1ULL << GPIO_NUM_12), // Seleccionamos el pin
        .mode = GPIO_MODE_OUTPUT,              // Modo salida
        .pull_up_en = GPIO_PULLUP_DISABLE,     // Sin resistencia de pull-up interna
        .pull_down_en = GPIO_PULLDOWN_DISABLE, // Sin resistencia de pull-down interna
        .intr_type = GPIO_INTR_DISABLE         // Sin interrupciones
    };
    gpio_config(&io_conf11);*/

    gpio_config_t io_conf7 = {
        .pin_bit_mask = (1ULL << GPIO_NUM_7), // Seleccionamos el pin
        .mode = GPIO_MODE_OUTPUT,              // Modo salida
        .pull_up_en = GPIO_PULLUP_DISABLE,     // Sin resistencia de pull-up interna
        .pull_down_en = GPIO_PULLDOWN_DISABLE, // Sin resistencia de pull-down interna
        .intr_type = GPIO_INTR_DISABLE         // Sin interrupciones
    };
    gpio_config(&io_conf7);


    gpio_config_t io_conf10 = {
        .pin_bit_mask = (1ULL << GPIO_NUM_10), // Seleccionamos el pin
        .mode = GPIO_MODE_OUTPUT,              // Modo salida
        .pull_up_en = GPIO_PULLUP_DISABLE,     // Sin resistencia de pull-up interna
        .pull_down_en = GPIO_PULLDOWN_DISABLE, // Sin resistencia de pull-down interna
        .intr_type = GPIO_INTR_DISABLE         // Sin interrupciones
    };
    gpio_config(&io_conf10);
    
    


}

volatile bool already_pressed = false;

void IRAM_ATTR boton_ap_isr(void *arg)
{
    if(!already_pressed){
     boton_ap_irq = true;}
}

void init_boton_ap(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << BOTON_AP_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,       // Pull-Up habilitada para botón a GND
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE
    };
    gpio_config(&cfg);

    //gpio_install_isr_service(0);
    gpio_isr_handler_add(BOTON_AP_GPIO, boton_ap_isr, NULL);

    ESP_LOGI("BOTON_AP", "Botón AP inicializado");
}


//tarea de blinkeo de led cuando está iniciado el AP

void led_blink_task(void *arg)
{
ESP_ERROR_CHECK(esp_task_wdt_add(NULL));

    while (1) {
        ESP_ERROR_CHECK(esp_task_wdt_reset());
        gpio_set_level(BLINK_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(500));
        gpio_set_level(BLINK_GPIO, 0);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
//Tarea AP boton

void boton_ap_task(void *arg)
{
    TickType_t press_start;

    ESP_ERROR_CHECK(esp_task_wdt_add(NULL));

    while (1)
    {
        if (boton_ap_irq)
        {
ESP_ERROR_CHECK(esp_task_wdt_reset());

            boton_ap_irq = false;
            
            // --- PROTECCIÓN: Si el AP ya está abierto, ignoramos la pulsación ---
            if (ap_apretado)
            {
                ESP_LOGI("BOTON_AP", "El AP ya está abierto, ignorando botón.");
                vTaskDelay(pdMS_TO_TICKS(200));
                continue;
            }
            // -------------------------------------------------------------------

            already_pressed = true;

            // antirrebote
            vTaskDelay(pdMS_TO_TICKS(50));

            if (gpio_get_level(BOTON_AP_GPIO) == 0) //boton a gnd al presionar boton
            {
                press_start = xTaskGetTickCount(); //empieza a contar el tiempo de presionado

                // Mientras esté presionado
                while (gpio_get_level(BOTON_AP_GPIO) == 0)
                {
                    if ((xTaskGetTickCount() - press_start) * portTICK_PERIOD_MS >= BOTON_AP_HOLD_MS) //si se presiona mas de 1 seg se realiza esto
                    {
                        ESP_LOGW("BOTON_AP", "Boton apretado: entrando a modo AP");

                        //esp_wifi_stop(); //apago wifi del esp
                        vTaskDelay(pdMS_TO_TICKS(200));

                        start_access_point();  // abro ap
                        xTaskCreate(led_blink_task, "blink", 1024, NULL, 1, NULL); //parpadeo led

                        // espero que el usuario suelte el boton
                        while (gpio_get_level(BOTON_AP_GPIO) == 0)
                        {
                            vTaskDelay(pdMS_TO_TICKS(50));
                        }

                        break;
                    }

                    vTaskDelay(pdMS_TO_TICKS(10));
                }
                
            }
                        already_pressed = false;

        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

//BOTON ON-OFF

volatile bool boton_on_off_irq = false;

void IRAM_ATTR boton_on_off_isr(void *arg)
{
     boton_on_off_irq = true;
}

void init_boton_on_off(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << BOTON_ON_OFF_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_POSEDGE
    };
    gpio_config(&cfg);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BOTON_ON_OFF_GPIO, boton_on_off_isr, NULL);

    ESP_LOGI("BOTON_ON-OFF", "Botón ON-OFF inicializado");
}

void power_task(void *arg)
{
    ESP_LOGI("BOTON_ON-OFF", "tarea iniciada");
    TickType_t press_start;
    float usb = 0;

    while (1)
    {
    
/*
        if (boton_on_off_irq)
        {
            boton_on_off_irq = false;

            // antirrebote
            vTaskDelay(pdMS_TO_TICKS(50));

            
            if (gpio_get_level(BOTON_ON_OFF_GPIO) == 1) //boton a vcc al presionar boton
            {
                press_start = xTaskGetTickCount(); //empieza a contar el tiempo de presionado

                // Mientras esté presionado
                while (gpio_get_level(BOTON_ON_OFF_GPIO) == 1) //estado logico 1
                {
                    if ((xTaskGetTickCount() - press_start) * portTICK_PERIOD_MS >= BOTON_ON_OFF_HOLD_MS) //si se presiona mas de 1 seg se realiza esto
                    {//ESP_LOGI("BOTON_ON-OFF", "Botón ON-OFF presionado");
                       // esp_mqtt_client_publish(client, mqtt_topic_pub, "boton off presionado", 0, 1, 0);
                       //gpio_set_level(PIN_PWRON, 0);


usb = leer_5v();
//ESP_LOGI("USB", "USB ADC = %.2f V", usb);
if (usb<3.5){ //solo se apaga el regulador si el usb no esta conectado

gpio_set_level(LED_GPIO, 1);
vTaskDelay(pdMS_TO_TICKS(500));
gpio_set_level(LED_GPIO, 0);
vTaskDelay(pdMS_TO_TICKS(500));
gpio_set_level(LED_GPIO, 1);
vTaskDelay(pdMS_TO_TICKS(500));
gpio_set_level(LED_GPIO, 0);

                        gpio_set_direction(PIN_EN_REG, GPIO_MODE_OUTPUT);
while(1) {
    gpio_set_level(PIN_EN_REG, 0);
    esp_deep_sleep_start(); //por precaucion despues de apagar el reg, se pone en sleep
    vTaskDelay(pdMS_TO_TICKS(100));
}
     

                        break;}


                    }

                    vTaskDelay(pdMS_TO_TICKS(10));
                }
            }
        }
*/
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}


#define GPIO_IO_10      10
#define VOLT_THRESHOLD  25.0f

// Ajusta este factor según tu divisor resistivo (ej: 11.0f si usas 100k y 10k)
#define DIVIDER_RATIO   1.0f 

static const char *TAG = "POWER_BEAGLE";

void power_beagle(void *pvParameters)
{
    // 1. Configurar el GPIO 10 como salida
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_IO_10),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    // Asegurarse de iniciar en estado bajo (apagado)
    gpio_set_level(GPIO_IO_10, 0);

    while (1) {
        // Leer el voltaje del pin
        float pin_volts = leer_bateria();
        
        // Calcular el voltaje real de la batería usando el divisor
        float battery_volts = pin_volts * DIVIDER_RATIO;

        if (battery_volts < VOLT_THRESHOLD) {
            // Menor a 25V -> Apagar IO10
            gpio_set_level(GPIO_IO_10, 0);
            ESP_LOGW(TAG, "Voltaje bajo: %.2fV (< 25V). IO10 APAGADO", battery_volts);
        } else {
            // Mayor o igual a 25V -> Prender IO10
            gpio_set_level(GPIO_IO_10, 1);
            ESP_LOGI(TAG, "Voltaje OK: %.2fV (>= 25V). IO10 ENCENDIDO", battery_volts);
        }

        // Esperar 1 segundo antes de la siguiente lectura
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}