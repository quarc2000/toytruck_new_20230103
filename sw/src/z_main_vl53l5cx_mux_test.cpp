#include <Arduino.h>

#include "expander.h"
#include "sensors/vl53l5cx_basic.h"

namespace
{
constexpr uint8_t TCA9548_ADDR = 0x70;
constexpr uint8_t MCP23017_ADDR = 0x20;
constexpr uint8_t VL53L5CX_ADDR = 0x29;
constexpr uint8_t VL53L5CX_CHANNEL = 2;

EXPANDER io_expander(TCA9548_ADDR, MCP23017_ADDR);
Vl53l5cxBasic sensor(VL53L5CX_ADDR);
} // namespace

void setup()
{
    Serial.begin(57600);
    delay(500);
    Serial.println();
    Serial.println(F("VL53L5CX mux test"));
    Serial.println(F("No drivetrain modules are included in this env."));

    io_expander.initSwitch();
    io_expander.setChannel(VL53L5CX_CHANNEL);

    const bool probe_ok = sensor.probe();
    Serial.print(F("Channel "));
    Serial.print(VL53L5CX_CHANNEL);
    Serial.print(F(" probe at 0x"));
    Serial.print(VL53L5CX_ADDR, HEX);
    Serial.print(F(": "));
    Serial.println(probe_ok ? F("ACK") : F("NO ACK"));

    uint16_t device_id = 0;
    const bool id_ok = probe_ok && sensor.readDeviceId(device_id);
    Serial.print(F("Device ID reg 0x010F: "));
    if (id_ok)
    {
        Serial.print(F("0x"));
        Serial.println(device_id, HEX);
    }
    else
    {
        Serial.println(F("READ FAIL"));
    }

    io_expander.setChannel(0);
}

void loop()
{
    io_expander.setChannel(VL53L5CX_CHANNEL);

    const bool present = sensor.probe();
    Serial.print(F("CH"));
    Serial.print(VL53L5CX_CHANNEL);
    Serial.print(F(" VL53L5CX "));
    Serial.print(present ? F("ACK") : F("NO ACK"));

    uint16_t device_id = 0;
    if (present && sensor.readDeviceId(device_id))
    {
        Serial.print(F(" ID=0x"));
        Serial.println(device_id, HEX);
    }
    else
    {
        Serial.println(F(" ID READ FAIL"));
    }

    io_expander.setChannel(0);
    delay(1000);
}
