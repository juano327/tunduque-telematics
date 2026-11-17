#ifndef http_control_H_
#define http_control_H_

esp_err_t gen204_handler(httpd_req_t *req);
esp_err_t status_get_handler(httpd_req_t *req);
esp_err_t scan_get_handler(httpd_req_t *req);
esp_err_t scan_result_handler(httpd_req_t *req);
esp_err_t root_get_handler(httpd_req_t *req);
esp_err_t config_handler(httpd_req_t *req);
esp_err_t ota_web_handler(httpd_req_t *req);
httpd_handle_t start_webserver(void);
esp_err_t http_404_handler(httpd_req_t *req, httpd_err_code_t err);

#endif