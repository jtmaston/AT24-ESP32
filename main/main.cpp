#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "eeprom.hpp"

void eeprom_task(void *arg) {

    EEPROM eeprom (0x50, 0x0, 0x1000, 256, true);

    // EEPROM single byte write example
    uint8_t starting_address = 0x23;
    uint8_t single_write_byte = 0x23;
    eeprom.write(starting_address, single_write_byte);
    printf("Wrote byte 0x%02X to address 0x%04X\n", single_write_byte, starting_address);
    vTaskDelay(20/portTICK_PERIOD_MS);


    // EEPROM random read example
    uint8_t random_read_byte = eeprom.read(starting_address);
    printf("Read byte 0x%02X at address 0x%04X\n", random_read_byte, starting_address);
    vTaskDelay(20/portTICK_PERIOD_MS);

    // EEPROM page write example
    uint8_t page_write_data[] = "This is a simple test, heya!\0";

    eeprom.write(0, page_write_data, sizeof (page_write_data));
    printf("Wrote the following string to EEPROM: %s\n", page_write_data);

    vTaskDelay(20/portTICK_PERIOD_MS);

    // Sequential read example
    printf("Read the following string from EEPROM: ");
    uint8_t* pageReadData = eeprom.read(0x0, sizeof (page_write_data));
    for(int i = 0 ; i < sizeof (page_write_data); i++)
        printf("%c", pageReadData[i]);
    printf("\n");

    // EEPROM format example
    eeprom.format(0x35);


    // EEPROM dump example
    uint8_t* dump = eeprom.dump();
    for(int i = 0 ; i < 0x1000; i++)
    {
        printf("0x%02X ", dump[i]);
        if(i % 30 == 0 && i)
            printf("\n");
    }


    printf("\n");
    fflush(stdout);
    vTaskDelete(nullptr);
}


#include <esp_event.h>
extern "C" void app_main() {
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    xTaskCreate(&eeprom_task, "eeprom_read_write_demo", 4096 * 2, nullptr, tskIDLE_PRIORITY, nullptr);
}