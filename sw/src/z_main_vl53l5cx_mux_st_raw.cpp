#include <Arduino.h>
#include <Wire.h>

extern "C"
{
#include "sensors/vl53l5cx_uld/vl53l5cx_api.h"
uint8_t vl53l5cx_debug_init_step(void);
}

namespace
{
constexpr uint8_t TCA9548_ADDR = 0x70;
constexpr uint8_t VL53L5CX_ADDR = 0x29;
constexpr uint8_t VL53L5CX_CHANNEL = 2;
constexpr uint8_t VL53L5CX_I2C_CHUNK = 64;
constexpr uint32_t VL53L5CX_READY_TIMEOUT_MS = 200;

VL53L5CX_Configuration g_config = {};
VL53L5CX_ResultsData g_results = {};
bool g_initialized = false;
uint8_t g_last_status = 0;
const char *g_last_stage = "idle";

int32_t raw_get_tick()
{
    return (int32_t)millis();
}

int32_t raw_write(uint16_t address8, uint16_t reg, uint8_t *data, uint16_t length)
{
    const uint8_t address7 = (uint8_t)(address8 >> 1);
    uint16_t offset = 0;

    while (offset < length)
    {
        const uint8_t chunk = (uint8_t)min((uint16_t)VL53L5CX_I2C_CHUNK, (uint16_t)(length - offset));
        const uint16_t current_reg = (uint16_t)(reg + offset);

        Wire.beginTransmission(address7);
        Wire.write((uint8_t)(current_reg >> 8));
        Wire.write((uint8_t)(current_reg & 0xFF));
        for (uint8_t i = 0; i < chunk; ++i)
        {
            Wire.write(data[offset + i]);
        }

        if (Wire.endTransmission(true) != 0)
        {
            return -1;
        }

        offset = (uint16_t)(offset + chunk);
    }

    return 0;
}

int32_t raw_read(uint16_t address8, uint16_t reg, uint8_t *data, uint16_t length)
{
    const uint8_t address7 = (uint8_t)(address8 >> 1);
    uint16_t offset = 0;

    while (offset < length)
    {
        const uint8_t chunk = (uint8_t)min((uint16_t)VL53L5CX_I2C_CHUNK, (uint16_t)(length - offset));
        const uint16_t current_reg = (uint16_t)(reg + offset);

        Wire.beginTransmission(address7);
        Wire.write((uint8_t)(current_reg >> 8));
        Wire.write((uint8_t)(current_reg & 0xFF));
        if (Wire.endTransmission(false) != 0)
        {
            return -1;
        }

        const uint8_t received = Wire.requestFrom((int)address7, (int)chunk, (int)true);
        if (received != chunk)
        {
            return -1;
        }

        for (uint8_t i = 0; i < chunk; ++i)
        {
            data[offset + i] = (uint8_t)Wire.read();
        }

        offset = (uint16_t)(offset + chunk);
    }

    return 0;
}

bool selectMuxChannel(uint8_t channel)
{
    Wire.beginTransmission(TCA9548_ADDR);
    Wire.write((uint8_t)(1U << channel));
    return Wire.endTransmission(true) == 0;
}

bool probeAddress(uint8_t address)
{
    Wire.beginTransmission(address);
    return Wire.endTransmission(true) == 0;
}

bool readDeviceId(uint16_t &device_id)
{
    uint8_t buffer[2] = {0};
    if (raw_read((uint16_t)(VL53L5CX_ADDR << 1), 0x010F, buffer, 2) != 0)
    {
        return false;
    }

    device_id = (uint16_t)(((uint16_t)buffer[0] << 8) | buffer[1]);
    return true;
}

const char *debugStepToStage(uint8_t step)
{
    switch (step)
    {
    case 2:
        return "init_boot_poll";
    case 3:
        return "init_fw_access";
    case 4:
        return "init_fw_download";
    case 5:
        return "init_fw_verify";
    case 6:
        return "init_mcu_boot";
    case 7:
        return "init_nvm";
    case 8:
        return "init_xtalk";
    case 9:
        return "init_default_cfg";
    default:
        return "init";
    }
}

bool initRanging()
{
    memset(&g_config, 0, sizeof(g_config));
    memset(&g_results, 0, sizeof(g_results));

    g_config.platform.address = (uint16_t)(VL53L5CX_ADDR << 1);
    g_config.platform.Write = raw_write;
    g_config.platform.Read = raw_read;
    g_config.platform.GetTick = raw_get_tick;

    g_last_stage = "init";
    g_last_status = vl53l5cx_init(&g_config);
    if (g_last_status != VL53L5CX_STATUS_OK)
    {
        g_last_stage = debugStepToStage(vl53l5cx_debug_init_step());
        return false;
    }

    g_last_stage = "resolution";
    g_last_status = vl53l5cx_set_resolution(&g_config, VL53L5CX_RESOLUTION_8X8);
    if (g_last_status != VL53L5CX_STATUS_OK)
    {
        return false;
    }

    g_last_stage = "start";
    g_last_status = vl53l5cx_start_ranging(&g_config);
    if (g_last_status != VL53L5CX_STATUS_OK)
    {
        return false;
    }

    g_last_stage = "running";
    g_last_status = VL53L5CX_STATUS_OK;
    g_initialized = true;
    return true;
}

bool readFrame()
{
    if (!g_initialized)
    {
        return false;
    }

    uint8_t ready = 0;
    const uint32_t start_ms = millis();
    do
    {
        g_last_stage = "data_ready";
        g_last_status = vl53l5cx_check_data_ready(&g_config, &ready);
        if (g_last_status != VL53L5CX_STATUS_OK)
        {
            return false;
        }
        if (ready != 0)
        {
            break;
        }
        delay(5);
    } while ((millis() - start_ms) < VL53L5CX_READY_TIMEOUT_MS);

    if (ready == 0)
    {
        g_last_stage = "data_timeout";
        g_last_status = VL53L5CX_STATUS_TIMEOUT_ERROR;
        return false;
    }

    g_last_stage = "read_frame";
    g_last_status = vl53l5cx_get_ranging_data(&g_config, &g_results);
    return g_last_status == VL53L5CX_STATUS_OK;
}

void printFrameSummary()
{
    int16_t min_distance = INT16_MAX;
    int16_t max_distance = INT16_MIN;
    int16_t center_distance = g_results.distance_mm[27];

    for (uint8_t i = 0; i < 64; ++i)
    {
        if (g_results.nb_target_detected[i] > 0U)
        {
            min_distance = min(min_distance, g_results.distance_mm[i]);
            max_distance = max(max_distance, g_results.distance_mm[i]);
        }
    }

    if (min_distance == INT16_MAX)
    {
        min_distance = -1;
        max_distance = -1;
    }

    Serial.print(F("Frame stream="));
    Serial.print(g_config.streamcount);
    Serial.print(F(" temp="));
    Serial.print(g_results.silicon_temp_degc);
    Serial.print(F("C min="));
    Serial.print(min_distance);
    Serial.print(F("mm max="));
    Serial.print(max_distance);
    Serial.print(F("mm center="));
    Serial.print(center_distance);
    Serial.println(F("mm"));
}
} // namespace

void setup()
{
    Serial.begin(57600);
    delay(500);
    Serial.println();
    Serial.println(F("VL53L5CX raw-Wire ST mux test"));
    Serial.println(F("No drivetrain modules are included in this env."));
    Serial.println(F("This env avoids task_safe_wire and uses raw Wire only."));

    Wire.begin();

    const bool mux_ok = selectMuxChannel(VL53L5CX_CHANNEL);
    Serial.print(F("Mux channel "));
    Serial.print(VL53L5CX_CHANNEL);
    Serial.print(F(": "));
    Serial.println(mux_ok ? F("OK") : F("FAIL"));

    const bool probe_ok = mux_ok && probeAddress(VL53L5CX_ADDR);
    Serial.print(F("Channel "));
    Serial.print(VL53L5CX_CHANNEL);
    Serial.print(F(" probe at 0x"));
    Serial.print(VL53L5CX_ADDR, HEX);
    Serial.print(F(": "));
    Serial.println(probe_ok ? F("ACK") : F("NO ACK"));

    uint16_t device_id = 0;
    const bool id_ok = probe_ok && readDeviceId(device_id);
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

    const bool init_ok = probe_ok && initRanging();
    Serial.print(F("Init/start ranging: "));
    Serial.println(init_ok ? F("OK") : F("FAIL"));
    if (!init_ok)
    {
        Serial.print(F("Init fail stage="));
        Serial.print(g_last_stage);
        Serial.print(F(" status=0x"));
        Serial.println(g_last_status, HEX);
    }
}

void loop()
{
    if (readFrame())
    {
        printFrameSummary();
    }
    else
    {
        Serial.print(F("VL53L5CX frame read failed at "));
        Serial.print(g_last_stage);
        Serial.print(F(" status=0x"));
        Serial.println(g_last_status, HEX);
    }

    delay(1000);
}
