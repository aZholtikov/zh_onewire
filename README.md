# ESP32 ESP-IDF and ESP8266 RTOS SDK component for 1-Wire interface

## Tested on

1. ESP8266 RTOS_SDK v3.4
2. ESP32 ESP-IDF v5.1.0

## Using

In an existing project, run the following command to install the component:

```text
cd ../your_project/components
git clone http://git.zh.com.ru/alexey.zholtikov/zh_onewire.git
```

In the application, add the component:

```c
#include "zh_onewire.h"
```

## Example

Search 1-Wire devices on bus:

```c
#include "zh_onewire.h"

void app_main(void)
{
    uint8_t *rom = NULL;
    zh_onewire_init(GPIO_NUM_5);
    if (zh_onewire_reset() != ESP_OK)
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
