# I2C EEPROM driver for the ESP32 microcontroller
Driver for reading and writing data to external I2C EEPROMs. A fully reworked version of [this repo](https://github.com/zacharyvincze/esp32-i2c-eeprom).

# What works, what doesn't?
- This has been specifically designed to work with AT24CM02. As such, it follows the datasheet given [here](https://ww1.microchip.com/downloads/en/DeviceDoc/AT24CM02-I2C-Compatible-Two-Wire-Serial-EEPROM-2-Mbit-262,144x8-20006197C.pdf1). 
I'm fairly certain this is standard across the Microchip family of eeproms, but make no guarantees. If you'd like to 
pick up the project and extend it to your needs, feel free to do so!
- What works:
  - Byte writes
  - Byte reads
  - Page writes (256 bytes)
  - Sequential reads
- What doesn't work:
  - ???
