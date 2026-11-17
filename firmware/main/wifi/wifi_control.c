#include <common.h>

//extern led_strip_handle_t led_strip;

//AP


// Timer del AP
static TimerHandle_t ap_timer_handle = NULL;


static void vRestartTimerCallback(TimerHandle_t xTimer)
{
    ESP_LOGW("TIMER AP", "Tiempo agotado de AP, Reiniciando el dispositivo...");
    
    
    // Obtienes los segundos actuales usando la función pública del horómetro
//uint32_t segundos_actuales = hourmeter_get_total_seconds();

// Llamas a tu función de guardado con la clave y el valor exacto
//nvs_save_u32("hm_sec", segundos_actuales);

// Pausa breve opcional para asegurar commit en Flash y reinicias
esp_restart(); // no usar vTaskDelay dentro de un callback de software-timer (Tmr Svc)

}

void init_ap_timer(void)
{
    ap_timer_handle = xTimerCreate(
        "ap_timer",               // Nombre del timer (para debugging)
        pdMS_TO_TICKS(900000),          // Tiempo en milisegundos (900000 = 15min)
        pdFALSE,                       // pdFALSE = One-shot (solo salta una vez)
        (void *)0,                     // ID opcional
        vRestartTimerCallback          // Función que se ejecuta al expirar
    );

    if (ap_timer_handle == NULL)
    {
        ESP_LOGE("TIMER", "No se pudo crear el timer de ap");
    }
}



///

// Timer de reintentos de conexion wifi

int bandera_wifi = 0;
bool wifi_retry_agotado = false;

static TimerHandle_t retry_timer_handle = NULL;

static TaskHandle_t xSleepTaskHandle = NULL;

void vManageSleepTask(void *pvParameters)
{
    // Registramos el handle de esta tarea para que el callback la reconozca
    xSleepTaskHandle = xTaskGetCurrentTaskHandle();

    while (1) {
        // La tarea duerme bloqueada consumiendo 0% de CPU hasta que el timer expire y la despierte
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        ESP_LOGI("SLEEP_TASK", "Ejecutando rutina de suspensión por timeout de reintentos...");

        // 1. ACTIVAMOS EL BLOQUEO DE RECONEXIÓN INMEDIATAMENTE
        wifi_retry_agotado = true;

        // 2. Desconectamos y detenemos el driver de Wi-Fi para que cese cualquier intento en curso
        esp_wifi_disconnect();
        esp_wifi_stop();

        /*
        // 3. Matar la tarea MQTT
        if (mqtt_task_handle != NULL) {
            vTaskDelete(mqtt_task_handle);
            mqtt_task_handle = NULL;
        }*/

        // 4. Limpiar el timer de reintentos
        if (retry_timer_handle != NULL) {
            xTimerStop(retry_timer_handle, 0);
            xTimerDelete(retry_timer_handle, 0);
            retry_timer_handle = NULL;
        }

        // 5. Maneja la suspensión
        uint32_t current_sec = hourmeter_get_total_seconds();
        entrar_en_suspension(current_sec);

        // Por seguridad, si entra_en_suspension regresa o demora, eliminamos la tarea
        vTaskDelete(NULL);
    }
}

static void vRetryTimerCallback(TimerHandle_t xTimer)
{
    ESP_LOGW("TIMER RETRY WIFI", "Tiempo agotado de REINTENTOS (expiro la ventana de 10 min) t=%lld us, SLEEP...", esp_timer_get_time());
    
    if (xSleepTaskHandle != NULL) {
        // Despierta de forma segura a la tarea encargada desde la interrupción/timer
        xTaskNotifyGive(xSleepTaskHandle);
    }
}

void init_retry_timer(void)
{
    retry_timer_handle = xTimerCreate(
        "retry_timer",               // Nombre del timer (para debugging)
        pdMS_TO_TICKS(600000),          // Tiempo en milisegundos 
        pdFALSE,                       // pdFALSE = One-shot (solo salta una vez)
        (void *)0,                     // ID opcional
        vRetryTimerCallback          // Función que se ejecuta al expirar
    );

    if (retry_timer_handle == NULL)
    {
        ESP_LOGE("TIMER RETRY WIFI", "No se pudo crear el timer de RETRY WIFI");
    }
}






bool wifi_connected = false;  //para el timer del ap
bool wifi_ok = false;
esp_timer_handle_t wifi_timeout_timer = NULL; //timer para sleep de wifi cuando no hay usb

char nombre_ap[32];
uint8_t mac_para_ap[6];

bool ap_apretado = false;

//inicio ap

void start_access_point(void)
{

    

ap_apretado = true;
//led_strip_set_pixel(led_strip, 0, 0, 0, 30);
//led_strip_refresh(led_strip);

// --- DETENEMOS EL TIMER DE REINTENTO SI ESTÁ ACTIVO ---
    if (retry_timer_handle != NULL)
    {
        if (xTimerIsTimerActive(retry_timer_handle) == pdTRUE)
        {
            if (xTimerStop(retry_timer_handle, 0) == pdPASS)
            {
                ESP_LOGI("TIMER RETRY", "AP abierto: Timer de retry detenido exitosamente.");
            }
            else
            {
                ESP_LOGE("TIMER RETRY", "Error al detener el timer de retry.");
            }
        }
    }
    /////

if(wifi_connected){ //solo activar el timer si hay una red configurada, sino queda ap abierto siempre
if (ap_timer_handle != NULL)
    {
        // xTimerReset reinicia el conteo desde cero si ya estaba corriendo,
        // o lo inicia si estaba detenido.
        if (xTimerReset(ap_timer_handle, 0) == pdPASS)
        {
            ESP_LOGE("TIMER AP", "Timer de AP activado/reiniciado con éxito.");
        }
        else
        {
            ESP_LOGE("TIMER AP", "Error al activar el timer.");
        }
    }}

if (esp_timer_is_active(wifi_timeout_timer)) {    //si está corriendo el timer, lo apago
        ESP_LOGI("Timer", "Ap abierto, elimino timer");
        esp_timer_stop(wifi_timeout_timer);}

    esp_netif_create_default_wifi_ap();

    snprintf(nombre_ap, sizeof(nombre_ap),
             "TUNDU_SETUP-%02X%02X%02X%02X%02X%02X",
             mac_para_ap[0], mac_para_ap[1], mac_para_ap[2],
             mac_para_ap[3], mac_para_ap[4], mac_para_ap[5]);


    wifi_config_t ap_cfg = {
        .ap = {
            //.ssid = nombre_ap,
            .ssid_len = 0,
            .channel = 1,                        //configuracion de AP
            .max_connection = 5,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
        }
    };
    strcpy((char *)ap_cfg.ap.ssid, nombre_ap);
    strcpy((char*)ap_cfg.ap.password, "catam1234!");

    esp_wifi_set_mode(WIFI_MODE_APSTA); 
    esp_wifi_set_config(WIFI_IF_AP, &ap_cfg);
    esp_wifi_start();     //arranco AP

    ESP_LOGI("AP", "Access Point iniciado. Conectar a:");
    ESP_LOGI("AP", "SSID: %s",nombre_ap);
    ESP_LOGI("AP", "PASS: catam1234!");
    ESP_LOGI("AP", "INGRESAR A: http://192.168.4.1");

start_dns_server();

    start_webserver();



}



//inicializacion del wifi

bool ap = false;
//bool use_static = false;

void wifi_init(void)
{

   // init_ap_timer(); //inicializo solamente el timer del ap 
    init_retry_timer(); //inicializo solamente el timer de retry de wifi
    esp_netif_init();
    esp_event_loop_create_default();

    // Crear interfaz WiFi STA
    esp_netif_t *netif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    vTaskDelay(pdMS_TO_TICKS(100)); // yield para alimentar el WDT

    // Registrar eventos
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);

    // Carga de datos de la NVS
    wifi_creds_t creds = {0};

    if (load_wifi_creds(&creds) == ESP_OK && strlen(creds.ssid) > 0)
    {
        ESP_LOGI("WIFI", "Usando credenciales guardadas:");
        ESP_LOGI("WIFI", "SSID: %s", creds.ssid);
        ap = false;
    }
    else
    {
        ESP_LOGW("WIFI", "No hay credenciales guardadas. Inicio de AP.");
        ap = true;
        return; // <--- SALIMOS ACÁ para evitar inicializar STA y evitar bloqueos/WDT
    }

    //config wifi con las credenciales
    wifi_config_t wifi_config = {0};

    strcpy((char*)wifi_config.sta.ssid, creds.ssid);
    strcpy((char*)wifi_config.sta.password, creds.pass);

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);


 ip_fija_t ipcfg;

 nvs_handle_t handle;
    uint8_t use_static = 0;

    if (nvs_open("storage", NVS_READONLY, &handle) == ESP_OK) {
        nvs_get_u8(handle, NVS_KEY_USE_STATIC, &use_static);
        nvs_close(handle);
    } else {
        ESP_LOGI("NVS", "Error abriendo handler de nvs (bandera)");
    }

    //si hay datos cargados en la nvs de la ip fija:
    if (load_static_ip(&ipcfg) == ESP_OK  && use_static==1)
    {
        ESP_LOGI("WIFI", "Configurando IP fija");

        esp_netif_dhcpc_stop(netif); //apago dhcp

        esp_netif_ip_info_t info = {0};
        esp_netif_str_to_ip4(ipcfg.ip, &info.ip);
esp_netif_str_to_ip4(ipcfg.gw, &info.gw);
esp_netif_str_to_ip4(ipcfg.mask, &info.netmask);

        esp_netif_set_ip_info(netif, &info);

        esp_netif_dns_info_t dns = {0};
dns.ip.type = ESP_IPADDR_TYPE_V4;
esp_netif_str_to_ip4(ipcfg.dns, &dns.ip.u_addr.ip4);
esp_netif_set_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns);

    }else{

ESP_LOGI("WIFI", "No hay IP fija, se usa DHCP");
 esp_netif_dhcpc_start(netif); 
    }

    vTaskDelay(pdMS_TO_TICKS(100)); // yield antes de start
    esp_wifi_start();
}






//handler del wifi


void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{

float usb = 0;
    usb = leer_5v();

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
{
    wifi_ok = false;
    wifi_connected = false;  

    // NUEVO: Si ya se agotaron los reintentos por el timer, ignoramos por completo cualquier reconexión
    if (wifi_retry_agotado) {
        ESP_LOGW("WIFI", "Reintentos cancelados. Se ignoró el evento de desconexión.");
        return;
    }

     if(ap_apretado == false){ //intenta reconectar siempre y cuando no este el ap abierto
         ESP_LOGW("WIFI", "WiFi desconectado. Reintentando...");
         gpio_set_level(BLINK_GPIO, 0);
         
         // Intentar reconectar
         esp_wifi_connect();

         if (retry_timer_handle != NULL)
         {
             if (xTimerIsTimerActive(retry_timer_handle) == pdFALSE)
             {
                 if (xTimerStart(retry_timer_handle, 0) == pdPASS)
                 {
                     ESP_LOGW("TIMER RETRY", "Timer de RETRY WIFI iniciado (ventana 10 min) t=%lld us", esp_timer_get_time());
                 }
                 else
                 {
                     ESP_LOGE("TIMER RETRY", "Error al iniciar el timer.");
                 }
             }
         }
     }
     else{
         ESP_LOGW("WIFI", "WiFi desconectado pero AP abierto, no se intenta reconectar");
     
}


    //    timer para abrir ap luego de un tiempo desconectado
    // Arranca el timer solo la primera vez

    
/*
    if(usb < 3.5){
    if (!esp_timer_is_active(wifi_timeout_timer))
    {
        if(ap_apretado == false){ //evito que se vuelva a disparar el timer si esta el ap abierto con baterias
            ESP_LOGI("WIFI", "Iniciando ventana de 5 minutos para reconectar"); //despues de 5 min entra en sleep
        esp_timer_start_once(wifi_timeout_timer, 5 * 60 * 1000000ULL);} //cambiar min
    }}

    if (usb >= 3.5)
{
    if (esp_timer_is_active(wifi_timeout_timer)) {
        ESP_LOGI("Timer", "USB conectado, cancelando timer");
        esp_timer_stop(wifi_timeout_timer);
    }
}*/

    /*
    else
    {
        ESP_LOGW(TAG1, "Fallaron intentos. Iniciando Access Point para configurar WiFi...");
        xTaskCreate(led_blink_task, "led_blink", 1024, NULL, 1, NULL);
        start_access_point();   

         

        //retry = 0; // reinicio para la próxima vez
        return;    // no seguir intentando reconectar
    }*/
}
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ESP_LOGI("WIFI", "WiFi conectado");
        gpio_set_level(GPIO_NUM_7, 1); //prendo relay
        gpio_set_level(BLINK_GPIO,1);
        wifi_ok = true;
        wifi_connected = true;

        // FIX: al reconectar hay que DETENER el retry_timer, si no expira igual
        // (10 min despues) y manda el equipo a sleep aun con internet.
        if (retry_timer_handle != NULL && xTimerIsTimerActive(retry_timer_handle) == pdTRUE) {
            xTimerStop(retry_timer_handle, 0);
            ESP_LOGI("TIMER RETRY", "WiFi conectado: timer de retry detenido");
        }
        wifi_retry_agotado = false; // permitir el ciclo normal de reconexion la proxima vez

        if (client != NULL) {
        esp_mqtt_client_start(client);
        
    }
        
        if (esp_timer_is_active(wifi_timeout_timer)) {
        ESP_LOGI("Timer", "Wifi conectado, elimino timer");
        esp_timer_stop(wifi_timeout_timer);
    }
        
    }
}


//obtener nivel de señal de wifi
int get_wifi_rssi(void)
{
    wifi_ap_record_t info;

    if (esp_wifi_sta_get_ap_info(&info) == ESP_OK) {
        return info.rssi;     // RSSI en dBm (ej: -45 es excelente, -80 es malo)
    } else {
        return 0;             // o algún valor por defecto
    }
}


