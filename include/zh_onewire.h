#pragma once

#include "stdio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"
#include "esp_err.h"
#include "esp_log.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Initialize 1-Wire interface.
     *
     * @param[in] pin 1-Wire bus gpio connection.
     *
     * @return
     *              - ESP_OK if initialization was successful
     *              - ESP_ERR_INVALID_ARG if select incorrect gpio number
     */
    esp_err_t zh_onewire_init(const uint8_t pin);

    /**
     * @brief Reset command for all 1-Wire devices on bus.
     *
     * @return
     *              - ESP_OK if reset was successful
     *              - ESP_ERR_INVALID_STATE if 1-Wire bus not initialized
     *              - ESP_ERR_INVALID_RESPONSE if the bus is busy
     *              - ESP_ERR_TIMEOUT if there are no 1-Wire devices available on the bus or the devices are not responding
     */
    esp_err_t zh_onewire_reset(void);

    /**
     * @brief Send one byte to 1-Wire device.
     *
     * @param[in] byte Byte value.
     */
    void zh_onewire_send_byte(uint8_t byte);

    /**
     * @brief Read one byte from 1-Wire device.
     *
     * @return Byte value
     */
    uint8_t zh_onewire_read_byte(void);

    /**
     * @brief Initialize the bus master to address all 1-Wire devices on the bus.
     *
     * @return
     *              - ESP_OK if initialization was successful
     *              - ESP_ERR_INVALID_STATE if 1-Wire bus not initialized
     *              - ESP_FAIL if there are no 1-Wire devices available on the bus or the devices are not responding
     */
    esp_err_t zh_onewire_skip_rom(void);

    /**
     * @brief Read rom value if only one 1-Wire device is present on the bus.
     *
     * @param[out] buf Pointer to a buffer containing an eight-byte rom value.
     *
     * @return
     *              - ESP_OK if read was successful
     *              - ESP_ERR_INVALID_STATE if 1-Wire bus not initialized
     *              - ESP_FAIL if there are no 1-Wire devices available on the bus or the devices are not responding
     *              - ESP_ERR_INVALID_CRC if more than one 1-Wire device is present on the bus
     */
    esp_err_t zh_onewire_read_rom(uint8_t *buf);

    /**
     * @brief Initialize the bus master to address a specific 1-Wire device on bus.
     *
     * @param[in] data Pointer to a buffer containing an eight-byte rom value.
     *
     * @return
     *              - ESP_OK if initialization was successful
     *              - ESP_ERR_INVALID_STATE if 1-Wire bus not initialized
     *              - ESP_FAIL if there are no 1-Wire devices available on the bus or the devices are not responding
     */
    esp_err_t zh_onewire_match_rom(const uint8_t *data);

    /**
     * @brief      Initialize search 1-Wire devices on bus.
     *
     * @return
     *              - ESP_OK if initialization was successful
     *              - ESP_ERR_INVALID_STATE if 1-Wire bus not initialized
     */
    esp_err_t zh_onewire_search_rom_init(void);

    /**
     * @brief Search next 1-Wire device on bus.
     *
     * @attention Initialize search 1-Wire devices on bus must be initialized first. @code zh_onewire_search_rom_init() @endcode
     *
     * @return
     *              - Pointer to a buffer containing an eight-byte rom value
     *              - NULL if the search is terminated or if there are no 1-Wire devices available on the bus or the devices are not responding or if 1-Wire bus not initialized
     */
    uint8_t *zh_onewire_search_rom_next(void);

#ifdef __cplusplus
}
#endif
