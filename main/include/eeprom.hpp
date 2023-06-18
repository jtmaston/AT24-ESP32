#ifndef EEPROM_H
#define EEPROM_H

// This has been tested with an AT24CM02 series eeprom. While it sticks to Microchip's spec, your mileage may vary
// The datasheet referenced in order to construct the commands is available at
// https://ww1.microchip.com/downloads/en/DeviceDoc/AT24CM02-I2C-Compatible-Two-Wire-Serial-EEPROM-2-Mbit-262,144x8-20006197C.pdf


#include <stdio.h>
#include "driver/i2c.h"
#include "esp_log.h"

static const char* EEPROM_TAG = "EEPROM";

// #define M_PERFORMANCE_METRICS      // uncomment to enable performance metrics of write functions

#ifdef M_PERFORMANCE_METRICS
#include "esp_timer.h"
#endif

#ifdef M_USE_MUTEX

#include <mutex>
extern std::mutex M_MUTEX_PROTECTOR;

#endif

class EEPROM{
public:
    EEPROM(uint16_t addr, uint32_t baseAddress, uint32_t topAddress, uint16_t pageSize, bool initBus = false,
           uint8_t scl = 22, uint8_t sda = 21, uint8_t num = 0, uint32_t freq = 4e5, bool pullUp = true );           // constructor, will initialize the bus
                                                                            // if it has not been init
    ~EEPROM();                                                              // destructor, will free any memory used

    void write(uint16_t addr, uint8_t byte);                                // write a single byte to address
    void write(uint16_t baseAddr, uint8_t* byteArray, uint16_t size);       // write a bytearray to address

    uint8_t read(uint16_t addr);                                            // read a single byte
    uint8_t* read(uint16_t addr, uint16_t size);                            // read a bytearray into buffer, then return
                                                                            // a pointer to it

    uint8_t* dump();
    void format(uint8_t filler = 0);

private:
    uint16_t address;                                   // address of the eeprom on the bus
    uint32_t baseAddress = 0x0;                         // the base address of the eeprom, should be 0
    uint32_t topAddress;                                // the top address, represents eeprom size
    uint16_t pageSize;                                   // the pagesize, for page reads
    uint8_t* readBuffer;                                // internal buffer used in reading
    uint16_t bufferSize;                                // the size of the buffer at the last allocation
    i2c_port_t portNum;
    static void parseError(esp_err_t errCode);  // print the error code
};
#endif