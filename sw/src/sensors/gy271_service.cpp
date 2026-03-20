// GY-271 (QMC5883L) Magnetometer Service Task
// Integrates with IO expander/switch (port 3) when present and publishes raw magnetic axes plus
// calculatedMagCourse and calculatedMagDisturbance.

#include <Arduino.h>
#include <math.h>
#include <sensors/GY271.h>
#include <variables/setget.h>
#include <expander.h>
#include <task_safe_wire.h>
#include <basic_telemetry/basic_logger.h>

namespace {
constexpr uint8_t TCA9548_ADDR = 0x70;
constexpr uint8_t MCP23017_ADDR = 0x20;
constexpr uint8_t GY271_I2C_ADDR = 0x0D;
constexpr uint8_t GY271_SWITCH_PORT = 3;
constexpr float MAG_DISTURBANCE_THRESHOLD_MAG = 0.3f;  // 30% deviation in magnitude
constexpr TickType_t GY271_TASK_PERIOD_MS = 100;
constexpr uint8_t QMC5883L_STATUS_DRDY = 0x01;
constexpr uint8_t QMC5883L_STATUS_OVL = 0x02;
constexpr uint8_t QMC5883L_STATUS_DOR = 0x04;
constexpr uint32_t GY271_PRINT_PERIOD_MS = 3000;
constexpr uint8_t GY271_BASELINE_SAMPLES = 8;
constexpr TickType_t GY271_BASELINE_TIMEOUT_MS = 1500;
constexpr TickType_t GY271_FIRST_SAMPLE_TIMEOUT_MS = 2500;
constexpr uint8_t GY271_BOOTSTRAP_SAMPLE_RETRIES = 12;
constexpr TickType_t GY271_BOOTSTRAP_RETRY_MS = 50;
// Current bias for the newly installed sensor:
// - subtract 467 from X
// - add 347 to Y
// Previous sensor bias kept here for reference if the old module is reinstalled:
// - old X bias: +400
// - old Y bias: -150
constexpr int16_t GY271_X_BIAS_LSB = -467;
constexpr int16_t GY271_Y_BIAS_LSB = 347;

EXPANDER gy271_expander(TCA9548_ADDR, MCP23017_ADDR);
bool gy271_task_started = false;

bool deviceResponds(uint8_t address)
{
    task_safe_wire_begin(address);
    const uint8_t result = task_safe_wire_end();
    return result == 0;
}

bool magnetometerPresent()
{
    const bool expanderPresent = deviceResponds(TCA9548_ADDR);
    globalVar_set(configExpanderPresent, expanderPresent ? 1 : 0);
    if (!expanderPresent) {
        return false;
    }

    gy271_expander.initSwitch();
    gy271_expander.pushChannel(GY271_SWITCH_PORT);
    const bool present = deviceResponds(GY271_I2C_ADDR);
    gy271_expander.popChannel();
    globalVar_set(configGy271Present, present ? 1 : 0);
    return present;
}
}

static int computeCourseDeg10FromAdjustedXY(int16_t adjusted_x, int16_t adjusted_y)
{
    const float fx = static_cast<float>(adjusted_x);
    const float fy = static_cast<float>(adjusted_y);
    float heading_y_rad = atan2f(-fx, fy);
    float heading_y_deg = heading_y_rad * 180.0f / 3.14159265f;
    if (heading_y_deg < 0) heading_y_deg += 360.0f;
    return static_cast<int>(lroundf(heading_y_deg * 10.0f));
}

static void printRegisterSnapshot(GY271 &mag)
{
    uint8_t chip_id = 0;
    uint8_t status = 0;
    uint8_t control1 = 0;
    uint8_t control2 = 0;
    uint8_t set_reset = 0;

    const bool id_ok = mag.readChipId(chip_id);
    const bool status_ok = mag.readStatus(status);
    const bool ctrl1_ok = mag.readControl1(control1);
    const bool ctrl2_ok = mag.readControl2(control2);
    const bool set_ok = mag.readSetResetPeriod(set_reset);

    Serial.print("GY-271 REGS: ID=");
    if (id_ok) {
        Serial.print("0x");
        Serial.print(chip_id, HEX);
    } else {
        Serial.print("READ_FAIL");
    }
    Serial.print(" STATUS=");
    if (status_ok) {
        Serial.print("0x");
        Serial.print(status, HEX);
    } else {
        Serial.print("READ_FAIL");
    }
    Serial.print(" CTRL1=");
    if (ctrl1_ok) {
        Serial.print("0x");
        Serial.print(control1, HEX);
    } else {
        Serial.print("READ_FAIL");
    }
    Serial.print(" CTRL2=");
    if (ctrl2_ok) {
        Serial.print("0x");
        Serial.print(control2, HEX);
    } else {
        Serial.print("READ_FAIL");
    }
    Serial.print(" SETRST=");
    if (set_ok) {
        Serial.print("0x");
        Serial.print(set_reset, HEX);
    } else {
        Serial.print("READ_FAIL");
    }
    Serial.println();
}

static void gy271_task(void *pvParameters) {
    GY271 mag(GY271_I2C_ADDR);
    gy271_expander.pushChannel(GY271_SWITCH_PORT);
    const bool begin_ok = mag.begin();
    printRegisterSnapshot(mag);
    gy271_expander.popChannel();
    Serial.print("GY-271 begin: ");
    Serial.println(begin_ok ? "OK" : "FAIL");
    if (!begin_ok) {
        globalVar_set(calculatedMagDisturbance, 1);
        globalVar_set(configGy271Present, 0);
        basic_log_warn("GY-271 detected but begin failed on mux port 3");
        vTaskDelete(nullptr);
        return;
    }
    globalVar_set(configGy271Present, 1);
    basic_log_info("GY-271 detected on mux port 3");

    // Reference Earth's field magnitude at Stockholm in nT. This is only used
    // for a rough disturbance flag until hard/soft iron calibration exists.
    const float expected_mag_nt = 52075.3f;
    const float expected_mag_lsb = expected_mag_nt / 100000.0f * 3000.0f; // 8 G range, 3000 LSB/G
    int32_t baseline_mag_sum = 0;
    uint8_t baseline_count = 0;
    const TickType_t baseline_started = xTaskGetTickCount();
    while (baseline_count < GY271_BASELINE_SAMPLES &&
           (xTaskGetTickCount() - baseline_started) < pdMS_TO_TICKS(GY271_BASELINE_TIMEOUT_MS)) {
        int16_t bx = 0;
        int16_t by = 0;
        int16_t bz = 0;
        gy271_expander.pushChannel(GY271_SWITCH_PORT);
        const bool data_ok = mag.readDataBlock(bx, by, bz);
        gy271_expander.popChannel();

        if (data_ok) {
            baseline_mag_sum += static_cast<int32_t>(lroundf(sqrtf((float)bx * (float)bx + (float)by * (float)by + (float)bz * (float)bz)));
            baseline_count++;
        }
        vTaskDelay(pdMS_TO_TICKS(GY271_TASK_PERIOD_MS));
    }

    const int16_t baseline_mag = baseline_count > 0
        ? static_cast<int16_t>(baseline_mag_sum / baseline_count)
        : static_cast<int16_t>(lroundf(expected_mag_lsb));
    Serial.print("GY-271 baseline MAG=");
    Serial.print(baseline_mag);
    Serial.println();

    int16_t last_x = 0;
    int16_t last_y = 0;
    int16_t last_z = 0;
    uint32_t last_print_ms = 0;
    int last_trusted_course_deg10 = 0;
    bool have_sample = false;
    const TickType_t sample_wait_started = xTaskGetTickCount();

    for (;;) {
        int16_t x = last_x;
        int16_t y = last_y;
        int16_t z = last_z;
        gy271_expander.pushChannel(GY271_SWITCH_PORT);
        uint8_t status = 0;
        const bool status_ok = mag.readStatus(status);
        const bool data_ok = mag.readDataBlock(x, y, z);
        gy271_expander.popChannel();

        if (data_ok) {
            last_x = x;
            last_y = y;
            last_z = z;
            have_sample = true;
            globalVar_set(rawMagX, x);
            globalVar_set(rawMagY, y);
            globalVar_set(rawMagZ, z);
        }

        if (!have_sample) {
            const TickType_t waited = xTaskGetTickCount() - sample_wait_started;
            if (waited >= pdMS_TO_TICKS(GY271_FIRST_SAMPLE_TIMEOUT_MS)) {
                globalVar_set(configGy271Present, 0);
                globalVar_set(configMagHeadingValid, 0);
                globalVar_set(calculatedMagDisturbance, 1);
                basic_log_warn("GY-271 detected but no first sample, disabling magnetic startup lock");
                vTaskDelete(nullptr);
                return;
            }
            globalVar_set(calculatedMagDisturbance, 1);
            vTaskDelay(pdMS_TO_TICKS(GY271_TASK_PERIOD_MS));
            continue;
        }

        const int16_t adjusted_x = static_cast<int16_t>(x + GY271_X_BIAS_LSB);
        const int16_t adjusted_y = static_cast<int16_t>(y + GY271_Y_BIAS_LSB);

        float fx = (float)adjusted_x;
        float fy = (float)adjusted_y;
        float fz = (float)z;
        float mag_val = sqrtf(fx*fx + fy*fy + fz*fz);
        // Current heading convention uses the native sensor axes directly:
        // +Y is treated as forward and the clockwise heading from north is atan2(-X, Y).
        const int course_y_deg10 = computeCourseDeg10FromAdjustedXY(adjusted_x, adjusted_y);
        last_trusted_course_deg10 = course_y_deg10;
        globalVar_set(calculatedMagCourse, last_trusted_course_deg10);
        globalVar_set(configMagHeadingValid, 1);

        const float mag_deviation = expected_mag_lsb > 1.0f ? fabsf(mag_val - expected_mag_lsb) / expected_mag_lsb : 0.0f;
        const bool overflow = status_ok && ((status & QMC5883L_STATUS_OVL) != 0);
        const bool disturbance = overflow || (mag_deviation > MAG_DISTURBANCE_THRESHOLD_MAG);
        globalVar_set(calculatedMagDisturbance, disturbance ? 1 : 0);

        if ((millis() - last_print_ms) >= GY271_PRINT_PERIOD_MS) {
            char course_buf[20];
            if (disturbance) {
                snprintf(course_buf,
                         sizeof(course_buf),
                         "[%6.1f]",
                         last_trusted_course_deg10 / 10.0f);
            } else {
                snprintf(course_buf,
                         sizeof(course_buf),
                         " %6.1f ",
                         last_trusted_course_deg10 / 10.0f);
            }

            char line[80];
            snprintf(line,
                     sizeof(line),
                     "X=%7d  Y=%7d  C=%s",
                     static_cast<int>(adjusted_x),
                     static_cast<int>(adjusted_y),
                     course_buf);
            Serial.println(line);
            last_print_ms = millis();
        }

        vTaskDelay(pdMS_TO_TICKS(GY271_TASK_PERIOD_MS));
    }
}

// Call this from your main setup to start the task
void start_gy271_service() {
    if (gy271_task_started) {
        return;
    }

    globalVar_set(rawMagX, 0);
    globalVar_set(rawMagY, 0);
    globalVar_set(rawMagZ, 0);
    globalVar_set(calculatedMagCourse, 0);
    globalVar_set(calculatedMagDisturbance, 1);
    globalVar_set(configGy271Present, 0);
    globalVar_set(configMagHeadingValid, 0);

    if (!magnetometerPresent()) {
        Serial.println("GY-271 path not present, magnetometer disabled");
        basic_log_info("GY-271 not detected on mux port 3");
        gy271_task_started = true;
        return;
    }

    // Bootstrap synchronously before spawning the task so startup gating uses a
    // real first sample, not only an address probe.
    GY271 bootstrapMag(GY271_I2C_ADDR);
    gy271_expander.pushChannel(GY271_SWITCH_PORT);
    const bool begin_ok = bootstrapMag.begin();
    gy271_expander.popChannel();
    if (!begin_ok) {
        globalVar_set(configGy271Present, 0);
        basic_log_warn("GY-271 detected but bootstrap begin failed on mux port 3");
        gy271_task_started = true;
        return;
    }

    bool first_sample_ok = false;
    int16_t sx = 0;
    int16_t sy = 0;
    int16_t sz = 0;
    for (uint8_t i = 0; i < GY271_BOOTSTRAP_SAMPLE_RETRIES; ++i) {
        gy271_expander.pushChannel(GY271_SWITCH_PORT);
        first_sample_ok = bootstrapMag.readDataBlock(sx, sy, sz);
        gy271_expander.popChannel();
        if (first_sample_ok) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(GY271_BOOTSTRAP_RETRY_MS));
    }
    if (!first_sample_ok) {
        globalVar_set(configGy271Present, 0);
        globalVar_set(configMagHeadingValid, 0);
        basic_log_warn("GY-271 detected but bootstrap first sample failed on mux port 3");
        gy271_task_started = true;
        return;
    }

    const int16_t adjusted_x = static_cast<int16_t>(sx + GY271_X_BIAS_LSB);
    const int16_t adjusted_y = static_cast<int16_t>(sy + GY271_Y_BIAS_LSB);
    globalVar_set(rawMagX, sx);
    globalVar_set(rawMagY, sy);
    globalVar_set(rawMagZ, sz);
    globalVar_set(calculatedMagCourse, computeCourseDeg10FromAdjustedXY(adjusted_x, adjusted_y));
    globalVar_set(calculatedMagDisturbance, 0);
    globalVar_set(configGy271Present, 1);
    globalVar_set(configMagHeadingValid, 1);
    basic_log_info("GY-271 bootstrap first sample accepted on mux port 3");

    xTaskCreatePinnedToCore(gy271_task, "gy271_task", 4096, NULL, 1, NULL, 1);
    gy271_task_started = true;
}
