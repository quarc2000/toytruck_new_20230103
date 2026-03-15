#include <Arduino.h>

#include "expander.h"
#include "sensors/vl53l0x_basic.h"

namespace
{
constexpr uint8_t TCA9548_ADDR = 0x70;
constexpr uint8_t MCP23017_ADDR = 0x20;
constexpr uint8_t VL53L0X_ADDR = 0x29;
constexpr uint8_t CHANNEL_A = 1;
constexpr uint8_t CHANNEL_B = 2;

EXPANDER io_expander(TCA9548_ADDR, MCP23017_ADDR);
Vl53l0xBasic front_left_sensor(VL53L0X_ADDR);
Vl53l0xBasic front_right_sensor(VL53L0X_ADDR);

void selectChannel(uint8_t channel)
{
    io_expander.setChannel(channel);
}

bool initSensorOnChannel(uint8_t channel, Vl53l0xBasic &sensor)
{
    selectChannel(channel);
    const bool ok = sensor.probe() && sensor.init();
    io_expander.setChannel(0);
    return ok;
}

bool readSensorOnChannel(uint8_t channel, Vl53l0xBasic &sensor, uint16_t &distance_mm)
{
    selectChannel(channel);
    const bool ok = sensor.readDistanceMm(distance_mm);
    io_expander.setChannel(0);
    return ok;
}
} // namespace

void setup()
{
    Serial.begin(57600);
    delay(500);
    Serial.println();
    Serial.println(F("VL53L0X mux test"));
    Serial.println(F("No drivetrain modules are included in this env."));

    io_expander.initSwitch();

    const bool channel_1_ok = initSensorOnChannel(CHANNEL_A, front_left_sensor);
    Serial.print(F("Channel 1 init: "));
    Serial.println(channel_1_ok ? F("OK") : F("FAIL"));

    const bool channel_2_ok = initSensorOnChannel(CHANNEL_B, front_right_sensor);
    Serial.print(F("Channel 2 init: "));
    Serial.println(channel_2_ok ? F("OK") : F("FAIL"));
}

void loop()
{
    uint16_t channel_1_distance = 0;
    uint16_t channel_2_distance = 0;

    const bool channel_1_ok = readSensorOnChannel(CHANNEL_A, front_left_sensor, channel_1_distance);
    const bool channel_2_ok = readSensorOnChannel(CHANNEL_B, front_right_sensor, channel_2_distance);

    Serial.print(F("CH1 "));
    if (channel_1_ok)
    {
        Serial.print(channel_1_distance);
        Serial.print(F(" mm"));
    }
    else
    {
        Serial.print(F("FAIL"));
    }

    Serial.print(F(" | CH2 "));
    if (channel_2_ok)
    {
        Serial.print(channel_2_distance);
        Serial.println(F(" mm"));
    }
    else
    {
        Serial.println(F("FAIL"));
    }

    delay(500);
}
