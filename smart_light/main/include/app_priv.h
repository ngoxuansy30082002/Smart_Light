// Copyright 2020 Espressif Systems (Shanghai) Co. Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef __APP_PRIVATE_H__
#define __APP_PRIVATE_H__

#include <stdint.h>
#include <stdbool.h>

#define DEFAULT_POWER true
#define DEFAULT_HUE 180
#define DEFAULT_SATURATION 100
#define DEFAULT_BRIGHTNESS 25

#define APP_USE_LED_EXTERNAL    0

#if APP_USE_LED_EXTERNAL
/**
 * @brief
 *
 */
void app_driver_init(void);

/**
 * @brief
 *
 * @param state
 * @return int
 */
int app_driver_set_state(bool state);

/**
 * @brief
 *
 * @return true
 * @return false
 */
bool app_driver_get_state(void);
bool app_driver_set_rgb(uint8_t red, uint8_t green, uint8_t blue);

esp_err_t app_light_set_brightness(uint16_t brightness);
esp_err_t app_light_set_hue(uint16_t hue);
esp_err_t app_light_set_saturation(uint16_t saturation);

#else

#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_types.h>
#include <esp_rmaker_standard_params.h>

#include <ws2812_led.h>

#define DEFAULT_POWER       true
extern esp_rmaker_device_t *light_device;

void app_driver_init(void);
esp_err_t app_light_set(uint32_t hue, uint32_t saturation, uint32_t brightness);
esp_err_t app_light_set_power(bool power);
esp_err_t app_light_set_brightness(uint16_t brightness);
esp_err_t app_light_set_hue(uint16_t hue);
esp_err_t app_light_set_saturation(uint16_t saturation);

#endif



#endif /**< __APP_PRIVATE_H__ */
