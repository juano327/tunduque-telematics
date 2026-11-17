#include "common.h"
static const char *TAG = "IMU";

// Dirección I2C del LSM6DS3TR-C (0x6A por defecto, cambiar a 0x6B si SDO está en alto)
#define IMU_I2C_ADDRESS 0x6A

// Registros del LSM6DS3TR-C
#define IMU_REG_CTRL1_XL      0x10 // Control del acelerómetro (ODR y Full Scale)
#define IMU_REG_CTRL3_C       0x12 // Control general (Software reset, BDU, IF_INC)
#define IMU_REG_OUTX_L_XL     0x28 // Inicio de lectura del acelerómetro (Eje X, byte bajo)

// Escala para +/- 4g en el LSM6DS3TR-C (8192 LSB/g)
#define IMU_ACCEL_SCALE       8192.0f 

#define IMU_VIBRATION_THRESHOLD_G 0.25f 
#define RUIDO_DE_FONDO_G          0.05f

static bool imu_ready = false;

// Buffers de captura
static float accel_x_buf[N_SAMPLES];
static float accel_y_buf[N_SAMPLES];
static float accel_z_buf[N_SAMPLES];21` 

// Buffers para el procesamiento de ESP-DSP
__attribute__((aligned(16))) static float fft_work_buf[N_SAMPLES * 2];
__attribute__((aligned(16))) static float wind_buf[N_SAMPLES];

// Variables globales protegidas y su Mutex
static imu_datos_procesados_t datos_actuales;
static SemaphoreHandle_t imu_mutex = NULL;

static esp_err_t imu_i2c_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = IMU_I2C_MASTER_SDA_IO,
        .scl_io_num = IMU_I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = IMU_I2C_MASTER_FREQ_HZ,
        .clk_flags = 0,
    };

    esp_err_t err = i2c_param_config(IMU_I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_param_config failed: %s", esp_err_to_name(err));
        return err;
    }

    err = i2c_driver_install(IMU_I2C_MASTER_NUM, I2C_MODE_MASTER, 0, 0, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_driver_install failed: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

static esp_err_t imu_write_register(uint8_t reg, uint8_t value)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (IMU_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, value, true);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(IMU_I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(IMU_I2C_MASTER_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    return err;
}

static esp_err_t imu_read_registers(uint8_t reg, uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (IMU_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (IMU_I2C_ADDRESS << 1) | I2C_MASTER_READ, true);
    if (len > 1) {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(IMU_I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(IMU_I2C_MASTER_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    return err;
}

static bool imu_read_accel(float *ax, float *ay, float *az)
{
    uint8_t raw[6];
    // Lee los 6 registros consecutivos de X, Y, Z (gracias a IF_INC habilitado)
    esp_err_t err = imu_read_registers(IMU_REG_OUTX_L_XL, raw, sizeof(raw));
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "imu_read_registers failed: %s", esp_err_to_name(err));
        return false;
    }

    // LSM6DS3TR-C es Little-Endian: Byte Bajo primero, luego Byte Alto
    int16_t raw_ax = (raw[1] << 8) | raw[0];
    int16_t raw_ay = (raw[3] << 8) | raw[2];
    int16_t raw_az = (raw[5] << 8) | raw[4];

    *ax = raw_ax / IMU_ACCEL_SCALE;
    *ay = raw_ay / IMU_ACCEL_SCALE;
    *az = raw_az / IMU_ACCEL_SCALE;
    return true;
}

void imu_dsp_init(void)
{
    imu_mutex = xSemaphoreCreateMutex();
    if (imu_mutex == NULL) {
        ESP_LOGE(TAG, "Error creando el Mutex de la IMU");
        return;
    }

    esp_err_t ret = dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error inicializando ESP-DSP FFT: %s", esp_err_to_name(ret));
        return;
    }

    dsps_wind_hann_f32(wind_buf, N_SAMPLES);
    ESP_LOGI(TAG, "Módulo DSP inicializado correctamente.");
}

static float calcular_rms(float *data)
{
    float sum_squares = 0.0f;
    for (size_t i = 0; i < N_SAMPLES; i++) {
        sum_squares += data[i] * data[i];
    }
    return sqrtf(sum_squares / N_SAMPLES);
}

static void remover_componente_dc(float *data, size_t length)
{
    float suma = 0.0f;
    for (size_t i = 0; i < length; i++) {
        suma += data[i];
    }
    float media = suma / (float)length;

    for (size_t i = 0; i < length; i++) {
        data[i] -= media;
    }
}

static void analizar_espectro_completo(float *data, imu_analisis_eje_t *eje_salida, float rms_actual)
{   
    if (rms_actual < RUIDO_DE_FONDO_G) {
        eje_salida->freq_dominante = 0.0f;
        eje_salida->magnitud_pico = 0.0f;
        for (int i = 0; i < SPECTRUM_BINS; i++) {
            eje_salida->espectro_magnitud[i] = 0.0f;
        }
        return; 
    }

    for (int i = 0; i < N_SAMPLES; i++) {
        fft_work_buf[i * 2] = data[i] * wind_buf[i]; 
        fft_work_buf[i * 2 + 1] = 0.0f;              
    }

    dsps_fft2r_fc32(fft_work_buf, N_SAMPLES);
    dsps_bit_rev_fc32(fft_work_buf, N_SAMPLES);
    dsps_cplx2reC_fc32(fft_work_buf, N_SAMPLES);

    float max_val = 0.0f;
    int max_idx = 0;
    
    for (int i = 0; i < SPECTRUM_BINS; i++) {
        float magnitud = fft_work_buf[i];
        eje_salida->espectro_magnitud[i] = magnitud;

        if (i > 3 && magnitud > max_val) {
            max_val = magnitud;
            max_idx = i;
        }
    }

    eje_salida->freq_dominante = (float)max_idx * ((float)SAMPLE_RATE_HZ / (float)N_SAMPLES);
    eje_salida->magnitud_pico = max_val;
}

static void imu_task(void *pvParameters)
{

    ESP_ERROR_CHECK(esp_task_wdt_add(NULL));
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    // Calcular ticks y asegurar que sea al menos 1 para evitar el panic
    TickType_t xFrequency = pdMS_TO_TICKS(1000 / SAMPLE_RATE_HZ);
    if (xFrequency == 0) {
        xFrequency = 1;
    }

    while (1) {

ESP_ERROR_CHECK(esp_task_wdt_reset());

        for (int i = 0; i < N_SAMPLES; i++) {
            vTaskDelayUntil(&xLastWakeTime, xFrequency);
            
            float ax, ay, az;
            if (imu_read_accel(&ax, &ay, &az)) {
                accel_x_buf[i] = ax;
                accel_y_buf[i] = ay;
                accel_z_buf[i] = az - 1.0f; 
            }
        }

        // Resto del procesamiento DSP...
        imu_datos_procesados_t nuevos_datos;
        remover_componente_dc(accel_x_buf, N_SAMPLES);
        remover_componente_dc(accel_y_buf, N_SAMPLES);  
        remover_componente_dc(accel_z_buf, N_SAMPLES);

        nuevos_datos.x.rms = calcular_rms(accel_x_buf);
        nuevos_datos.y.rms = calcular_rms(accel_y_buf);
        nuevos_datos.z.rms = calcular_rms(accel_z_buf);

        analizar_espectro_completo(accel_x_buf, &nuevos_datos.x, nuevos_datos.x.rms);
        analizar_espectro_completo(accel_y_buf, &nuevos_datos.y, nuevos_datos.y.rms);
        analizar_espectro_completo(accel_z_buf, &nuevos_datos.z, nuevos_datos.z.rms);

        if (xSemaphoreTake(imu_mutex, portMAX_DELAY)) {
            datos_actuales = nuevos_datos;
            xSemaphoreGive(imu_mutex);
        }
    }
}

void imu_init(void)
{
    esp_err_t err = imu_i2c_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "IMU I2C init failed");
        imu_ready = false;
        return;
    }

    // 1. Configurar CTRL3_C (Registro 0x12)
    // Bit 6 (BDU = 1): Block Data Update (evita lectura parcial de registros high/low)
    // Bit 2 (IF_INC = 1): Incremento automático de dirección en ráfagas I2C
    err = imu_write_register(IMU_REG_CTRL3_C, 0x44);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "IMU CTRL3_C config failed: %s", esp_err_to_name(err));
        imu_ready = false;
        return;
    }

    // 2. Configurar CTRL1_XL (Registro 0x10) para el Acelerómetro
    // ODR_XL[3:0] = 0110 (416 Hz) -> Bits [7:4]
    // FS_XL[1:0]  = 10   (+/- 4g) -> Bits [3:2]
    // Resultado hexadecimal: 0x68
    err = imu_write_register(IMU_REG_CTRL1_XL, 0x68);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "IMU CTRL1_XL config failed: %s", esp_err_to_name(err));
        imu_ready = false;
        return;
    }

    imu_ready = true;
    ESP_LOGI(TAG, "IMU LSM6DS3TR-C inicializada: Rango +/-4g, ODR 416Hz. SDA=GPIO8 SCL=GPIO9");

    imu_dsp_init();

    xTaskCreate(imu_task, "IMU_TASK", 4096 * 2, NULL, 5, NULL);
}

bool imu_motion_detected(void)
{
    if (!imu_ready) {
        return false;
    }

    float ax, ay, az;
    if (!imu_read_accel(&ax, &ay, &az)) {
        return false;
    }

    static float ema_vibration = 0.0f;
    const float ALPHA = 0.55f; 

    float magnitude = sqrtf(ax * ax + ay * ay + az * az);
    float inst_delta = fabsf(magnitude - 1.0f);

    ema_vibration = (ALPHA * inst_delta) + ((1.0f - ALPHA) * ema_vibration);

    if (ema_vibration >= IMU_VIBRATION_THRESHOLD_G) {
        ESP_LOGW(TAG, "TRACTOR ENCENDIDO (Vibración: %.3f G)", ema_vibration);
        return true;
    } else {
        return false;
    }
}

bool imu_get_datos_procesados(imu_datos_procesados_t *datos_salida)
{
    if (imu_mutex != NULL && xSemaphoreTake(imu_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        *datos_salida = datos_actuales;
        xSemaphoreGive(imu_mutex);
        return true;
    }
    return false;
}