#ifndef HOURMETER_H
#define HOURMETER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa el módulo del horómetro, configura el ADC, carga el tiempo desde NVS y lanza la tarea y el timer.
 */
void hourmeter_init(void);

/**
 * @brief Obtiene el total de segundos acumulados.
 * @return uint32_t Segundos totales.
 */
uint32_t hourmeter_get_total_seconds(void);

/**
 * @brief Obtiene el total de minutos acumulados.
 * @return uint32_t Minutos totales.
 */
uint32_t hourmeter_get_total_minutes(void);

/**
 * @brief Obtiene una copia segura (snapshot) de los minutos acumulados.
 * @param minutes Puntero donde se almacenará el valor de los minutos.
 */
void hourmeter_get_total_minutes_snapshot(uint32_t *minutes);

/**
 * @brief Obtiene el total de horas acumuladas en formato decimal.
 * @return float Horas totales.
 */
float hourmeter_get_total_hours(void);

/**
 * @brief Indica si el horómetro se encuentra contando actualmente (activo).
 * @return true Si está corriendo.
 * @return false Si está detenido/pausado.
 */
bool hourmeter_is_running(void);

/**
 * @brief Fuerza manualmente el estado de activación para propósitos de depuración.
 * @param active true para forzar encendido, false para volver al comportamiento normal del sensor.
 */
void hourmeter_debug_set_active(bool active);

/**
 * @brief Define de forma asíncrona los minutos acumulados (útil para comandos MQTT o depuración).
 * @param minutes Minutos a establecer.
 */
void hourmeter_debug_set_minutes(uint32_t minutes);

/**
 * @brief Define de forma asíncrona los minutos acumulados.
 * @param minutes Minutos a establecer.
 */
void hourmeter_set_total_mins(uint32_t minutes);

/**
 * @brief Define de forma asíncrona las horas acumuladas.
 * @param hours Horas a establecer.
 */
void hourmeter_set_total_hours(uint32_t hours);

/**
 * @brief Resetea el contador del horómetro a cero y actualiza la NVS.
 */
void hourmeter_reset(void);

/**
 * @brief Inicia una simulación interna del temporizador para pruebas sin hardware físico conectado.
 */
void hourmeter_debug_start_simulation(void);

/**
 * @brief Detiene la simulación interna del temporizador.
 */
void hourmeter_debug_stop_simulation(void);

#ifdef __cplusplus
}
#endif

#endif // HOURMETER_H