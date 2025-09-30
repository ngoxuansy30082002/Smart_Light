/* LED Light Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include <nvs_flash.h>

#include "lwip/sockets.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include <esp_rmaker_console.h>
#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_params.h>
#include <esp_rmaker_standard_devices.h>
#include <esp_rmaker_schedule.h>
#include <esp_rmaker_console.h>
#include <esp_rmaker_scenes.h>

#include "mdns.h"
#include "esp_local_ctrl.h"
#include <esp_https_server.h>

#include "app_storage.h"
#include <app_network.h>
#include "app_priv.h"

static const char *TAG = "app_main";
esp_err_t err;

#define SERVICE_NAME "culi"

/* ----------------------- HTTPs server --------------------------------*/

static char buf[100] = "{\"status\": true}";
static esp_err_t esp_light_get_handler(httpd_req_t *req)
{
    httpd_resp_send(req, buf, strlen(buf));
    return ESP_OK;
}

static esp_err_t esp_light_set_handler(httpd_req_t *req)
{
    int ret, remaining = req->content_len;
    memset(buf, 0 ,sizeof(buf));
    while (remaining > 0) {
        if ((ret = httpd_req_recv(req, buf, remaining)) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            return ESP_FAIL;
        }
        remaining -= ret;
    }
    ESP_LOGI(TAG, "%.*s", req->content_len, buf);
    return ESP_OK;
}

static const httpd_uri_t status = {
    .uri       = "/light",
    .method    = HTTP_GET,
    .handler   = esp_light_get_handler,
};

static const httpd_uri_t ctrl = {
    .uri       = "/light",
    .method    = HTTP_POST,
    .handler   = esp_light_set_handler,
};

static esp_err_t esp_start_webserver()
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &status);
        httpd_register_uri_handler(server, &ctrl);
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return ESP_FAIL;
}

static esp_err_t root_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req,
    "<h1>Hello world, <span style='color:red; font-size:48px; font-weight:bold;'>WE ARE Culi Team!</span></h1>",
    HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
}

static const httpd_uri_t root = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = root_get_handler
};

static esp_err_t esp_create_https_server(void)
{
    httpd_handle_t server = NULL;

    ESP_LOGI(TAG, "Starting server");

    httpd_ssl_config_t conf = HTTPD_SSL_CONFIG_DEFAULT();

    extern const unsigned char servercert_crt_start[] asm("_binary_server_crt_start");
    extern const unsigned char servercert_crt_end[]   asm("_binary_server_crt_end");
    conf.servercert = servercert_crt_start;
    conf.servercert_len = servercert_crt_end - servercert_crt_start;

    extern const unsigned char server_privkey_pem_start[] asm("_binary_server_key_start");
    extern const unsigned char server_privkey_pem_end[]   asm("_binary_server_key_end");
    conf.prvtkey_pem = server_privkey_pem_start;
    conf.prvtkey_len = server_privkey_pem_end - server_privkey_pem_start;

    mdns_init();
    mdns_hostname_set(SERVICE_NAME);

    esp_err_t ret = httpd_ssl_start(&server, &conf);
    if (ESP_OK != ret) {
        ESP_LOGI(TAG, "Error starting server!");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Registering URI handlers");
    httpd_register_uri_handler(server, &root);
    return ESP_OK;
}

/* ----------------------- HTTPs server --------------------------------*/
void app_main()
{
    int i = 0;
    /* Initialize Application specific hardware drivers and
     * set initial state.
     */
    /**
     * @brief NVS Flash initialization
     */
    ESP_LOGI(TAG, "NVS Flash initialization");
    app_storage_init();

    /**
     * @brief Application driver initialization
     */
    ESP_LOGI(TAG, "Application driver initialization");
    app_driver_init();

    /* Initialize Wi-Fi/Thread. Note that, this should be called before esp_rmaker_node_init()
     */
    app_network_init();

    /* Initialize the ESP RainMaker Agent.
     * Note that this should be called after app_network_init() but before app_network_start()
     * */
    esp_rmaker_config_t rainmaker_cfg = {
        .enable_time_sync = false,
    };
    esp_rmaker_node_t *node = esp_rmaker_node_init(&rainmaker_cfg, "ESP RainMaker Device", "Lightbulb");
    if (!node)
    {
        ESP_LOGE(TAG, "Could not initialise node. Aborting!!!");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        abort();
    }

    /* Enable system service */
    esp_rmaker_system_serv_config_t system_serv_config = {
        .flags = SYSTEM_SERV_FLAGS_ALL,
        .reboot_seconds = 2,
        .reset_seconds = 2,
        .reset_reboot_seconds = 2,
    };
    esp_rmaker_system_service_enable(&system_serv_config);

    /* Start the ESP RainMaker Agent */
    esp_rmaker_start();

    err = app_network_set_custom_mfg_data(MGF_DATA_DEVICE_TYPE_LIGHT, MFG_DATA_DEVICE_SUBTYPE_LIGHT);
    /* Start the Wi-Fi/Thread.
     * If the node is provisioned, it will start connection attempts,
     * else, it will start Wi-Fi provisioning. The function will return
     * after a connection has been successfully established
     */
    err = app_network_start((app_network_pop_type_t)POP_TYPE_RANDOM);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Could not start network. Aborting!!!");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        abort();
    }
    esp_create_https_server();

    while (1)
    {
        ESP_LOGI(TAG, "[%02d] Hello world!", i++);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
