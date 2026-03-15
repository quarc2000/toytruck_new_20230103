#include <Arduino.h>

#include "expander.h"
#include "task_safe_wire.h"

namespace
{
constexpr uint8_t TCA9548_ADDR = 0x70;
constexpr uint8_t MCP23017_ADDR = 0x20;
constexpr uint8_t PCA9685_ADDR = 0x40;
constexpr uint8_t PCA9685_CHANNEL = 0;

constexpr uint8_t PCA9685_MODE1 = 0x00;
constexpr uint8_t PCA9685_MODE2 = 0x01;
constexpr uint8_t PCA9685_LED0_ON_L = 0x06;
constexpr uint8_t PCA9685_LED15_ON_L = PCA9685_LED0_ON_L + (15 * 4);
constexpr uint8_t PCA9685_PRESCALE = 0xFE;

constexpr uint8_t MODE1_RESTART = 0x80;
constexpr uint8_t MODE1_SLEEP = 0x10;
constexpr uint8_t MODE1_ALLCALL = 0x01;
constexpr uint8_t MODE1_AUTOINC = 0x20;

constexpr uint8_t SERVO_CHANNEL = 0;
constexpr uint16_t SERVO_PULSE_NEUTRAL_US = 1500;
constexpr uint16_t SERVO_PULSE_MIN_US = 1100;
constexpr uint16_t SERVO_PULSE_MAX_US = 1900;
constexpr uint16_t SERVO_FRAME_US = 20000;
constexpr uint8_t SERVO_PRESCALE_50HZ = 121;

EXPANDER io_expander(TCA9548_ADDR, MCP23017_ADDR);
bool g_servo_test_active = false;

struct Pca9685Snapshot
{
    uint8_t mode1;
    uint8_t mode2;
    uint8_t prescale;
    uint8_t led0[4];
    uint8_t led15[4];
};

bool probeAddress(uint8_t address)
{
    task_safe_wire_begin(address);
    const uint8_t result = task_safe_wire_end();
    return result == 0;
}

bool readRegister(uint8_t reg, uint8_t &value)
{
    task_safe_wire_begin(PCA9685_ADDR);
    task_safe_wire_write(reg);
    task_safe_wire_restart();
    const uint8_t count = task_safe_wire_request_from(PCA9685_ADDR, (uint8_t)1);
    if (count != 1)
    {
        task_safe_wire_end();
        return false;
    }

    value = (uint8_t)task_safe_wire_read();
    task_safe_wire_end();
    return true;
}

bool readRegisters(uint8_t start_reg, uint8_t *buffer, uint8_t length)
{
    task_safe_wire_begin(PCA9685_ADDR);
    task_safe_wire_write(start_reg);
    task_safe_wire_restart();
    const uint8_t count = task_safe_wire_request_from(PCA9685_ADDR, length);
    if (count != length)
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

bool writeRegister(uint8_t reg, uint8_t value)
{
    task_safe_wire_begin(PCA9685_ADDR);
    task_safe_wire_write(reg);
    task_safe_wire_write(value);
    return task_safe_wire_end() == 0;
}

bool writeRegisters(uint8_t start_reg, const uint8_t *values, uint8_t length)
{
    task_safe_wire_begin(PCA9685_ADDR);
    task_safe_wire_write(start_reg);
    for (uint8_t i = 0; i < length; ++i)
    {
        task_safe_wire_write(values[i]);
    }
    return task_safe_wire_end() == 0;
}

void selectPca9685Channel()
{
    io_expander.setChannel(PCA9685_CHANNEL);
}

bool captureSnapshot(Pca9685Snapshot &snapshot)
{
    return readRegister(PCA9685_MODE1, snapshot.mode1) &&
           readRegister(PCA9685_MODE2, snapshot.mode2) &&
           readRegister(PCA9685_PRESCALE, snapshot.prescale) &&
           readRegisters(PCA9685_LED0_ON_L, snapshot.led0, sizeof(snapshot.led0)) &&
           readRegisters(PCA9685_LED15_ON_L, snapshot.led15, sizeof(snapshot.led15));
}

bool restoreSnapshotToSafeSleep(const Pca9685Snapshot &snapshot)
{
    const uint8_t safe_mode1 = (uint8_t)((snapshot.mode1 | MODE1_SLEEP) & ~0x80);
    return writeRegisters(PCA9685_LED0_ON_L, snapshot.led0, sizeof(snapshot.led0)) &&
           writeRegisters(PCA9685_LED15_ON_L, snapshot.led15, sizeof(snapshot.led15)) &&
           writeRegister(PCA9685_PRESCALE, snapshot.prescale) &&
           writeRegister(PCA9685_MODE2, snapshot.mode2) &&
           writeRegister(PCA9685_MODE1, safe_mode1);
}

bool forceOutputsOffAndSleep()
{
    const uint8_t off_registers[4] = {
        0x00,
        0x00,
        0x00,
        0x10, // FULL_OFF bit in LEDn_OFF_H
    };

    return writeRegisters(PCA9685_LED0_ON_L + (SERVO_CHANNEL * 4), off_registers, sizeof(off_registers)) &&
           writeRegister(PCA9685_MODE1, (uint8_t)(MODE1_ALLCALL | MODE1_AUTOINC | MODE1_SLEEP));
}

bool configureServoPwm()
{
    const uint8_t sleep_mode1 = (uint8_t)(MODE1_ALLCALL | MODE1_AUTOINC | MODE1_SLEEP);
    const uint8_t wake_mode1 = (uint8_t)(MODE1_ALLCALL | MODE1_AUTOINC);
    const uint8_t restart_mode1 = (uint8_t)(wake_mode1 | MODE1_RESTART);

    Serial.print(F("Config write MODE1 sleep=0x"));
    Serial.println(sleep_mode1, HEX);
    if (!writeRegister(PCA9685_MODE1, sleep_mode1))
    {
        return false;
    }
    Serial.print(F("Config write PRE=0x"));
    Serial.println(SERVO_PRESCALE_50HZ, HEX);
    if (!writeRegister(PCA9685_PRESCALE, SERVO_PRESCALE_50HZ))
    {
        return false;
    }
    Serial.println(F("Config write MODE2=0x04"));
    if (!writeRegister(PCA9685_MODE2, 0x04))
    {
        return false;
    }
    Serial.print(F("Config write MODE1 wake=0x"));
    Serial.println(wake_mode1, HEX);
    if (!writeRegister(PCA9685_MODE1, wake_mode1))
    {
        return false;
    }

    delay(5);
    Serial.print(F("Config write MODE1 restart=0x"));
    Serial.println(restart_mode1, HEX);
    if (!writeRegister(PCA9685_MODE1, restart_mode1))
    {
        return false;
    }

    uint8_t mode1_verify = 0;
    uint8_t mode2_verify = 0;
    uint8_t prescale_verify = 0;
    if (!readRegister(PCA9685_MODE1, mode1_verify) ||
        !readRegister(PCA9685_MODE2, mode2_verify) ||
        !readRegister(PCA9685_PRESCALE, prescale_verify))
    {
        return false;
    }

    Serial.print(F("Servo config MODE1=0x"));
    Serial.print(mode1_verify, HEX);
    Serial.print(F(" MODE2=0x"));
    Serial.print(mode2_verify, HEX);
    Serial.print(F(" PRE=0x"));
    Serial.println(prescale_verify, HEX);

    return (mode1_verify & MODE1_SLEEP) == 0 &&
           ((mode1_verify & (uint8_t)~MODE1_RESTART) == wake_mode1) &&
           mode2_verify == 0x04 &&
           prescale_verify == SERVO_PRESCALE_50HZ;
}

bool setServoPulseUs(uint8_t channel, uint16_t pulse_us)
{
    const uint16_t pulse_counts = (uint16_t)(((uint32_t)pulse_us * 4096UL) / SERVO_FRAME_US);
    const uint8_t pwm_registers[4] = {
        0x00,
        0x00,
        (uint8_t)(pulse_counts & 0xFF),
        (uint8_t)((pulse_counts >> 8) & 0x0F),
    };

    return writeRegisters((uint8_t)(PCA9685_LED0_ON_L + (channel * 4)), pwm_registers, sizeof(pwm_registers));
}

void printBytes(const uint8_t *values, uint8_t length)
{
    for (uint8_t i = 0; i < length; ++i)
    {
        if (i != 0)
        {
            Serial.print(' ');
        }
        if (values[i] < 0x10)
        {
            Serial.print('0');
        }
        Serial.print(values[i], HEX);
    }
}

void printSnapshot(const __FlashStringHelper *label, const Pca9685Snapshot &snapshot)
{
    Serial.print(label);
    Serial.print(F(" MODE1=0x"));
    Serial.print(snapshot.mode1, HEX);
    Serial.print(F(" MODE2=0x"));
    Serial.print(snapshot.mode2, HEX);
    Serial.print(F(" PRE=0x"));
    Serial.print(snapshot.prescale, HEX);
    Serial.print(F(" LED0=["));
    printBytes(snapshot.led0, sizeof(snapshot.led0));
    Serial.print(F("] LED15=["));
    printBytes(snapshot.led15, sizeof(snapshot.led15));
    Serial.println(']');
}

bool verifyRegisterValue(const __FlashStringHelper *name, uint8_t reg, uint8_t expected)
{
    uint8_t actual = 0;
    if (!readRegister(reg, actual))
    {
        Serial.print(F("FAIL read "));
        Serial.println(name);
        return false;
    }

    Serial.print(name);
    Serial.print(F(" = 0x"));
    Serial.print(actual, HEX);
    if (actual != expected)
    {
        Serial.print(F(" expected 0x"));
        Serial.println(expected, HEX);
        return false;
    }

    Serial.println(F(" OK"));
    return true;
}

bool verifyMode1Value(const __FlashStringHelper *name, uint8_t expected_without_restart)
{
    uint8_t actual = 0;
    if (!readRegister(PCA9685_MODE1, actual))
    {
        Serial.print(F("FAIL read "));
        Serial.println(name);
        return false;
    }

    Serial.print(name);
    Serial.print(F(" = 0x"));
    Serial.print(actual, HEX);
    if ((actual & (uint8_t)~MODE1_RESTART) != expected_without_restart)
    {
        Serial.print(F(" expected 0x"));
        Serial.println(expected_without_restart, HEX);
        return false;
    }

    Serial.println(F(" OK"));
    return true;
}

bool verifyRegisterBlock(const __FlashStringHelper *name, uint8_t start_reg, const uint8_t *expected, uint8_t length)
{
    uint8_t actual[4] = {0};
    if (!readRegisters(start_reg, actual, length))
    {
        Serial.print(F("FAIL read block "));
        Serial.println(name);
        return false;
    }

    Serial.print(name);
    Serial.print(F(" = ["));
    printBytes(actual, length);
    Serial.print(F("] "));
    for (uint8_t i = 0; i < length; ++i)
    {
        if (actual[i] != expected[i])
        {
            Serial.println(F("MISMATCH"));
            return false;
        }
    }

    Serial.println(F("OK"));
    return true;
}

bool runRegisterSelfTest()
{
    Pca9685Snapshot original = {};
    if (!captureSnapshot(original))
    {
        Serial.println(F("FAIL could not capture original PCA9685 register snapshot"));
        return false;
    }

    printSnapshot(F("Original"), original);

    const uint8_t safe_sleep_mode1 = (uint8_t)((original.mode1 | MODE1_SLEEP) & ~0x80);
    const uint8_t safe_sleep_autoinc_mode1 = (uint8_t)(safe_sleep_mode1 | MODE1_AUTOINC);
    const uint8_t test_mode2 = 0x04;
    const uint8_t test_prescale = 121;
    const uint8_t led0_pattern[4] = {0x34, 0x01, 0x56, 0x02};
    const uint8_t led15_pattern[4] = {0x78, 0x00, 0xBC, 0x03};

    bool ok = true;
    ok = writeRegister(PCA9685_MODE1, safe_sleep_mode1) && ok;
    ok = verifyMode1Value(F("MODE1"), safe_sleep_mode1) && ok;

    ok = writeRegister(PCA9685_MODE2, test_mode2) && ok;
    ok = verifyRegisterValue(F("MODE2"), PCA9685_MODE2, test_mode2) && ok;

    ok = writeRegister(PCA9685_PRESCALE, test_prescale) && ok;
    ok = verifyRegisterValue(F("PRESCALE"), PCA9685_PRESCALE, test_prescale) && ok;

    // The PCA9685 needs MODE1 auto-increment enabled for multi-byte LED register
    // transactions; otherwise repeated block reads and writes can appear to hit the
    // same register address.
    ok = writeRegister(PCA9685_MODE1, safe_sleep_autoinc_mode1) && ok;
    ok = verifyMode1Value(F("MODE1 AI"), safe_sleep_autoinc_mode1) && ok;

    ok = writeRegisters(PCA9685_LED0_ON_L, led0_pattern, sizeof(led0_pattern)) && ok;
    ok = verifyRegisterBlock(F("LED0"), PCA9685_LED0_ON_L, led0_pattern, sizeof(led0_pattern)) && ok;

    ok = writeRegisters(PCA9685_LED15_ON_L, led15_pattern, sizeof(led15_pattern)) && ok;
    ok = verifyRegisterBlock(F("LED15"), PCA9685_LED15_ON_L, led15_pattern, sizeof(led15_pattern)) && ok;

    if (!restoreSnapshotToSafeSleep(original))
    {
        Serial.println(F("WARN failed to restore PCA9685 snapshot to safe sleep state"));
        ok = false;
    }

    Pca9685Snapshot restored = {};
    if (captureSnapshot(restored))
    {
        printSnapshot(F("Restored"), restored);
    }
    else
    {
        Serial.println(F("WARN could not read restored PCA9685 snapshot"));
        ok = false;
    }

    Serial.println(ok ? F("PCA9685 register self-test PASS") : F("PCA9685 register self-test FAIL"));
    return ok;
}
} // namespace

void setup()
{
    Serial.begin(57600);
    delay(500);
    Serial.println();
    Serial.println(F("PCA9685 isolated register test"));
    Serial.println(F("No drivetrain modules are included in this env."));

    io_expander.initSwitch();

    if (!probeAddress(TCA9548_ADDR))
    {
        Serial.println(F("FAIL TCA9548 not responding at 0x70"));
        return;
    }

    selectPca9685Channel();
    Serial.println(F("TCA9548 channel 0 selected for PCA9685 test."));

    if (!probeAddress(PCA9685_ADDR))
    {
        Serial.println(F("FAIL PCA9685 not responding at 0x40 on selected channel"));
        io_expander.setChannel(0);
        return;
    }

    Serial.println(F("PCA9685 acknowledged on I2C."));
    const bool register_ok = runRegisterSelfTest();
    if (!register_ok)
    {
        forceOutputsOffAndSleep();
        Serial.println(F("Register self-test failed, servo PWM will not start."));
        io_expander.setChannel(0);
        return;
    }

    if (!configureServoPwm())
    {
        forceOutputsOffAndSleep();
        Serial.println(F("FAIL could not configure PCA9685 for 50 Hz servo PWM"));
        io_expander.setChannel(0);
        return;
    }

    if (!setServoPulseUs(SERVO_CHANNEL, SERVO_PULSE_NEUTRAL_US))
    {
        forceOutputsOffAndSleep();
        Serial.println(F("FAIL could not write initial servo neutral pulse"));
        io_expander.setChannel(0);
        return;
    }

    Serial.println(F("PCA9685 configured for 50 Hz servo output on channel 0."));
    Serial.println(F("Servo sequence: neutral 1.5 ms, min 1.1 ms, max 1.9 ms."));
    g_servo_test_active = true;

    io_expander.setChannel(0);
}

void loop()
{
    if (!g_servo_test_active)
    {
        delay(250);
        return;
    }

    static uint32_t last_report_ms = 0;
    static uint8_t servo_phase = 0;
    const uint32_t now = millis();
    if (now - last_report_ms < 2000)
    {
        delay(20);
        return;
    }

    last_report_ms = now;

    selectPca9685Channel();
    uint8_t mode1 = 0;
    uint8_t mode2 = 0;
    uint8_t prescale = 0;
    uint16_t pulse_us = SERVO_PULSE_NEUTRAL_US;
    const __FlashStringHelper *label = F("neutral");

    switch (servo_phase)
    {
    case 0:
        pulse_us = SERVO_PULSE_NEUTRAL_US;
        label = F("neutral");
        break;
    case 1:
        pulse_us = SERVO_PULSE_MIN_US;
        label = F("min");
        break;
    default:
        pulse_us = SERVO_PULSE_MAX_US;
        label = F("max");
        break;
    }

    const bool write_ok = setServoPulseUs(SERVO_CHANNEL, pulse_us);

    const bool ok = readRegister(PCA9685_MODE1, mode1) &&
                    readRegister(PCA9685_MODE2, mode2) &&
                    readRegister(PCA9685_PRESCALE, prescale);

    if (ok && write_ok)
    {
        Serial.print(F("Servo "));
        Serial.print(label);
        Serial.print(F(" pulse="));
        Serial.print(pulse_us);
        Serial.print(F("us MODE1=0x"));
        Serial.print(mode1, HEX);
        Serial.print(F(" MODE2=0x"));
        Serial.print(mode2, HEX);
        Serial.print(F(" PRE=0x"));
        Serial.println(prescale, HEX);
    }
    else
    {
        Serial.println(F("Heartbeat read failed"));
    }

    io_expander.setChannel(0);
    servo_phase = (uint8_t)((servo_phase + 1) % 3);
}
