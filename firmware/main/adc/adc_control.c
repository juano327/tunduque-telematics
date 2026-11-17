#include <common.h>

//ADC configuracion 

adc_oneshot_unit_handle_t adc_handle = NULL;
adc_oneshot_unit_handle_t adc_handle2 = NULL;
adc_cali_handle_t cali_handle = NULL;
adc_cali_handle_t cali_handle_e = NULL;

static const char *TAG = "ADC_IMU";
//static adc_oneshot_unit_handle_t adc2_handle;

// Inicialización de ADC + canal + calibración
void init_adc(void)
{
    // 1. Crear unidad ADC2
    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = ADC_UNIT
    };
    //ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_cfg, &adc_handle));

    // 2. Configurar el canal para el GPIO 5
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN_DB_12       // Rango de 0V a ~3.3V
    };
    //ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &config));

    // 3. Calibración (Curve fitting)
    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id = ADC_UNIT,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12
    };

    /*
    if (adc_cali_create_scheme_curve_fitting(&cali_cfg, &cali_handle) == ESP_OK) {
        ESP_LOGI(TAG, "Calibración ADC OK (Curve Fitting) en GPIO 5");
    } else {
        ESP_LOGW(TAG, "No hay calibración disponible, se usará método aproximado");
        cali_handle = NULL;
    }*/
}

float leer_bateria(void)
{
    int raw_value = 0;
    int voltage_mv = 0;

    // Usar los nombres correctos: adc_handle y ADC_CHANNEL
    ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw_value));

    // Si la calibración está activa, obtenemos milivoltios reales; si no, calculamos linealmente
    if (cali_handle != NULL) {
        adc_cali_raw_to_voltage(cali_handle, raw_value, &voltage_mv);
    } else {
        voltage_mv = (raw_value * 3300) / 4095;
    }

    // Convertir de milivoltios a Voltios
    float volts = (float)voltage_mv / 1000.0f;

    return volts;
}

float leer_5v(void)
{
    return 0;
}


int bateria_porcentaje(float v)
{
   return 0;
}

float filtro_iir(float new)   //suavizo cambio de porcentaje de bateria para mostrar en html
{
   return 0;
}


//temperatura de agua
//se utiliza un divisor resistivo (1kohm y 10kohm, se mide en la de 1kohm)



float leer_temperatura(void)
{
    return 0;
}



//presion de aceite




float leer_aceite(void)
{
    return 0;
}


#define FACTOR_CALIBRACION  (23.7f / 22.0f)

//BATERIA MAQUINA
float leer_ON_OFF(void)
{
   
   return 25;
}