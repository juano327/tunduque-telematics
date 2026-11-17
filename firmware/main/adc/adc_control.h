#ifndef adc_control_H_
#define adc_control_H_

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"

#define ADC_UNIT        ADC_UNIT_2        // El GPIO 5 opera en la unidad ADC2
#define ADC_CHANNEL     ADC_CHANNEL_0     // El GPIO 5 es el canal 0 de la unidad 2
#define ADC_CHANNEL_5V  ADC_CHANNEL_4     // (Mantener si usa otra unidad/pin)
#define ADC_ATTEN       ADC_ATTEN_DB_12
#define ADC_BITWIDTH    ADC_BITWIDTH_12
#define ADC_CHANNEL_W   ADC_CHANNEL_0   
#define ADC_CHANNEL_O   ADC_CHANNEL_1

//bateria maquinaria (control de encendido y apagado)
#define ADC_CHANNEL_EN   ADC_CHANNEL_2

extern adc_oneshot_unit_handle_t adc_handle;
extern adc_cali_handle_t cali_handle;

void init_adc(void);
float leer_bateria(void);
float leer_5v(void);
int bateria_porcentaje(float v);
float filtro_iir(float new);
float leer_temperatura(void);
float leer_aceite(void);
float leer_ON_OFF(void);



#endif