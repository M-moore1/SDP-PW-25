#ifndef i2c_H
#define i2c_H

#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "pinout.h"

i2c_master_bus_handle_t i2c_init();
i2c_master_dev_handle_t device_init(i2c_master_bus_handle_t bus_handle, uint8_t dev_addr);


#endif