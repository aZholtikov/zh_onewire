#include "zh_onewire.h"

#define MASTER_RESET_PULSE_DURATION 480 // Reset time high. Reset time low.
#define RESPONSE_MAX_DURATION 60        // Presence detect high.
#define PRESENCE_PULSE_MAX_DURATION 240 // Presence detect low.
#define RECOVERY_DURATION 1             // Bus recovery time.
#define TIME_SLOT_START_DURATION 1      // Time slot start.
#define TIME_SLOT_DURATION 120          // Time slot.
#define VALID_DATA_DURATION 15          // Valid data duration.

#define SKIP_ROM 0xCC   // Skip ROM command to address all 1-Wire devices.
#define MATCH_ROM 0x55  // Match ROM command to address a specific 1-Wire device.
#define READ_ROM 0x33   // Read ROM command for read ROM on only one 1-Wire device.
#define SEARCH_ROM 0xF0 // Read ROM on all 1-Wire devices.

#define pgm_read_byte(addr) (*(const uint8_t *)(addr))

static uint8_t _read_bit(void);
static void _send_bit(const uint8_t bit);

#ifdef CONFIG_IDF_TARGET_ESP8266
#define esp_delay_us(x) os_delay_us(x)
#else
#define esp_delay_us(x) esp_rom_delay_us(x)
#endif

static const char *TAG = "zh_onewire";

static uint8_t _pin;
static uint8_t _rom[8];
static uint8_t _rom_fork_bit = 0xFF;
static bool _is_initialized = false;

static const uint8_t _rom_crc_table[] = {
    0x00, 0x5e, 0xbc, 0xe2, 0x61, 0x3f, 0xdd, 0x83, 0xc2, 0x9c, 0x7e, 0x20, 0xa3, 0xfd, 0x1f, 0x41,
    0x9d, 0xc3, 0x21, 0x7f, 0xfc, 0xa2, 0x40, 0x1e, 0x5f, 0x01, 0xe3, 0xbd, 0x3e, 0x60, 0x82, 0xdc,
    0x23, 0x7d, 0x9f, 0xc1, 0x42, 0x1c, 0xfe, 0xa0, 0xe1, 0xbf, 0x5d, 0x03, 0x80, 0xde, 0x3c, 0x62,
    0xbe, 0xe0, 0x02, 0x5c, 0xdf, 0x81, 0x63, 0x3d, 0x7c, 0x22, 0xc0, 0x9e, 0x1d, 0x43, 0xa1, 0xff,
    0x46, 0x18, 0xfa, 0xa4, 0x27, 0x79, 0x9b, 0xc5, 0x84, 0xda, 0x38, 0x66, 0xe5, 0xbb, 0x59, 0x07,
    0xdb, 0x85, 0x67, 0x39, 0xba, 0xe4, 0x06, 0x58, 0x19, 0x47, 0xa5, 0xfb, 0x78, 0x26, 0xc4, 0x9a,
    0x65, 0x3b, 0xd9, 0x87, 0x04, 0x5a, 0xb8, 0xe6, 0xa7, 0xf9, 0x1b, 0x45, 0xc6, 0x98, 0x7a, 0x24,
    0xf8, 0xa6, 0x44, 0x1a, 0x99, 0xc7, 0x25, 0x7b, 0x3a, 0x64, 0x86, 0xd8, 0x5b, 0x05, 0xe7, 0xb9,
    0x8c, 0xd2, 0x30, 0x6e, 0xed, 0xb3, 0x51, 0x0f, 0x4e, 0x10, 0xf2, 0xac, 0x2f, 0x71, 0x93, 0xcd,
    0x11, 0x4f, 0xad, 0xf3, 0x70, 0x2e, 0xcc, 0x92, 0xd3, 0x8d, 0x6f, 0x31, 0xb2, 0xec, 0x0e, 0x50,
    0xaf, 0xf1, 0x13, 0x4d, 0xce, 0x90, 0x72, 0x2c, 0x6d, 0x33, 0xd1, 0x8f, 0x0c, 0x52, 0xb0, 0xee,
    0x32, 0x6c, 0x8e, 0xd0, 0x53, 0x0d, 0xef, 0xb1, 0xf0, 0xae, 0x4c, 0x12, 0x91, 0xcf, 0x2d, 0x73,
    0xca, 0x94, 0x76, 0x28, 0xab, 0xf5, 0x17, 0x49, 0x08, 0x56, 0xb4, 0xea, 0x69, 0x37, 0xd5, 0x8b,
    0x57, 0x09, 0xeb, 0xb5, 0x36, 0x68, 0x8a, 0xd4, 0x95, 0xcb, 0x29, 0x77, 0xf4, 0xaa, 0x48, 0x16,
    0xe9, 0xb7, 0x55, 0x0b, 0x88, 0xd6, 0x34, 0x6a, 0x2b, 0x75, 0x97, 0xc9, 0x4a, 0x14, 0xf6, 0xa8,
    0x74, 0x2a, 0xc8, 0x96, 0x15, 0x4b, 0xa9, 0xf7, 0xb6, 0xe8, 0x0a, 0x54, 0xd7, 0x89, 0x6b, 0x35};

esp_err_t zh_onewire_init(const uint8_t pin)
{
    ESP_LOGI(TAG, "Onewire initialization begin.");
    _pin = pin;
    gpio_config_t config;
    config.intr_type = GPIO_INTR_DISABLE;
    config.mode = GPIO_MODE_INPUT;
    config.pin_bit_mask = (1ULL << _pin);
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.pull_up_en = GPIO_PULLUP_ENABLE;
    if (gpio_config(&config) != ESP_OK)
    {
        ESP_LOGE(TAG, "Onewire initialization fail. Incorrect GPIO number.");
        return ESP_ERR_INVALID_ARG;
    }
    _is_initialized = true;
    ESP_LOGI(TAG, "Onewire initialization success.");
    return ESP_OK;
}

esp_err_t zh_onewire_reset(void)
{
    ESP_LOGI(TAG, "Onewire reset begin.");
    if (_is_initialized == false)
    {
        ESP_LOGE(TAG, "Onewire reset fail. Onewire not initialized.");
        return ESP_ERR_INVALID_STATE;
    }
    if (gpio_get_level(_pin) != 1)
    {
        ESP_LOGE(TAG, "Onewire reset fail. Bus is busy.");
        return ESP_ERR_INVALID_RESPONSE;
    }
    gpio_set_direction(_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(_pin, 0);
    esp_delay_us(MASTER_RESET_PULSE_DURATION);
    gpio_set_level(_pin, 1);
    gpio_set_direction(_pin, GPIO_MODE_INPUT);
    esp_delay_us(RECOVERY_DURATION);
    uint8_t response_time = 0;
    while (gpio_get_level(_pin) == 1)
    {
        if (response_time > RESPONSE_MAX_DURATION)
        {
            ESP_LOGE(TAG, "Onewire reset fail. Timeout exceeded.");
            return ESP_ERR_TIMEOUT;
        }
        ++response_time;
        esp_delay_us(1);
    }
    uint8_t presence_time = 0;
    while (gpio_get_level(_pin) == 0)
    {
        if (presence_time > PRESENCE_PULSE_MAX_DURATION)
        {
            ESP_LOGE(TAG, "Onewire reset fail. Timeout exceeded.");
            return ESP_ERR_TIMEOUT;
        }
        ++presence_time;
        esp_delay_us(1);
    }
    esp_delay_us(MASTER_RESET_PULSE_DURATION - response_time - presence_time);
    ESP_LOGI(TAG, "Onewire reset success.");
    return ESP_OK;
}

void zh_onewire_send_byte(uint8_t byte)
{
    for (uint8_t i = 0; i < 8; ++i)
    {
        _send_bit(byte & 1);
        byte >>= 1;
    }
}

esp_err_t zh_onewire_skip_rom(void)
{
    ESP_LOGI(TAG, "Onewire skip ROM begin.");
    if (_is_initialized == false)
    {
        ESP_LOGE(TAG, "Onewire skip ROM fail. Onewire not initialized.");
        return ESP_ERR_INVALID_STATE;
    }
    if (zh_onewire_reset() != ESP_OK)
    {
        ESP_LOGE(TAG, "Onewire skip ROM fail.");
        return ESP_FAIL;
    }
    zh_onewire_send_byte(SKIP_ROM);
    ESP_LOGI(TAG, "Onewire skip ROM success.");
    return ESP_OK;
}

esp_err_t zh_onewire_read_rom(uint8_t *buf)
{
    ESP_LOGI(TAG, "Onewire read ROM begin.");
    if (_is_initialized == false)
    {
        ESP_LOGE(TAG, "Onewire read ROM fail. Onewire not initialized.");
        return ESP_ERR_INVALID_STATE;
    }
    if (zh_onewire_reset() != ESP_OK)
    {
        ESP_LOGE(TAG, "Onewire read ROM fail.");
        return ESP_FAIL;
    }
    zh_onewire_send_byte(READ_ROM);
    uint8_t crc = 0;
    for (uint8_t i = 0; i < 8; ++i)
    {
        *buf = zh_onewire_read_byte();
        crc = pgm_read_byte(&_rom_crc_table[crc ^ *buf]);
        ++buf;
    }
    if (crc != 0)
    {
        ESP_LOGE(TAG, "Onewire read ROM fail. Invalid CRC.");
        return ESP_ERR_INVALID_CRC;
    }
    ESP_LOGI(TAG, "Onewire read ROM success.");
    return ESP_OK;
}

esp_err_t zh_onewire_match_rom(const uint8_t *data)
{
    ESP_LOGI(TAG, "Onewire match ROM begin.");
    if (_is_initialized == false)
    {
        ESP_LOGE(TAG, "Onewire match ROM fail. Onewire not initialized.");
        return ESP_ERR_INVALID_STATE;
    }
    if (zh_onewire_reset() != ESP_OK)
    {
        ESP_LOGE(TAG, "Onewire match ROM fail.");
        return ESP_FAIL;
    }
    zh_onewire_send_byte(MATCH_ROM);
    for (uint8_t i = 0; i < 8; ++i)
    {
        zh_onewire_send_byte(*(data++));
    }
    ESP_LOGI(TAG, "Onewire match ROM success.");
    return ESP_OK;
}

esp_err_t zh_onewire_search_rom_init(void)
{
    if (_is_initialized == false)
    {
        return ESP_ERR_INVALID_STATE;
    }
    for (uint8_t i = 0; i < 8; ++i)
    {
        _rom[i] = 0;
    }
    _rom_fork_bit = 65;
    return ESP_OK;
}

uint8_t *zh_onewire_search_rom_next(void)
{
    ESP_LOGI(TAG, "Onewire search ROM begin.");
    if (_is_initialized == false)
    {
        ESP_LOGE(TAG, "Onewire search ROM fail. Onewire not initialized.");
        return NULL;
    }
    if (_rom_fork_bit == 0xFF)
    {
        ESP_LOGE(TAG, "Onewire search ROM not initialized.");
        return NULL;
    }
    if (_rom_fork_bit == 0)
    {
        ESP_LOGI(TAG, "Onewire search ROM terminated.");
        return NULL;
    }
    if (zh_onewire_reset() != ESP_OK)
    {
        ESP_LOGE(TAG, "Onewire search ROM fail.");
        return NULL;
    }
    uint8_t bit_position = 8;
    uint8_t *p_prev_bit = &_rom[0];
    uint8_t prev_bit = *p_prev_bit;
    uint8_t next_bit = 0;
    uint8_t counter = 1;
    zh_onewire_send_byte(SEARCH_ROM);
    uint8_t new_fork = 0;
    for (;;)
    {
        uint8_t bit_not_0 = _read_bit();
        uint8_t bit_not_1 = _read_bit();
        if (bit_not_0 == 0)
        {
            if (bit_not_1 == 0)
            {
                if (counter < _rom_fork_bit)
                {
                    if (prev_bit == 1)
                    {
                        next_bit |= 0x80;
                    }
                    else
                    {
                        new_fork = counter;
                    }
                }
                else if (counter == _rom_fork_bit)
                {
                    next_bit |= 0x80;
                }
                else
                {
                    new_fork = counter;
                }
            }
        }
        else
        {
            if (bit_not_1 == 0)
            {
                next_bit |= 0x80;
            }
            else
            {
                ESP_LOGI(TAG, "Onewire search ROM terminated.");
                return NULL;
            }
        }
        _send_bit(next_bit & 0x80);
        --bit_position;
        if (bit_position == 0)
        {
            *p_prev_bit = next_bit;
            if (counter >= 64)
            {
                break;
            }
            next_bit = 0;
            ++p_prev_bit;
            prev_bit = *p_prev_bit;
            bit_position = 8;
        }
        else
        {
            if (counter >= 64)
            {
                break;
            }
            prev_bit >>= 1;
            next_bit >>= 1;
        }
        ++counter;
    }
    _rom_fork_bit = new_fork;
    ESP_LOGI(TAG, "Onewire search ROM success.");
    return &_rom[0];
}

uint8_t zh_onewire_read_byte(void)
{
    uint8_t byte = 0;
    for (uint8_t i = 0; i < 8; ++i)
    {
        byte >>= 1;
        if (_read_bit() != 0)
        {
            byte |= 0x80;
        }
    }
    return byte;
}

static uint8_t _read_bit(void)
{
    gpio_set_direction(_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(_pin, 0);
    esp_delay_us(TIME_SLOT_START_DURATION);
    gpio_set_level(_pin, 1);
    gpio_set_direction(_pin, GPIO_MODE_INPUT);
    esp_delay_us(VALID_DATA_DURATION - TIME_SLOT_START_DURATION);
    uint8_t bit = gpio_get_level(_pin);
    esp_delay_us(TIME_SLOT_DURATION - RECOVERY_DURATION - VALID_DATA_DURATION);
    return bit;
}

static void _send_bit(const uint8_t bit)
{
    gpio_set_direction(_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(_pin, 0);
    if (bit == 0)
    {
        esp_delay_us(TIME_SLOT_DURATION);
        gpio_set_level(_pin, 1);
        gpio_set_direction(_pin, GPIO_MODE_INPUT);
        esp_delay_us(RECOVERY_DURATION);
    }
    else
    {
        esp_delay_us(TIME_SLOT_START_DURATION);
        gpio_set_level(_pin, 1);
        gpio_set_direction(_pin, GPIO_MODE_INPUT);
        esp_delay_us(TIME_SLOT_DURATION);
    }
}