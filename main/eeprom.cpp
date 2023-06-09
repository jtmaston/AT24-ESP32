#include "eeprom.hpp"


EEPROM::EEPROM(uint16_t addr, uint32_t baseAddress, uint32_t topAddress, uint16_t pageSize, bool initBus,
               uint8_t scl, uint8_t sda, uint8_t num, uint32_t freq, bool pullUp) {
                // Default constructor, initializes the parameters of the eeprom and (optionally) the bus

    if (initBus) {
        i2c_config_t conf = {
                .mode = I2C_MODE_MASTER,
                .sda_io_num = sda,
                .scl_io_num = scl,
                .sda_pullup_en = pullUp,
                .scl_pullup_en = pullUp,
        };

        conf.master.clk_speed = freq;
        i2c_param_config((i2c_port_t) num, &conf);
        i2c_driver_install((i2c_port_t) num, conf.mode, 0, 0, 0);
    }
    this->address = addr;
    this->topAddress = topAddress;
    this->baseAddress = baseAddress;
    this->pageSize = pageSize;
    this->bufferSize = 0;
    this->readBuffer = nullptr;
    this->portNum = (i2c_port_t) num;
}

EEPROM::~EEPROM() {                                     // a destructor meant to clean up
    if (readBuffer != nullptr)
        free(readBuffer);
}


void EEPROM::write(uint16_t addr, uint8_t byte) {                           // writes a single byte
#ifdef M_PERFORMANCE_METRICS
    uint64_t start = esp_timer_get_time();
#endif
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();                   // i2c commands as described by AT24CM02 datasheet,
    i2c_master_start(cmd);                                          // page 17
    i2c_master_write_byte(cmd, (this->address << 1) | I2C_MASTER_WRITE, I2C_MASTER_ACK);
    i2c_master_write_byte(cmd, addr >> 8, I2C_MASTER_ACK);
    i2c_master_write_byte(cmd, addr & 0xFF, I2C_MASTER_ACK);
    i2c_master_write_byte(cmd, byte, I2C_MASTER_ACK);
    i2c_master_stop(cmd);
    parseError(i2c_master_cmd_begin(this->portNum, cmd, 1000 / portTICK_PERIOD_MS));
    i2c_cmd_link_delete(cmd);
    vTaskDelay(10 / portTICK_PERIOD_MS);
#ifdef M_PERFORMANCE_METRICS
    uint64_t end = esp_timer_get_time();
    char output[100] = "";
    sprintf(output, "Wrote 1 byte in %f seconds, giving a speed of %f Bps",
            (float) (end - start) / 1e6,
            1 / ((float) (end - start) / 1e6)
    );
    printf("%s\n", output);
    fflush(stdout);
#endif
}


void EEPROM::write(uint16_t baseAddr, uint8_t *byteArray, uint16_t size) {  // writes a string, page by page
#ifdef M_PERFORMANCE_METRICS                                                // stream as described by datasheet, page 17
    uint64_t start = esp_timer_get_time();
#endif

    for (int bytesWritten = 0; bytesWritten < size; bytesWritten += (this->pageSize)) {
        uint16_t pageAddress = baseAddr + bytesWritten;

        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (this->address << 1) | I2C_MASTER_WRITE, I2C_MASTER_ACK);
        i2c_master_write_byte(cmd, pageAddress >> 8, I2C_MASTER_ACK);
        i2c_master_write_byte(cmd, pageAddress & 0xFF, I2C_MASTER_ACK);
        for (int byte = 0; (byte < this->pageSize) && (bytesWritten + byte) < size; byte++) {
            fflush(stdout);
            i2c_master_write_byte(cmd, byteArray[bytesWritten + byte], I2C_MASTER_ACK);
        }
        i2c_master_stop(cmd);
        parseError(i2c_master_cmd_begin(this->portNum, cmd, 1000 / portTICK_PERIOD_MS));
        i2c_cmd_link_delete(cmd);
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
#ifdef M_PERFORMANCE_METRICS
    uint64_t end = esp_timer_get_time();
    char output[100] = "";
    sprintf(output, "Wrote %d bytes in %f seconds, giving a speed of %f Bps", (this->topAddress - this->baseAddress),
            (float) (end - start) / 1e6,
            (this->topAddress - this->baseAddress) / ((float) (end - start) / 1e6)
    );
    printf("%s\n", output);
    fflush(stdout);
#endif

}

uint8_t EEPROM::read(uint16_t addr) {                                           // reads a single byte
#ifdef M_PERFORMANCE_METRICS
    uint64_t start = esp_timer_get_time();
#endif
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();                               // datasheet page 21
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (this->address << 1) | I2C_MASTER_WRITE, I2C_MASTER_ACK);
    i2c_master_write_byte(cmd, addr << 8, I2C_MASTER_ACK);
    i2c_master_write_byte(cmd, addr & 0xFF, I2C_MASTER_ACK);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (this->address << 1) | I2C_MASTER_READ, I2C_MASTER_ACK);

    uint8_t data;
    i2c_master_read_byte(cmd, &data, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    parseError(i2c_master_cmd_begin(this->portNum, cmd, 1000 / portTICK_PERIOD_MS));
    i2c_cmd_link_delete(cmd);
#ifdef M_PERFORMANCE_METRICS
    uint64_t end = esp_timer_get_time();
    char output[100] = "";
    sprintf(output, "Read 1 byte in %f seconds, giving a speed of %f Bps",
            (float) (end - start) / 1e6,
            1 / ((float) (end - start) / 1e6)
    );
    printf("%s\n", output);
    fflush(stdout);
#endif
    return data;
}

uint8_t *EEPROM::read(uint16_t addr, uint16_t size) {                           // reads a chunk of the eeprom, from
#ifdef M_PERFORMANCE_METRICS                                                    // base address to the specified size
    uint64_t start = esp_timer_get_time();
#endif
    if (!this->bufferSize) {                                                    // an internal buffer is used to store data
        ESP_LOGD(EEPROM_TAG, "Creating buffer");
        this->readBuffer = (uint8_t *) calloc((size + 10), sizeof(uint8_t));
        this->bufferSize = size + 10;
    } else {
        if (size > bufferSize) {                                                // if it overflows, extend the buffer
            ESP_LOGD(EEPROM_TAG, "Increasing buffer size");
            this->readBuffer = (uint8_t *) realloc(this->readBuffer, (size + 10) * sizeof(uint8_t));
            this->bufferSize = size + 10;
        } else if (size + 100 < bufferSize) {                                   // if the buffer is 100B larger than the
            ESP_LOGD(EEPROM_TAG, "Decreasing buffer size");                            // size being read, decrease size
            this->readBuffer = (uint8_t *) realloc(this->readBuffer, (size + 10) * sizeof(uint8_t));
            this->bufferSize = size + 10;
        }
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();                               // datasheet page 22
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (this->address << 1) | I2C_MASTER_WRITE, I2C_MASTER_ACK);
    i2c_master_write_byte(cmd, addr << 8, I2C_MASTER_ACK);
    i2c_master_write_byte(cmd, addr & 0xFF, I2C_MASTER_ACK);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (this->address << 1) | I2C_MASTER_READ, I2C_MASTER_ACK);

    uint16_t readSize = 0;
    while (readSize < size - 1) {
        i2c_master_read_byte(cmd, &this->readBuffer[readSize], I2C_MASTER_ACK);
        readSize++;
    }
    i2c_master_read_byte(cmd, &this->readBuffer[readSize], I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    parseError(i2c_master_cmd_begin(this->portNum, cmd, 1000 / portTICK_PERIOD_MS));
    i2c_cmd_link_delete(cmd);
#ifdef M_PERFORMANCE_METRICS
    uint64_t end = esp_timer_get_time();
    char output[100] = "";
    sprintf(output, "Read %d bytes in %f seconds, giving a speed of %f Bps", size,
            (float) (end - start) / 1e6,
            size / ((float) (end - start) / 1e6)
    );
    printf("%s\n", output);
    fflush(stdout);
#endif

    return this->readBuffer;
}

uint8_t *EEPROM::dump() {                                                   // dumps whole contents of the eeprom, from
    this->read(this->baseAddress, (this->topAddress - this->baseAddress));  // base address to top address
    return this->readBuffer;                            // ! ! ! THIS SHOULD NOT BE USED IN PRODUCTION ! ! !
}                                                       // any sensible eeprom greater than 125KB will most likely
                                                        // overflow the heap. Possibly. Maybe. Fixme
void EEPROM::format(uint8_t filler) {

    uint16_t size = this->topAddress - this->baseAddress;       // formats the eeprom, filling it with the value
    auto *buf = (uint8_t *) malloc(size * sizeof(uint8_t));     // specified by filler. ! ! ! NOT PRODUCTION READY ! ! !
    for (uint_fast16_t i = 0; i < size; i++)                    // this should really write things page by page, not
        buf[i] = filler;                                        // create a huge ass buffer. Fixme

    this->write(0x0, buf, size);
    free(buf);
}

void EEPROM::parseError(esp_err_t errCode) {            // a cheap error parser
    switch (errCode) {
        case ESP_OK:
            ESP_LOGD(EEPROM_TAG, "Operation successful");
            break;
        case ESP_ERR_INVALID_ARG:
            ESP_LOGE(EEPROM_TAG, "Invalid argument!");
            break;
        case ESP_ERR_INVALID_STATE:
            ESP_LOGE(EEPROM_TAG, "Invalid state!");
            break;
        case ESP_ERR_TIMEOUT:
            ESP_LOGE(EEPROM_TAG, "Timeout!");
            break;
        default:
            break;
    }
}