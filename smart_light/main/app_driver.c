/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "iot_button.h"
#include "light_driver.h"

#include "board_esp32c3_devkitc.h"
#include "app_priv.h"


#define TAG "app_driver"

static uint16_t g_hue = DEFAULT_HUE;
static uint16_t g_saturation = DEFAULT_SATURATION;
static uint16_t g_value = DEFAULT_BRIGHTNESS;
static bool g_output_state = true;
static bool g_power = DEFAULT_POWER;

#if APP_USE_LED_EXTERNAL

static void push_btn_cb(void *arg)
{
    app_driver_set_state(!g_output_state);
}

esp_err_t app_light_set_led(uint32_t hue, uint32_t saturation, uint32_t brightness)
{
    light_driver_set_hsv(hue, saturation, brightness);
    return ESP_OK;
}

void app_driver_init()
{
    /* Configure push button */
    button_config_t btn_cfg = {
        .type = BUTTON_TYPE_GPIO,
        .gpio_button_config = {
            .gpio_num = LIGHT_BUTTON_GPIO,
            .active_level = LIGHT_BUTTON_ACTIVE_LEVEL,
        },
    };
    button_handle_t btn_handle = iot_button_create(&btn_cfg);
    if (btn_handle)
    {
        /* Register a callback for a button short press event */
        iot_button_register_cb(btn_handle, BUTTON_SINGLE_CLICK, push_btn_cb);
    }

    /**
     * @brief Light driver initialization
     */
    light_driver_config_t driver_config = {
        .gpio_red = LIGHT_GPIO_RED,
        .gpio_green = LIGHT_GPIO_GREEN,
        .gpio_blue = LIGHT_GPIO_BLUE,
        .gpio_cold = LIGHT_GPIO_COLD,
        .gpio_warm = LIGHT_GPIO_WARM,
        .fade_period_ms = LIGHT_FADE_PERIOD_MS,
        .blink_period_ms = LIGHT_BLINK_PERIOD_MS,
        .freq_hz = LIGHT_FREQ_HZ,
        .clk_cfg = LEDC_USE_APB_CLK,
        .duty_resolution = LEDC_TIMER_13_BIT,
    };
    ESP_ERROR_CHECK(light_driver_init(&driver_config));
    light_driver_set_switch(true);
}

int IRAM_ATTR app_driver_set_state(bool state)
{
    if (g_output_state != state)
    {
        g_output_state = state;
        if (g_output_state)
        {
            // light on
            ESP_LOGI(TAG, "Light ON");
            // light_driver_set_switch(true);
            light_driver_fade_brightness(100);
        }
        else
        {
            // light off
            ESP_LOGI(TAG, "Light OFF");
            // light_driver_set_switch(false);
            light_driver_fade_brightness(0);
        }
    }
    return ESP_OK;
}
bool app_driver_set_rgb(uint8_t red, uint8_t green, uint8_t blue)
{
    light_driver_set_rgb(red, green, blue);
    return true;
}

bool app_driver_get_state(void)
{
    return g_output_state;
}

esp_err_t app_light_set_brightness(uint16_t brightness)
{
    g_value = brightness;
    return app_light_set_led(g_hue, g_saturation, g_value);
}
esp_err_t app_light_set_hue(uint16_t hue)
{
    g_hue = hue;
    return app_light_set_led(g_hue, g_saturation, g_value);
}
esp_err_t app_light_set_saturation(uint16_t saturation)
{
    g_saturation = saturation;
    return app_light_set_led(g_hue, g_saturation, g_value);
}

#else

esp_err_t app_light_set_led(uint32_t hue, uint32_t saturation, uint32_t brightness)
{
    /* Whenever this function is called, light power will be ON */
    if (!g_power) {
        g_power = true;
        esp_rmaker_param_update_and_report(
                esp_rmaker_device_get_param_by_type(light_device, ESP_RMAKER_PARAM_POWER),
                esp_rmaker_bool(g_power));
    }
    return ws2812_led_set_hsv(hue, saturation, brightness);
}

esp_err_t app_light_set_power(bool power)
{
    g_power = power;
    if (power) {
        ws2812_led_set_hsv(g_hue, g_saturation, g_value);
    } else {
        ws2812_led_clear();
    }
    return ESP_OK;
}

esp_err_t app_light_init(void)
{
    esp_err_t err = ws2812_led_init();
    if (err != ESP_OK) {
        return err;
    }
    if (g_power) {
        ws2812_led_set_hsv(g_hue, g_saturation, g_value);
    } else {
        ws2812_led_clear();
    }
    return ESP_OK;
}

esp_err_t app_light_set_brightness(uint16_t brightness)
{
    g_value = brightness;
    return app_light_set_led(g_hue, g_saturation, g_value);
}
esp_err_t app_light_set_hue(uint16_t hue)
{
    g_hue = hue;
    return app_light_set_led(g_hue, g_saturation, g_value);
}
esp_err_t app_light_set_saturation(uint16_t saturation)
{
    g_saturation = saturation;
    return app_light_set_led(g_hue, g_saturation, g_value);
}

static void push_btn_cb(void *arg)
{
    app_light_set_power(!g_power);
    esp_rmaker_param_update_and_report(
            esp_rmaker_device_get_param_by_type(light_device, ESP_RMAKER_PARAM_POWER),
            esp_rmaker_bool(g_power));
}

void app_driver_init()
{
    app_light_init();
    // button_handle_t btn_handle = iot_button_create(BUTTON_GPIO, BUTTON_ACTIVE_LEVEL);
    // if (btn_handle) {
        /* Register a callback for a button tap (short press) event */
        // iot_button_set_evt_cb(btn_handle, BUTTON_CB_TAP, push_btn_cb, NULL);
        /* Register Wi-Fi reset and factory reset functionality on same button */
        // app_reset_button_register(btn_handle, WIFI_RESET_BUTTON_TIMEOUT, FACTORY_RESET_BUTTON_TIMEOUT);
    // }
}

#endif