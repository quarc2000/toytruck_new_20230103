#include "sensors/vl53l5cx_basic.h"

#include "task_safe_wire.h"

namespace
{
constexpr uint16_t VL53L5CX_DEVICE_ID_REG = 0x010F;
}

Vl53l5cxBasic::Vl53l5cxBasic(uint8_t address) : address(address) {}

bool Vl53l5cxBasic::probe() const
{
    task_safe_wire_begin(address);
    return task_safe_wire_end() == 0;
}

bool Vl53l5cxBasic::readDeviceId(uint16_t &device_id) const
{
    uint8_t buffer[2] = {0};
    if (!readRegisters(VL53L5CX_DEVICE_ID_REG, buffer, 2))
    {
        return false;
    }

    device_id = (uint16_t)(((uint16_t)buffer[0] << 8) | buffer[1]);
    return true;
}

bool Vl53l5cxBasic::readRegister(uint16_t reg, uint8_t &value) const
{
    return readRegisters(reg, &value, 1);
}

bool Vl53l5cxBasic::readRegisters(uint16_t start_reg, uint8_t *buffer, uint16_t length) const
{
    task_safe_wire_begin(address);
    task_safe_wire_write((uint8_t)(start_reg >> 8));
    task_safe_wire_write((uint8_t)(start_reg & 0xFF));
    task_safe_wire_restart();

    if (task_safe_wire_request_from(address, (uint8_t)length) != length)
    {
        task_safe_wire_end();
        return false;
    }

    for (uint16_t i = 0; i < length; ++i)
    {
        buffer[i] = (uint8_t)task_safe_wire_read();
    }

    task_safe_wire_end();
    return true;
}

bool Vl53l5cxBasic::writeRegister(uint16_t reg, uint8_t value) const
{
    return writeRegisters(reg, &value, 1);
}

bool Vl53l5cxBasic::writeRegisters(uint16_t start_reg, const uint8_t *buffer, uint16_t length) const
{
    task_safe_wire_begin(address);
    task_safe_wire_write((uint8_t)(start_reg >> 8));
    task_safe_wire_write((uint8_t)(start_reg & 0xFF));

    for (uint16_t i = 0; i < length; ++i)
    {
        task_safe_wire_write(buffer[i]);
    }

    return task_safe_wire_end() == 0;
}
