# ESP32 ESP-IDF component for 1-Wire interface

There are two branches - for ESP8266 family and for ESP32 family. Please use the appropriate one.

## Using

In an existing project, run the following command to install the component:

```text
cd ../your_project/components
git clone -b esp32 --recursive http://git.zh.com.ru/alexey.zholtikov/zh_onewire.git
```

In the application, add the component:

```c
#include "zh_onewire.h"
```

## Example

Search 1-Wire devices on bus:

```c
#include "stdio.h"
#include "zh_onewire.h"

void app_main(void)
{
    uint8_t *rom = NULL;
    ESP_ERROR_CHECK(zh_onewire_init(GPIO));
    if (zh_onewire_reset() == ESP_FAIL)
    {
        printf("There are no 1-Wire devices available on the bus.\n");
    }
    else
    {
        zh_onewire_search_rom_init();
        for (;;)
        {
            rom = zh_onewire_search_rom_next();
            if (rom == NULL)
            {
                break;
            }
            printf("Found device ROM: ");
            for (uint8_t i = 0; i < 8; ++i)
            {
                printf("%X ", *(rom++));
            }
            rom -= 8;
            printf("\n");
        }
    }
}
```