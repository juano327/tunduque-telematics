#ifndef IMU_H
#define IMU_H

#include "common.h"
#define N_SAMPLES 512     
#define SPECTRUM_BINS (N_SAMPLES / 2) // Número de bins en el espectro FFT
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa el bus I2C y prepara la IMU para lectura de aceleración.
 */
void imu_init(void);

/**
 * @brief Indica si se detecta movimiento a partir de la IMU.
 *
 * @return true cuando la aceleración supera el umbral de movimiento.
 * @return false cuando no hay movimiento o la IMU no está disponible.
 */
bool imu_motion_detected(void);

// Estructura para almacenar el resultado del espectro de un eje
typedef struct {
    float rms;              // Valor RMS total del eje en G
    float freq_dominante;   // Frecuencia con mayor energía en Hz
    float magnitud_pico;    // Magnitud relativa de esa frecuencia
    
    float espectro_magnitud[SPECTRUM_BINS];
} imu_analisis_eje_t;

// Estructura que agrupa el análisis tridimensional
typedef struct {
    imu_analisis_eje_t x;
    imu_analisis_eje_t y;
    imu_analisis_eje_t z;
} imu_datos_procesados_t;

// Funciones para obtener los datos procesados desde cualquier otra tarea (Seguras con Mutex)
bool imu_get_datos_procesados(imu_datos_procesados_t *datos_salida);
void imu_init(void);
static void analizar_espectro_completo(float *data, imu_analisis_eje_t *eje_salida, float rms_actual);
#ifdef __cplusplus
}
#endif

#endif // IMU_H
