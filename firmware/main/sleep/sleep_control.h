#ifndef sleep_control_H_
#define sleep_control_H_

void entrar_en_deep_sleep(uint64_t sec);
void wifi_timeout_callback(void *arg);
void wifi_timeout_timer_init(void);
void power_control(void *arg); //apagado ante bateria muy baja
void entrar_en_suspension(uint32_t current_seconds);
void sleep_task(void *pvParameters);


#endif