#include <sensors/GY271.h>

#include <math.h>
#include <task_safe_wire.h>

namespace {
constexpr uint8_t QMC5883L_REG_XOUT_L = 0x00;
constexpr uint8_t QMC5883L_REG_STATUS = 0x06;
constexpr uint8_t QMC5883L_REG_CONTROL1 = 0x09;
constexpr uint8_t QMC5883L_REG_CONTROL2 = 0x0A;
constexpr uint8_t QMC5883L_REG_SET_RESET = 0x0B;
constexpr uint8_t QMC5883L_REG_CHIP_ID = 0x0D;

constexpr uint8_t QMC5883L_STATUS_DRDY = 0x01;
constexpr uint8_t QMC5883L_STATUS_OVL = 0x02;
constexpr uint8_t QMC5883L_STATUS_DOR = 0x04;

constexpr uint8_t QMC5883L_CONTROL1_CONT_10HZ_8G_OSR512 = 0x11;
constexpr uint8_t QMC5883L_CONTROL2_SOFT_RESET = 0x80;
constexpr uint8_t QMC5883L_SET_RESET_RECOMMENDED = 0x01;
constexpr uint8_t QMC5883L_CHIP_ID_EXPECTED = 0xFF;
} // namespace

GY271::GY271(uint8_t address) : _address(address), _x(0), _y(0), _z(0) {}

bool GY271::writeRegister(uint8_t reg, uint8_t value)
{
    task_safe_wire_begin(_address);
    task_safe_wire_write(reg);
    task_safe_wire_write(value);
    const uint8_t result = task_safe_wire_end();
    return result == 0;
}

bool GY271::readRegister(uint8_t reg, uint8_t &value)
{
    task_safe_wire_begin(_address);
    task_safe_wire_write(reg);
    task_safe_wire_restart();
    const uint8_t count = task_safe_wire_request_from(_address, static_cast<uint8_t>(1));
    if (count != 1) {
        task_safe_wire_end();
        return false;
    }

    value = static_cast<uint8_t>(task_safe_wire_read());
    task_safe_wire_end();
    return true;
}

bool GY271::begin()
{
    uint8_t chip_id = 0;
    if (!readChipId(chip_id)) {
        return false;
    }

    // Datasheet setup example:
    // 1. Optional soft reset to restore defaults.
    // 2. Write 0x0B = 0x01 to define set/reset period.
    // 3. Write 0x09 for continuous mode. For human-readable debug we use
    //    10 Hz instead of the datasheet's 200 Hz example so the service does
    //    not sit in permanent data-overrun when polled at a modest rate.
    if (!writeRegister(QMC5883L_REG_CONTROL2, QMC5883L_CONTROL2_SOFT_RESET)) {
        return false;
    }
    delay(10);

    if (!writeRegister(QMC5883L_REG_SET_RESET, QMC5883L_SET_RESET_RECOMMENDED)) {
        return false;
    }
    delay(10);

    if (!writeRegister(QMC5883L_REG_CONTROL1, QMC5883L_CONTROL1_CONT_10HZ_8G_OSR512)) {
        return false;
    }
    delay(10);

    uint8_t control1 = 0;
    uint8_t set_reset = 0;
    if (!readControl1(control1) || !readSetResetPeriod(set_reset)) {
        return false;
    }

    return chip_id == QMC5883L_CHIP_ID_EXPECTED &&
           control1 == QMC5883L_CONTROL1_CONT_10HZ_8G_OSR512 &&
           set_reset == QMC5883L_SET_RESET_RECOMMENDED;
}

void GY271::readData(int16_t &x, int16_t &y, int16_t &z)
{
    updateData();
    x = _x;
    y = _y;
    z = _z;
}

bool GY271::readDataBlock(int16_t &x, int16_t &y, int16_t &z)
{
    task_safe_wire_begin(_address);
    task_safe_wire_write(QMC5883L_REG_XOUT_L);
    task_safe_wire_restart();

    const uint8_t count = task_safe_wire_request_from(_address, static_cast<uint8_t>(6));
    if (count != 6) {
        task_safe_wire_end();
        return false;
    }

    const uint8_t xl = static_cast<uint8_t>(task_safe_wire_read());
    const uint8_t xh = static_cast<uint8_t>(task_safe_wire_read());
    const uint8_t yl = static_cast<uint8_t>(task_safe_wire_read());
    const uint8_t yh = static_cast<uint8_t>(task_safe_wire_read());
    const uint8_t zl = static_cast<uint8_t>(task_safe_wire_read());
    const uint8_t zh = static_cast<uint8_t>(task_safe_wire_read());
    task_safe_wire_end();

    x = static_cast<int16_t>(static_cast<uint16_t>(xl) | (static_cast<uint16_t>(xh) << 8));
    y = static_cast<int16_t>(static_cast<uint16_t>(yl) | (static_cast<uint16_t>(yh) << 8));
    z = static_cast<int16_t>(static_cast<uint16_t>(zl) | (static_cast<uint16_t>(zh) << 8));
    return true;
}

int16_t GY271::getX()
{
    updateData();
    return _x;
}

int16_t GY271::getY()
{
    updateData();
    return _y;
}

int16_t GY271::getZ()
{
    updateData();
    return _z;
}

bool GY271::readStatus(uint8_t &status)
{
    return readRegister(QMC5883L_REG_STATUS, status);
}

bool GY271::readControl1(uint8_t &value)
{
    return readRegister(QMC5883L_REG_CONTROL1, value);
}

bool GY271::readControl2(uint8_t &value)
{
    return readRegister(QMC5883L_REG_CONTROL2, value);
}

bool GY271::readSetResetPeriod(uint8_t &value)
{
    return readRegister(QMC5883L_REG_SET_RESET, value);
}

bool GY271::readChipId(uint8_t &value)
{
    return readRegister(QMC5883L_REG_CHIP_ID, value);
}

bool GY271::isDataReady()
{
    uint8_t status = 0;
    return readStatus(status) && ((status & QMC5883L_STATUS_DRDY) != 0);
}

void GY271::updateData()
{
    int16_t x = 0;
    int16_t y = 0;
    int16_t z = 0;
    if (!readDataBlock(x, y, z)) {
        _x = 0;
        _y = 0;
        _z = 0;
        return;
    }

    _x = x;
    _y = y;
    _z = z;
}

int GY271::getCompassDirection()
{
    updateData();
    float degrees = atan2f(static_cast<float>(_y), static_cast<float>(_x)) * 180.0f / static_cast<float>(M_PI);
    if (degrees < 0.0f) {
        degrees += 360.0f;
    }
    return static_cast<int>(degrees * 10.0f);
}
