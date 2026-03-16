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

void printFrame(const Vl53l5cxFrame &frame)
{
    Serial.print(F("Frame stream="));
    Serial.print(frame.streamcount);
    Serial.print(F(" temp="));
    Serial.print(frame.silicon_temp_degc);
    Serial.print(F("C min="));
    Serial.print(frame.min_distance_mm);
    Serial.print(F("mm max="));
    Serial.print(frame.max_distance_mm);
    Serial.print(F("mm center="));
    Serial.print(frame.center_distance_mm);
    Serial.println(F("mm"));

    for (uint8_t row = 0; row < 8; ++row)
    {
        for (uint8_t col = 0; col < 8; ++col)
        {
            const uint8_t index = (uint8_t)(row * 8 + col);
            Serial.print(frame.distance_mm[index]);
            Serial.print(F("/"));
            Serial.print(frame.target_status[index]);
            if (col < 7)
            {
                Serial.print(F("\t"));
            }
        }
        Serial.println();
    }
}
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

    const bool init_ok = probe_ok && sensor.initRanging8x8();
    Serial.print(F("Init/start ranging: "));
    Serial.println(init_ok ? F("OK") : F("FAIL"));
    if (!init_ok)
    {
        Serial.print(F("Init fail stage="));
        Serial.print(sensor.lastStage());
        Serial.print(F(" status=0x"));
        Serial.println(sensor.lastStatus(), HEX);
    }

    io_expander.setChannel(0);
}

void loop()
{
    io_expander.setChannel(VL53L5CX_CHANNEL);
    Vl53l5cxFrame frame = {};
    if (sensor.isInitialized() && sensor.readFrame(frame))
    {
        printFrame(frame);
    }
    else
    {
        Serial.print(F("VL53L5CX frame read failed at "));
        Serial.print(sensor.lastStage());
        Serial.print(F(" status=0x"));
        Serial.println(sensor.lastStatus(), HEX);
    }

    io_expander.setChannel(0);
    delay(1000);
}
