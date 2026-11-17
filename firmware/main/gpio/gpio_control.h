#ifndef gpio_control_H_
#define gpio_control_H_


#define BOTON_AP_GPIO 19  //boton AP
#define BOTON_AP_HOLD_MS   1000  //tiempo de boton presionado

#define BOTON_ON_OFF_GPIO 2  //boton off
#define BOTON_ON_OFF_HOLD_MS   2000  //tiempo de boton presionado
#define PIN_EN_REG  4   // sin regulador enable en este PCB (pin no conectado)
#define PIN_PWRON  15   

#define BLINK_GPIO 6 //led




extern volatile bool boton_ap_irq;
extern volatile bool boton_on_off_irq;

void start_gpio_config(void);
void boton_ap_isr(void *arg);
void init_boton_ap(void);
void led_blink_task(void *arg);
void boton_ap_task(void *arg);
void boton_on_off_isr(void *arg);
void init_boton_on_off(void);
void power_task(void *arg);


#endif