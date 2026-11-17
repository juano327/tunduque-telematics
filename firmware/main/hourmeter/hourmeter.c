#include "common.h"




static const char *TAG = "HOURMETER";

#define HOURMETER_NVS_KEY "hm_sec"
#define HOURMETER_SAVE_INTERVAL_SEC 30

// Umbrales de tensión en Voltios
#define VOLTAGE_THRESHOLD_START  1.2f  
#define VOLTAGE_THRESHOLD_STOP   1.0f  

volatile uint32_t hourmeter_total_seconds = 0;
volatile uint32_t hourmeter_tmp_seconds = 0;
volatile uint32_t hourmeter_total_minutes = 0;
volatile uint32_t hourmeter_tmp_minutes = 0;
volatile uint32_t hourmeter_total_hours = 0;
volatile uint32_t hourmeter_tmp_hours = 0;

SemaphoreHandle_t hourmeter_mutex = NULL;
TimerHandle_t hourmeter_timer = NULL;

static bool running = false;
static uint32_t last_persist_seconds = 0;
static bool debug_force_active = false;
static bool debug_simulation_enabled = false;
static uint32_t debug_simulation_elapsed_s = 0;

static volatile bool     pending_set = false;
static volatile uint32_t pending_set_value = 0;
static volatile bool     force_nvs_save = false;


extern esp_err_t nvs_save_u32(const char *key, uint32_t value);
extern esp_err_t nvs_load_u32(const char *key, uint32_t *value);

static uint32_t hourmeter_try_get_total_seconds(void)
{
    uint32_t value = hourmeter_total_seconds;
    if (hourmeter_mutex != NULL) {
        if (xSemaphoreTake(hourmeter_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            value = hourmeter_total_seconds;
            hourmeter_tmp_seconds = value;
            xSemaphoreGive(hourmeter_mutex);
        }
    }
    return value;
}

static void hourmeter_try_set_total_seconds(uint32_t value)
{
    if (hourmeter_mutex == NULL) {
        hourmeter_total_seconds = value;
        return;
    }
    if (xSemaphoreTake(hourmeter_mutex, portMAX_DELAY) == pdTRUE) {
        hourmeter_total_seconds = value;
        hourmeter_tmp_seconds = value;
        xSemaphoreGive(hourmeter_mutex);
    }
}

// Lectura de tensión 
static bool hourmeter_input_active(void)
{
    if (debug_force_active) {
        return true;
    }

    if (debug_simulation_enabled) {
        return ((debug_simulation_elapsed_s / 30U) % 2U) == 0U;
    }

    bool motion = imu_motion_detected();
    return motion;
}

static esp_err_t persist_accumulated_time(void)
{
    uint32_t value_to_save = hourmeter_try_get_total_seconds();
    esp_err_t err = nvs_save_u32(HOURMETER_NVS_KEY, value_to_save);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "NVS save failed: %s", esp_err_to_name(err));
    }
    return err;
}

static void counter_timer_callback(TimerHandle_t xTimer)
{
    if (running) {
        if (xSemaphoreTake(hourmeter_mutex, 0) == pdTRUE) {
            hourmeter_total_seconds++;
            hourmeter_tmp_seconds = hourmeter_total_seconds;
            
            if ((hourmeter_total_seconds - last_persist_seconds) >= HOURMETER_SAVE_INTERVAL_SEC) {
                force_nvs_save = true;
                last_persist_seconds = hourmeter_total_seconds;
            }
            xSemaphoreGive(hourmeter_mutex);
        }
    }
    
    if (debug_simulation_enabled) {
        debug_simulation_elapsed_s++;
    }
}

static void hourmeter_task(void *pvParameters)
{
ESP_ERROR_CHECK(esp_task_wdt_add(NULL));

    (void)pvParameters;
    last_persist_seconds = hourmeter_try_get_total_seconds();

    float on_off = 0;

    while (1) {

        ESP_ERROR_CHECK(esp_task_wdt_reset());

        if (pending_set) {
            hourmeter_try_set_total_seconds(pending_set_value);
            pending_set = false;
            last_persist_seconds = hourmeter_total_seconds;
            ESP_LOGW(TAG, "Aplicando nuevo tiempo: %lu segundos", (unsigned long)pending_set_value);
            persist_accumulated_time();
        }

        if (force_nvs_save) {
            persist_accumulated_time();
            ESP_LOGI(TAG, "Running: total_seconds=%lu, total_minutes=%lu", (unsigned long)hourmeter_get_total_seconds(), (unsigned long)hourmeter_get_total_minutes());
            force_nvs_save = false;
        }

        bool sensor_active = hourmeter_input_active();
        on_off = leer_ON_OFF();

        if (sensor_active && !running) {
            running = true;
            xTimerStart(hourmeter_timer, 0);
            ESP_LOGI(TAG, "Hourmeter started (Voltage reached threshold)");
            gpio_set_level(GPIO_NUM_10, 1);
            persist_accumulated_time(); 
        } else if (!sensor_active && running) {
            running = false;
            xTimerStop(hourmeter_timer, 0);
            ESP_LOGI(TAG, "Hourmeter paused (Voltage dropped)");
            gpio_set_level(GPIO_NUM_10, 0);
            persist_accumulated_time();
        }

        vTaskDelay(pdMS_TO_TICKS(1000)); 
    }    
}

void hourmeter_init(void)
{
    if (hourmeter_mutex == NULL) {
        hourmeter_mutex = xSemaphoreCreateMutex();
        if (hourmeter_mutex == NULL) {
            ESP_LOGE(TAG, "Failed to create hourmeter mutex");
            return;
        }
    }

    uint32_t stored_seconds = 0;
    esp_err_t err = nvs_load_u32(HOURMETER_NVS_KEY, &stored_seconds);
    if (err == ESP_OK) {
        hourmeter_try_set_total_seconds(stored_seconds);
        ESP_LOGI(TAG, "Recovered %lu seconds from NVS", (unsigned long)stored_seconds);
    } else {
        hourmeter_try_set_total_seconds(0);
        ESP_LOGI(TAG, "No stored time in NVS, starting at 0");
    }

    imu_init();
    hourmeter_timer = xTimerCreate("hm_timer", pdMS_TO_TICKS(1000), pdTRUE, (void *)0, counter_timer_callback);

    running = false;
    last_persist_seconds = 0; 
    pending_set = false;
    pending_set_value = 0;
    debug_simulation_enabled = false;
    debug_simulation_elapsed_s = 0;
    
    xTaskCreate(hourmeter_task, "hourmeter_task", 4096, NULL, 5, NULL);
}
void hourmeter_set_total_mins(uint32_t minutes) {
    uint32_t total_seconds = minutes * 60U;
    hourmeter_try_set_total_seconds(total_seconds);
    last_persist_seconds = total_seconds;
    force_nvs_save = false;
    pending_set = false;
    pending_set_value = 0;
    ESP_LOGW(TAG, "Hourmeter set_total_mins(): %lu minutes (%lu seconds)", (unsigned long)minutes, (unsigned long)total_seconds);
    persist_accumulated_time();
}


uint32_t hourmeter_get_total_minutes(void) { return hourmeter_try_get_total_seconds() / 60U; }
void hourmeter_get_total_minutes_snapshot(uint32_t *minutes) { if (minutes) *minutes = hourmeter_get_total_minutes(); }
float hourmeter_get_total_hours(void) { return (float)hourmeter_get_total_minutes() / 60.0f; }
uint32_t hourmeter_get_total_seconds(void) { return hourmeter_try_get_total_seconds(); }
bool hourmeter_is_running(void) { return running; }
void hourmeter_debug_set_active(bool active) { debug_force_active = active; }
void hourmeter_debug_set_minutes(uint32_t minutes) { pending_set_value = minutes * 60U; pending_set = true; }
void hourmeter_reset(void) { hourmeter_set_total_mins(0); }
void hourmeter_debug_start_simulation(void) { debug_simulation_enabled = true; debug_simulation_elapsed_s = 0; debug_force_active = false; }
void hourmeter_debug_stop_simulation(void) { debug_simulation_enabled = false; debug_force_active = false; }