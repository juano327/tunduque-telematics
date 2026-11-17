#include <common.h>

wifi_creds_t saved_wifi;
ip_fija_t saved_fixed_ip;


void save_device_id(const char *id)   //guardado de nueva ID en NVS
{
    nvs_handle_t h;
    nvs_open("storage", NVS_READWRITE, &h);
    nvs_set_str(h, "device_id", id);     //guardo en NVS la id nueva    
    nvs_commit(h);
    nvs_close(h);
}


esp_err_t load_device_id(char *out)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &h);
    if (err != ESP_OK) return err;

    size_t len = DEVICE_ID_MAX_LEN;
    err = nvs_get_str(h, "device_id", out, &len);  //obtengo la id guardada en nvs
    nvs_close(h);
    return err;
}



void save_gps_time(const char *gps_time)   //guardado de nueva ID en NVS
{
    nvs_handle_t h;
    nvs_open("storage", NVS_READWRITE, &h);
    nvs_set_str(h, "gps_time", gps_time);     //guardo en NVS el nuevo tiempo   
    nvs_commit(h);
    nvs_close(h);
}


esp_err_t load_gps_time(char *out)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &h);
    if (err != ESP_OK) return err;

    size_t len = DEVICE_ID_MAX_LEN;
    err = nvs_get_str(h, "gps_time", out, &len);  //obtengo la id guardada en nvs
    nvs_close(h);
    return err;
}


//SPIFFS, para guardado de datos cuando no hay wifi

void spiffs_init(void)    //inicializacion de spiffs para guardar archivos en esp32
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 3,
        .format_if_mount_failed = true
    };

    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&conf));
}

/*  GUARDADO DE DATOS DE GPS EN LA NVS (POR EL MOMENTO NO SE USA)
//guardo datos del gps en formato csv dentro del esp32
void guardar_gps_csv(float lat, float lon, float alt, float speed, int fix, int hour, int min, int seg)
{

    FILE *f = fopen("/spiffs/gps_buffer.csv", "a");
    if (!f) {
        ESP_LOGE("CSV", "No se pudo abrir archivo");
        return;
    }

    fprintf(f, "%.5f,%.5f,%.2f,%.2f,%d,%02d:%02d:%02d\n",
            lat, lon, alt, speed, fix, hour, min, seg);

    fflush(f);
   // fclose(f);

    ESP_LOGI("CSV", "Dato guardado en CSV");

 fclose(f);


}

*/

//NVS de la parte del WIFI

esp_err_t load_wifi_creds(wifi_creds_t *out)  //obtener datos de la red (ssid y pass)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &handle);  //abro particion de flash "storage"
    if (err != ESP_OK) return err; //caso error de apertura

    size_t ssid_len = sizeof(out->ssid);        //se leen los datos de la red desde memoria flash
    size_t pass_len = sizeof(out->pass);  

    nvs_get_str(handle, "ssid", out->ssid, &ssid_len);   //se guardan en la estructura de arriba
    nvs_get_str(handle, "pass", out->pass, &pass_len);

    nvs_close(handle);
    return ESP_OK;
}

void save_wifi_creds(const char *ssid, const char *pass)  //guardar datos de la red (ssid y pass)
{
    nvs_handle_t handle;
    nvs_open("storage", NVS_READWRITE, &handle); //abro particion en modo escrit/lect

    nvs_set_str(handle, "ssid", ssid);   //guarda nombre red
    nvs_set_str(handle, "pass", pass);   //guarda contraseñal
    nvs_commit(handle);  //graba en flash
    nvs_close(handle);
}


//ip fija, guardar datos en nvs
void save_static_ip(const char *ip, const char *gw, const char *mask, const char *dns){

    nvs_handle_t handle;


    nvs_open("storage", NVS_READWRITE, &handle); //abro particion en modo escrit/lect


    nvs_set_str(handle, "ip", ip);   //guardo
    nvs_set_str(handle, "gw", gw);    //guardo gateway
    nvs_set_str(handle, "mask", mask);   //guardo mascara de subred
    nvs_set_str(handle, "dns", dns);   //guardo server dns

uint8_t use_static = 1;
    nvs_set_u8(handle, NVS_KEY_USE_STATIC, use_static);  //bandera persistente en nvs

    nvs_commit(handle);  //graba en flash
    nvs_close(handle);


ESP_LOGI("WIFI", "Configuracion IP fija guardada en NVS");

//use_static = true; //esto no funciona porque se borra cuando se reinicia el esp32


    
}


//obtener datos de ip fija desde nvs
esp_err_t load_static_ip(ip_fija_t *out) //recupero datos de ip fija de nvs
{
     nvs_handle_t handle;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &handle);
    if (err != ESP_OK) return err;


    size_t len;

    len = sizeof(out->ip);
    nvs_get_str(handle, "ip", out->ip, &len);
   

    len = sizeof(out->gw);
    nvs_get_str(handle, "gw", out->gw, &len);
   

    len = sizeof(out->mask);
    nvs_get_str(handle, "mask", out->mask, &len);
    

    len = sizeof(out->dns);
    nvs_get_str(handle, "dns", out->dns, &len);
    

    nvs_close(handle);

    //use_static = true;
    return ESP_OK;
}

void clear_static_ip(void) //se borran datos de la nvs si se decide usar dhcp
{
    nvs_handle_t handle;

    if (nvs_open("storage", NVS_READWRITE, &handle) != ESP_OK){
        return;}

    nvs_erase_key(handle, "ip");
    nvs_erase_key(handle, "gw");
    nvs_erase_key(handle, "mask");
    nvs_erase_key(handle, "dns");


uint8_t use_static = 0;
    nvs_set_u8(handle, NVS_KEY_USE_STATIC, use_static);  //bandera persistente en nvs


    nvs_commit(handle);
    nvs_close(handle);

    ESP_LOGI("WIFI", "IP fija borrada, se usará DHCP");

   // use_static = false;
}



//mqtt

esp_err_t save_mqtt_creds(const char *host, int port, const char *user, const char *pass)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) return err;

    nvs_set_str(nvs_handle, "mqtt_host", host);
    nvs_set_i32(nvs_handle, "mqtt_port", (int32_t)port);
    nvs_set_str(nvs_handle, "mqtt_user", user);
    nvs_set_str(nvs_handle, "mqtt_pass", pass);

    err = nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    return err;
}



//horimetro
esp_err_t nvs_save_u32(const char *key, uint32_t value)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_set_u32(handle, key, value);
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    return err;
}

 esp_err_t nvs_load_u32(const char *key, uint32_t *value)
{   //cargar valor por defecto 
    nvs_handle_t handle;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &handle);
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_get_u32(handle, key, value);
    nvs_close(handle);
    return err;
}