#include <common.h>


//pagina html
static const char* WIFI_FORM_PAGE =
"<!DOCTYPE html>"
"<html lang='es'>"
"<head>"
"<meta charset='UTF-8'>"
"<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
"<title>Configuración de Tunduque</title>"
"<style>"
"body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background-color: #f4f7f6; color: #333; margin: 0; padding: 20px; display: flex; justify-content: center; }"
".container { background: white; padding: 30px; border-radius: 15px; box-shadow: 0 4px 15px rgba(0,0,0,0.05); width: 100%; max-width: 500px; }"
"h1 { color: #0078d4; font-size: 28px; margin-bottom: 5px; }"
".version { color: #666; font-size: 14px; margin-bottom: 25px; }"
".status-group { margin-bottom: 20px; }"
".status-line { margin-bottom: 12px; font-size: 16px; display: flex; align-items: center; font-weight: bold; }"
".status-val { font-weight: normal; margin-left: 8px; color: #444; }"
".section-card { background: #fdfdfd; border: 1px solid #eee; border-radius: 12px; padding: 20px; margin-top: 20px; box-shadow: 0 2px 8px rgba(0,0,0,0.02); }"
"h2 { font-size: 18px; color: #333; margin-top: 0; }"
"label { display: block; margin: 15px 0 5px; color: #555; font-size: 14px; }"
"input, select { width: 100%; padding: 10px; border: 1px solid #ccc; border-radius: 6px; box-sizing: border-box; font-size: 14px; }"
"button { width: auto; background: #0078d4; color: white; border: none; padding: 10px 20px; border-radius: 6px; cursor: pointer; font-weight: bold; margin-top: 15px; transition: 0.3s; }"
"button:hover { background: #005a9e; }"
".btn-scan { background: #0078d4; margin-bottom: 10px; width: 100%; }"
".btn-local { background: #6c757d; width: 100%; }"
".btn-local:hover { background: #5a6268; }"
".btn-finish { background: #28a745; width: 100%; font-size: 16px; padding: 12px; margin-top: 25px; }"
".btn-finish:hover { background: #218838; }"
".btn-update { background: #e1e1e1; color: #333; border: 1px solid #ccc; }"
".btn-update:hover { background: #d4d4d4; }"
".badge { font-size: 12px; margin-top: 8px; font-weight: bold; display: block; }"
".checkbox-line { display: flex; align-items: center; gap: 8px; margin-top: 15px; }"
".checkbox-line input[type=checkbox] { width: auto; margin: 0; }"
".checkbox-line span { font-size: 14px; color: #555; }"
"</style>"
"</head>"
"<body>"
"<div class='container'>"
"<h1>Configuración de Tunduque</h1>"
"<div class='version'>Versión de firmware: 1.0</div>"

"<div class='status-group'>"
"<div class='status-line'>Señal del cliente: <span class='status-val'><span id='rssi'>--</span> dBm</span></div>"
"<div class='status-line'>Batería: <span class='status-val'><span id='bat'>--</span> V</span></div>"
"<div class='status-line'>MAC: <span class='status-val' id='mac'>--</span></div>"
"<div class='status-line'>Temp. agua: <span class='status-val'><span id='w_temp'>--</span>°C</span></div>"
"<div class='status-line'>Presión aceite: <span class='status-val'><span id='o_press'>--</span> Lbs/p</span></div>"
"<div class='status-line'>Horímetro: <span class='status-val'><span id='horimetro'>--</span> hs</span></div>"
"</div>"

// ---------------------------------------------------------
// TARJETA WIFI
// ---------------------------------------------------------
"<div class='section-card'>"
"<h2>Configurar redes WiFi</h2>"
"<button type='button' class='btn-scan' id='scan-btn' onclick='scan()'>Escanear redes WiFi</button>"

"<label>Redes disponibles:</label>"
"<select id='net-select' onchange='document.getElementById(\"ssid\").value=this.value'>"
"<option>-- Seleccionar red --</option>"
"</select>"

"<input id='ssid' placeholder='Nombre de la red' style='margin-top:10px;'>"
"<label>Contraseña:</label>"
"<input type='password' id='pass' placeholder='Introducir contraseña'>"

"<label class='checkbox-line'>"
"<input type='checkbox' id='use_static' onchange='toggleStatic()'>"
"<span>Usar IP fija</span>"
"</label>"

"<div id='static-fields' style='display:none;'>"
"<label>IP fija:</label>"
"<input id='ip'>"
"<label>Puerta de enlace:</label>"
"<input id='gateway'>"
"<label>Máscara de red:</label>"
"<input id='netmask'>"
"<label>DNS:</label>"
"<input id='dns'>"
"</div>"

"<button type='button' class='btn-local' onclick='guardarWifiLocal()'>Guardar WiFi</button>"
"<span id='wifi-status' class='badge' style='color:#6c757d;'>WiFi no guardado</span>"
"</div>"

// ---------------------------------------------------------
// TARJETA MQTT
// ---------------------------------------------------------
"<div class='section-card'>"
"<h2>Configuración de MQTT</h2>"
"<label>Host del Broker:</label>"
"<input id='mqtt_host'>"
"<label>Puerto MQTT:</label>"
"<input type='number' id='mqtt_port'>"
"<label>Usuario MQTT:</label>"
"<input id='mqtt_user'>"
"<label>Contraseña MQTT:</label>"
"<input type='password' id='mqtt_pass'>"

"<button type='button' class='btn-local' onclick='guardarMqttLocal()'>Guardar MQTT</button>"
"<span id='mqtt-status' class='badge' style='color:#6c757d;'>MQTT no guardado</span>"
"</div>"

// ---------------------------------------------------------
// BOTÓN FINAL GLOBAL
// ---------------------------------------------------------
"<button type='button' class='btn-finish' onclick='enviarTodo()'>Finalizar configuración</button>"

"<div class='section-card' style='border:none; background:transparent; padding:0; margin-top:20px;'>"
"<h2>Actualización de Firmware</h2>"
"<input type='file' id='firmware' accept='.bin' style='border:none; padding:0;'>"
"<button class='btn-update' onclick='uploadOTA()'>Actualizar</button>"
"<div id='ota_status' style='font-size:12px; margin-top:5px; color:#d43f33;'></div>"
"</div>"

"</div>"

"<script>"
"let scanning = false;"
"let statusTimer = setInterval(updateStatus, 2000);"

// Objetos para almacenar los datos en memoria local del navegador
"let wifiData = null;"
"let mqttData = null;"

"function updateStatus() {"
"  if (scanning) return;"
"  fetch('/status').then(r => r.json()).then(d => {"
"    document.getElementById('mac').innerText = d.mac;"
"    document.getElementById('rssi').innerText = d.rssi;"
"    document.getElementById('bat').innerText = d.bat;"
"    document.getElementById('w_temp').innerText = d.w_temp;"     // NUEVO
"    document.getElementById('o_press').innerText = d.o_press;"   // NUEVO
"    document.getElementById('horimetro').innerText = d.horimetro;"  // NUEVO
"  }).catch(() => {});"
"}"

"function toggleStatic() {"
"  const chk = document.getElementById('use_static');"
"  const box = document.getElementById('static-fields');"
"  box.style.display = chk.checked ? 'block' : 'none';"
"}"

"function scan() {"
"  scanning = true;"
"  const btn = document.getElementById('scan-btn');"
"  const sel = document.getElementById('net-select');"
"  btn.innerText = 'Escaneando...';"
"  btn.disabled = true;"
"  fetch('/scan').then(() => setTimeout(() => {"
"    fetch('/scan_result').then(r => r.json()).then(list => {"
"      sel.innerHTML = '<option>-- Seleccionar red --</option>';"
"      list.forEach(n => {"
"        let opt = document.createElement('option');"
"        opt.value = n.ssid;"
"        opt.innerText = n.ssid + ' (' + n.rssi + ' dBm)';"
"        sel.appendChild(opt);"
"      });"
"    }).finally(() => {"
"      scanning = false;"
"      btn.innerText = 'Escanear redes WiFi';"
"      btn.disabled = false;"
"    });"
"  }, 2500));"
"}"

// Guarda WiFi en la variable temporal de JavaScript
"function guardarWifiLocal() {"
"  wifiData = {"
"    ssid: document.getElementById('ssid').value,"
"    pass: document.getElementById('pass').value,"
"    use_static: document.getElementById('use_static').checked ? '1' : '0',"
"    ip: document.getElementById('ip').value,"
"    gateway: document.getElementById('gateway').value,"
"    netmask: document.getElementById('netmask').value,"
"    dns: document.getElementById('dns').value"
"  };"
"  const st = document.getElementById('wifi-status');"
"  st.innerText = '✔ WiFi preparado para enviar';"
"  st.style.color = '#28a745';"
"}"

// Guarda MQTT en la variable temporal de JavaScript
"function guardarMqttLocal() {"
"  mqttData = {"
"    mqtt_host: document.getElementById('mqtt_host').value,"
"    mqtt_port: document.getElementById('mqtt_port').value,"
"    mqtt_user: document.getElementById('mqtt_user').value,"
"    mqtt_pass: document.getElementById('mqtt_pass').value"
"  };"
"  const st = document.getElementById('mqtt-status');"
"  st.innerText = '✔ MQTT preparado para enviar';"
"  st.style.color = '#28a745';"
"}"

// Empaqueta todo lo que esté listo y lo manda al ESP32 por POST
"async function enviarTodo() {"
"  if (!wifiData && !mqttData) {"
"    alert('Debes presionar \"Guardar WiFi\" o \"Guardar MQTT\" al menos en una sección antes de finalizar.');"
"    return;"
"  }"
"  let params = new URLSearchParams();"
"  if (wifiData) {"
"    for (let k in wifiData) params.append(k, wifiData[k]);"
"  }"
"  if (mqttData) {"
"    for (let k in mqttData) params.append(k, mqttData[k]);"
"  }"
"  "
"  document.body.innerHTML = '<div style=\"font-family:sans-serif; text-align:center; margin-top:50px;\"><h2>Guardando configuraciones...</h2><p>El dispositivo se reiniciará en unos momentos.</p></div>';"
"  "
"  try {"
"    let res = await fetch('/config', { method: 'POST', body: params });"
"    let txt = await res.text();"
"    document.body.innerHTML = '<div style=\"font-family:sans-serif; text-align:center; margin-top:50px;\"><h2>' + txt + '</h2></div>';"
"  } catch(e) {"
"    alert('Error al enviar configuraciones');"
"  }"
"}"

"async function uploadOTA() {"
"  const file = document.getElementById('firmware').files[0];"
"  if (!file) { alert('Selecciona un archivo .bin'); return; }"
"  const data = await file.arrayBuffer();"
"  document.getElementById('ota_status').innerText = 'Subiendo firmware...';"
"  try {"
"    const res = await fetch('/update', { method: 'POST', body: data });"
"    document.getElementById('ota_status').innerText = await res.text();"
"  } catch (err) { document.getElementById('ota_status').innerText = 'Error en actualización'; }"
"}"
"</script>"
"</body>"
"</html>";




//Cualquier URL desconocida debe redirigir a la pagina, no devolver 404
esp_err_t http_404_handler(httpd_req_t *req, httpd_err_code_t err)
{
 //abre la pagina automaticamente al conectarse a la red

    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

//handlers de scaneo de redes
esp_err_t scan_get_handler(httpd_req_t *req)
{
    wifi_scan_config_t scan_cfg = {
        .show_hidden = false,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time.active.min = 50,
        .scan_time.active.max = 100
    };

    esp_wifi_scan_start(&scan_cfg, false); 

    httpd_resp_set_type(req, "text/plain");
    httpd_resp_set_hdr(req, "Connection", "keep-alive");
    const char* resp = "OK";
    httpd_resp_send(req, resp, strlen(resp)); 
    return ESP_OK;
}

esp_err_t scan_result_handler(httpd_req_t *req)
{
    uint16_t ap_num = 0;
    esp_wifi_scan_get_ap_num(&ap_num);

    if (ap_num > 12) ap_num = 12; 

    wifi_ap_record_t *ap_records = malloc(ap_num * sizeof(wifi_ap_record_t));
    if (!ap_records) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No mem");
        return ESP_FAIL;
    }

    esp_wifi_scan_get_ap_records(&ap_num, ap_records);

    char *json = malloc(2048);
    if (!json) {
        free(ap_records);
        return ESP_FAIL;
    }

    int len = snprintf(json, 2048, "[");
    for (int i = 0; i < ap_num; i++) {
        if (len > 1900) break; 
        len += snprintf(json + len, 2048 - len,
            "{\"ssid\":\"%s\",\"rssi\":%d}%s",
            (char*)ap_records[i].ssid,
            ap_records[i].rssi,
            (i == ap_num - 1) ? "" : ",");
    }
    snprintf(json + len, 2048 - len, "]");

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, strlen(json));

    free(json);
    free(ap_records);
    return ESP_OK;
}


esp_err_t gen204_handler(httpd_req_t *req)  //samsung
{
    httpd_resp_set_status(req, "204 No Content");  //"no hay internet pero si wifi"
    httpd_resp_send(req, NULL, 0);  //envia 0 bytes
    return ESP_OK;
}

//handler del GET
esp_err_t root_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");  //al abrir el portal cautivo, envia html
    httpd_resp_send(req, WIFI_FORM_PAGE, HTTPD_RESP_USE_STRLEN);  //html envio 1 sola vez
    return ESP_OK;
}

esp_err_t status_get_handler(httpd_req_t *req)
{
    char json[256];  
    char mac_str[18];
    int rssi = 0;
    float batt = leer_ON_OFF();
    batt = filtro_iir(batt);

    float usb = leer_5v(); //estado conexion (usb o bat)
    char estado_conexion[32];

    int water_temp = leer_temperatura();
    int oil_pressure = leer_aceite();
    int horimetro = hourmeter_get_total_hours();

    char batt_dec[32];
snprintf(batt_dec, sizeof(batt_dec), "%.2f", batt);

    // Formatear la MAC de bytes a texto legible
    snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac_para_ap[0], mac_para_ap[1], mac_para_ap[2],
             mac_para_ap[3], mac_para_ap[4], mac_para_ap[5]);

    wifi_sta_list_t sta_list;
    if (esp_wifi_ap_get_sta_list(&sta_list) == ESP_OK && sta_list.num > 0) {
        rssi = sta_list.sta[0].rssi;
    }

    if(usb<3.5){

snprintf(estado_conexion, sizeof(estado_conexion), "Bateria");

    }
    if(usb>3.5){

snprintf(estado_conexion, sizeof(estado_conexion), "USB");

    }

    // JSON corregido: mac_str en lugar de mac_para_ap
    snprintf(json, sizeof(json),
             "{\"mac\":\"%s\",\"rssi\":%d,\"bat\":%s,\"conexion\":\"%s\","
             "\"w_temp\":%d,\"o_press\":%d,\"horimetro\":%d}",
             mac_str,
             rssi,
             batt_dec,
             estado_conexion,
             water_temp,
             oil_pressure,
             horimetro);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, strlen(json));
    return ESP_OK;
}

//handler del post, lector de datos recibidos del formulario
esp_err_t config_handler(httpd_req_t *req)
{
    char buf[768];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);

    if (ret <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No data");
        return ESP_FAIL;
    }
    buf[ret] = 0;

    char ssid[32] = {0};
    char pass[64] = {0};
    char ip[16] = {0};
    char gw[16] = {0};
    char mask[16] = {0};
    char dns[16] = {0};
    char static_flag[10] = {0};
    
    char mqtt_host[96] = {0};
    char mqtt_port_str[8] = {0};
    char mqtt_user[64] = {0};
    char mqtt_pass[64] = {0};

    // Intentar extraer los campos de WiFi
    httpd_query_key_value(buf, "ssid", ssid, sizeof(ssid));
    
    // Si el SSID viene presente en la petición, procesamos y guardamos WiFi
    if (strlen(ssid) > 0) {
        httpd_query_key_value(buf, "pass", pass, sizeof(pass));
        save_wifi_creds(ssid, pass);

        bool use_static = (httpd_query_key_value(buf, "use_static", static_flag, sizeof(static_flag)) == ESP_OK);
        if (use_static) {
            httpd_query_key_value(buf, "ip", ip, sizeof(ip));
            httpd_query_key_value(buf, "gateway", gw, sizeof(gw));
            httpd_query_key_value(buf, "netmask", mask, sizeof(mask));
            httpd_query_key_value(buf, "dns", dns, sizeof(dns));
            
            if (strlen(ip) >= 7) {
                save_static_ip(ip, gw, mask, dns);
            }
        } else {
            clear_static_ip();
        }
        ESP_LOGI("NVS", "WiFi actualizado. MQTT intacto.");
    }

    // Intentar extraer los campos de MQTT
    httpd_query_key_value(buf, "mqtt_host", mqtt_host, sizeof(mqtt_host));
    
    // Si el Host MQTT viene presente en la petición, procesamos y guardamos MQTT
    if (strlen(mqtt_host) > 0) {
        httpd_query_key_value(buf, "mqtt_port", mqtt_port_str, sizeof(mqtt_port_str));
        httpd_query_key_value(buf, "mqtt_user", mqtt_user, sizeof(mqtt_user));
        httpd_query_key_value(buf, "mqtt_pass", mqtt_pass, sizeof(mqtt_pass));

        int mqtt_port = atoi(mqtt_port_str);
        if (mqtt_port <= 0) mqtt_port = 1883;

        save_mqtt_creds(mqtt_host, mqtt_port, mqtt_user, mqtt_pass);
        ESP_LOGI("NVS", "MQTT actualizado. WiFi intacto.");
    }

    httpd_resp_sendstr(req, "Configuracion guardada con exito. Reiniciando dispositivo...");

    
   // Obtienes los segundos actuales usando la función pública del horómetro
uint32_t segundos_actuales = hourmeter_get_total_seconds();

// Llamas a tu función de guardado con la clave y el valor exacto
nvs_save_u32("hm_sec", segundos_actuales);

// Pausa breve opcional para asegurar commit en Flash y reinicias
vTaskDelay(pdMS_TO_TICKS(50));
esp_restart();

    return ESP_OK;
}


esp_err_t ota_web_handler(httpd_req_t *req) //ota web
{
    // (Tu código de configuración y bucle while de lectura se queda igual...)
    esp_ota_handle_t ota_handle;
    const esp_partition_t *ota_partition =
        esp_ota_get_next_update_partition(NULL);

    if (!ota_partition) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No OTA partition");
        return ESP_FAIL;
    }

    if (esp_ota_begin(ota_partition, OTA_SIZE_UNKNOWN, &ota_handle) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA begin failed");
        return ESP_FAIL;
    }

    char buf[1024];
    int received;
    int total = 0;

    while ((received = httpd_req_recv(req, buf, sizeof(buf))) > 0) {
        if (esp_ota_write(ota_handle, buf, received) != ESP_OK) {
            esp_ota_abort(ota_handle);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA write failed");
            return ESP_FAIL;
        }
        total += received;
    }

    if (esp_ota_end(ota_handle) != ESP_OK ||
        esp_ota_set_boot_partition(ota_partition) != ESP_OK) {

        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA finalize failed");
        return ESP_FAIL;
    }

    // ==========================================
    // CAMBIO CLAVE AQUÍ:
    // ==========================================
    
    // 1. Enviar la respuesta de éxito al navegador
    httpd_resp_sendstr(req, "Firmware actualizado con exito. Reiniciando...");

    // 2. Guardar el horómetro en NVS
    uint32_t segundos_actuales = hourmeter_get_total_seconds();
    nvs_save_u32("hm_sec", segundos_actuales);

    // 3. Dar un respiro de 250ms para que la red envíe el paquete HTTP 
    // antes de apagar el Wi-Fi y reiniciar el sistema.
    vTaskDelay(pdMS_TO_TICKS(250));

    esp_restart();
    return ESP_OK;
}

//inicializacion del servidor
httpd_handle_t start_webserver(void)  //servidor web del esp32
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 16384;
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK)
    {

        httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, http_404_handler);
        //redireccion de toda direccion a la ip del servidor


        // Handler GET /
        httpd_uri_t uri_root = {
            .uri = "/",            
            .method = HTTP_GET,          //se muestra el formulario al ingresar al servidor
            .handler = root_get_handler,
        };
        httpd_register_uri_handler(server, &uri_root);

        httpd_uri_t uri_status = {   //mostrar datos en html
    .uri = "/status",
    .method = HTTP_GET,
    .handler = status_get_handler,
};
httpd_register_uri_handler(server, &uri_status);  

        // Handler POST /config único
        httpd_uri_t uri_post = {
            .uri = "/config",
            .method = HTTP_POST,
            .handler = config_handler,
        };
        httpd_register_uri_handler(server, &uri_post);
     

httpd_uri_t uri_google = {  //samsung
    .uri = "/gen_204",
    .method = HTTP_GET,
    .handler = gen204_handler,
};
httpd_register_uri_handler(server, &uri_google);  


//escaneo de redes
httpd_uri_t uri_scan = {
    .uri = "/scan",
    .method = HTTP_GET,
    .handler = scan_get_handler,
};
httpd_register_uri_handler(server, &uri_scan);

httpd_uri_t uri_scan_result = {
    .uri = "/scan_result",
    .method = HTTP_GET,
    .handler = scan_result_handler,
};
httpd_register_uri_handler(server, &uri_scan_result);


//ota
httpd_uri_t ota_uri = {
    .uri = "/update",
    .method = HTTP_POST,
    .handler = ota_web_handler,
};
httpd_register_uri_handler(server, &ota_uri);



        ESP_LOGI("WEB", "Servidor HTTP iniciado.");
    }
    else
    {
        ESP_LOGE("WEB", "No se pudo iniciar el servidor HTTP.");
    }

    return server;
}