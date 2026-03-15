#ifndef VL53L0X_BASIC_H
#define VL53L0X_BASIC_H

#include <Arduino.h>

class Vl53l0xBasic
{
public:
    explicit Vl53l0xBasic(uint8_t address = 0x29);

    bool probe() const;
    bool init();
    bool readDistanceMm(uint16_t &distance_mm) const;

private:
    uint8_t address;

    bool writeRegister(uint8_t reg, uint8_t value) const;
    bool readRegisters(uint8_t start_reg, uint8_t *buffer, uint8_t length) const;
};

#endif
