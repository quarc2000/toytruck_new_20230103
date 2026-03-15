#include "sensors/vl53l0x_basic.h"

#include "task_safe_wire.h"

namespace
{
constexpr uint8_t VL53L0X_SYSRANGE_START = 0x00;
constexpr uint8_t VL53L0X_RESULT_RANGE_STATUS = 0x14;
constexpr uint8_t VL53L0X_IDENTIFICATION_MODEL_ID = 0xC0;
constexpr uint8_t VL53L0X_EXPECTED_MODEL_ID = 0xEE;
constexpr uint8_t VL53L0X_VHV_CONFIG_PAD_SCL_SDA__EXTSUP_HV = 0x89;
constexpr uint8_t VL53L0X_MSRC_CONFIG_CONTROL = 0x60;
constexpr uint8_t VL53L0X_SYSTEM_SEQUENCE_CONFIG = 0x01;
constexpr uint8_t VL53L0X_SYSTEM_INTERRUPT_CONFIG_GPIO = 0x0A;
constexpr uint8_t VL53L0X_GPIO_HV_MUX_ACTIVE_HIGH = 0x84;
constexpr uint8_t VL53L0X_SYSTEM_INTERRUPT_CLEAR = 0x0B;
} // namespace

Vl53l0xBasic::Vl53l0xBasic(uint8_t address) : address(address) {}

bool Vl53l0xBasic::probe() const
{
    task_safe_wire_begin(address);
    return task_safe_wire_end() == 0;
}

bool Vl53l0xBasic::writeRegister(uint8_t reg, uint8_t value) const
{
    task_safe_wire_begin(address);
    task_safe_wire_write(reg);
    task_safe_wire_write(value);
    return task_safe_wire_end() == 0;
}

bool Vl53l0xBasic::readRegisters(uint8_t start_reg, uint8_t *buffer, uint8_t length) const
{
    task_safe_wire_begin(address);
    task_safe_wire_write(start_reg);
    task_safe_wire_restart();
    if (task_safe_wire_request_from(address, length) != length)
    {
        task_safe_wire_end();
        return false;
    }

    for (uint8_t i = 0; i < length; ++i)
    {
        buffer[i] = (uint8_t)task_safe_wire_read();
    }

    task_safe_wire_end();
    return true;
}

bool Vl53l0xBasic::init()
{
    uint8_t model_id = 0;
    if (!readRegisters(VL53L0X_IDENTIFICATION_MODEL_ID, &model_id, 1))
    {
        return false;
    }

    if (model_id != VL53L0X_EXPECTED_MODEL_ID)
    {
        return false;
    }

    // This is a minimal bring-up path for the current mux test. It avoids a
    // full library dependency but sets the common control bits needed to start
    // single-shot ranging.
    return writeRegister(VL53L0X_VHV_CONFIG_PAD_SCL_SDA__EXTSUP_HV, 0x01) &&
           writeRegister(VL53L0X_MSRC_CONFIG_CONTROL, 0x12) &&
           writeRegister(VL53L0X_SYSTEM_SEQUENCE_CONFIG, 0xFF) &&
           writeRegister(VL53L0X_SYSTEM_INTERRUPT_CONFIG_GPIO, 0x04) &&
           writeRegister(VL53L0X_GPIO_HV_MUX_ACTIVE_HIGH, 0x01) &&
           writeRegister(VL53L0X_SYSTEM_INTERRUPT_CLEAR, 0x01);
}

bool Vl53l0xBasic::readDistanceMm(uint16_t &distance_mm) const
{
    if (!writeRegister(VL53L0X_SYSRANGE_START, 0x01))
    {
        return false;
    }

    uint8_t status = 0;
    const uint32_t start_ms = millis();
    do
    {
        if (!readRegisters(VL53L0X_RESULT_RANGE_STATUS, &status, 1))
        {
            return false;
        }

        if ((status & 0x01) != 0)
        {
            break;
        }

        delay(5);
    } while (millis() - start_ms < 100);

    if ((status & 0x01) == 0)
    {
        return false;
    }

    uint8_t result_bytes[2] = {0};
    if (!readRegisters((uint8_t)(VL53L0X_RESULT_RANGE_STATUS + 10), result_bytes, 2))
    {
        return false;
    }

    distance_mm = (uint16_t)(((uint16_t)result_bytes[0] << 8) | result_bytes[1]);
    return writeRegister(VL53L0X_SYSTEM_INTERRUPT_CLEAR, 0x01);
}
