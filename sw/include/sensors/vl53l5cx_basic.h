#ifndef VL53L5CX_BASIC_H
#define VL53L5CX_BASIC_H

#include <Arduino.h>

extern "C"
{
#include "sensors/vl53l5cx_uld/vl53l5cx_api.h"
}

struct Vl53l5cxFrame
{
    bool valid;
    uint8_t streamcount;
    int8_t silicon_temp_degc;
    uint8_t nb_target_detected[64];
    uint8_t target_status[64];
    int16_t distance_mm[64];
    int16_t min_distance_mm;
    int16_t max_distance_mm;
    int16_t center_distance_mm;
};

class Vl53l5cxBasic
{
public:
    explicit Vl53l5cxBasic(uint8_t address = 0x29);

    bool probe() const;
    bool readDeviceId(uint16_t &device_id) const;
    bool initRanging8x8();
    bool readFrame(Vl53l5cxFrame &frame);
    bool isInitialized() const;
    uint8_t lastStatus() const;
    const char *lastStage() const;
    bool readRegister(uint16_t reg, uint8_t &value) const;
    bool readRegisters(uint16_t start_reg, uint8_t *buffer, uint16_t length) const;
    bool writeRegister(uint16_t reg, uint8_t value) const;
    bool writeRegisters(uint16_t start_reg, const uint8_t *buffer, uint16_t length) const;

private:
    uint8_t address;
    bool initialized;
    uint8_t last_status;
    const char *last_stage;
    VL53L5CX_Configuration config;
    VL53L5CX_ResultsData results;
};

#endif
