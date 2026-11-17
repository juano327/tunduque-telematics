#ifndef common_H_
#define common_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_http_server.h"
#include "esp_spiffs.h"
#include "esp_ota_ops.h"
#include "esp_https_ota.h"
#include "esp_http_client.h"
#include "esp_timer.h"
#include "esp_sleep.h"
#include "driver/rtc_io.h" 
#include "nvs/nvs_control.h"
#include "sleep/sleep_control.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "nmea_parser/nmea_parser.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "mqtt_client.h"
#include "esp_random.h"
#include "wifi/wifi_control.h"
#include "adc/adc_control.h"
#include "ota/ota_control.h"
#include "mqtt/mqtt_control.h"
#include "gpio/gpio_control.h"
#include "http/http_control.h"
#include "dns/dns_control.h"
#include "cJSON.h"
#include "hourmeter/hourmeter.h"
#include <stdint.h>
#include <math.h>
#include "imu/imu.h"
#include "driver/i2c.h"
#include "led_strip.h"
#include "esp_dsp.h"
#include "freertos/semphr.h"
#include "esp_task_wdt.h"


#define NVS_KEY_USE_STATIC "use_static"  //bandera guardada en nvs para que persista luego de reiniciar
// LED y GPS_RESET no existen en este PCB ESP32-S3
// Se definen en pines no conectados para que compile sin romper nada
#define LED_GPIO 11         // sin LED en este PCB
#define GPS_RESET 48        // sin GPS reset en este PCB
#define NO_LED_HW    1      // flag: no hay LED físico
#define NO_REG_EN_HW 1      // flag: no hay enable de regulador

//PINES IMU
#define IMU_I2C_MASTER_SCL_IO GPIO_NUM_1
#define IMU_I2C_MASTER_SDA_IO GPIO_NUM_3
#define IMU_I2C_MASTER_NUM I2C_NUM_0
#define IMU_I2C_MASTER_FREQ_HZ 400000
#define IMU_I2C_MASTER_TIMEOUT_MS 1000

// Configuraciones de DSP
     
#define SAMPLE_RATE_HZ 500     // 500 Hz = 2 ms exactos en FreeRTOS (si configTICK_RATE_HZ es 1000)

extern char id_mqtt[128];

extern void power_beagle(void *pvParameters);

#endif