#ifndef VL53L5CX_BASIC_H
#define VL53L5CX_BASIC_H

#include <Arduino.h>

class Vl53l5cxBasic
{
public:
    explicit Vl53l5cxBasic(uint8_t address = 0x29);

    bool probe() const;
    bool readDeviceId(uint16_t &device_id) const;
    bool readRegister(uint16_t reg, uint8_t &value) const;
    bool readRegisters(uint16_t start_reg, uint8_t *buffer, uint16_t length) const;
    bool writeRegister(uint16_t reg, uint8_t value) const;
    bool writeRegisters(uint16_t start_reg, const uint8_t *buffer, uint16_t length) const;

private:
    uint8_t address;
};

#endif
