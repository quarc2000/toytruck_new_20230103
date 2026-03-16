#include <sensors/accsensor.h>
#include <Arduino.h>
#include <task_safe_wire.h>
#include <variables/setget.h>
#include <freertos/FreeRTOS.h>
#include <sensors/kalman_filter.h>

#define MPU6050_ADDR 0x68

// Power management registers for MPU6050
#define PWR_MGMT_1 0x6B
#define PWR_MGMT_2 0x6C

static constexpr int32_t MPU6050_TEMP_OFFSET_DEGC10 = 365;
static constexpr int32_t MPU6050_GYRO_LSB_PER_DPS = 131;
static constexpr int32_t MPU6050_GYRO_SCALE_FACTOR = 10;
static constexpr int32_t MPU6050_ACCEL_LSB_PER_G = 16384;
static constexpr int32_t GRAVITY_MMPS2 = 9807;
static constexpr int32_t MPU6050_AUX_EMA_ALPHA_NUMERATOR = 1;
static constexpr int32_t MPU6050_AUX_EMA_ALPHA_DENOMINATOR = 4;
static constexpr int32_t MPU6050_GYRO_Z_DEADBAND_DPS10 = 8;
static constexpr int32_t MPU6050_MAX_INTEGRATION_DT_MS = 40;
static constexpr int32_t MPU6050_ACCEL_DEADBAND_COUNTS = 30;
static constexpr int32_t MPU6050_STATIONARY_ACCEL_THRESHOLD_COUNTS = 140;
static constexpr int32_t MPU6050_STATIONARY_GYRO_THRESHOLD_DPS10 = 12;
static constexpr int32_t MPU6050_ZERO_TRACK_DIVISOR = 64;

static int32_t mpu6050TempRawToDegC10(int16_t raw_temp)
{
    // MPU6050 datasheet: temperature in C = raw / 340 + 36.53.
    // Store the shared value as tenths of degrees C without introducing floats.
    return ((static_cast<int32_t>(raw_temp) * 10) + 170) / 340 + MPU6050_TEMP_OFFSET_DEGC10;
}

static int32_t divideRoundNearest(int32_t numerator, int32_t denominator)
{
    if (numerator >= 0)
    {
        return (numerator + denominator / 2) / denominator;
    }
    return (numerator - denominator / 2) / denominator;
}

static int32_t mpu6050GyroRawToDegPs10(int16_t raw_gyro, int32_t bias_dps10 = 0)
{
    // The active gyro range is the MPU6050 default +-250 dps, which gives 131 LSB per dps.
    // Store gyro rates as deg/s * 10 so heading can integrate into deg * 10 cleanly.
    const int32_t scaled_dps10 = divideRoundNearest(static_cast<int32_t>(raw_gyro) * MPU6050_GYRO_SCALE_FACTOR, MPU6050_GYRO_LSB_PER_DPS);
    return scaled_dps10 + bias_dps10;
}

static int32_t applyEma(int32_t previous_value, int32_t new_value, int32_t numerator, int32_t denominator)
{
    // Keep gyro and temperature semantics aligned with the plain MPU6050 env so
    // the environment switch changes filter strategy, not the published contract.
    const int32_t keep_weight = denominator - numerator;
    return (previous_value * keep_weight + new_value * numerator) / denominator;
}

static int32_t applyGyroDeadband(int32_t yaw_rate_dps10)
{
    if (abs(yaw_rate_dps10) < MPU6050_GYRO_Z_DEADBAND_DPS10)
    {
        return 0;
    }
    return yaw_rate_dps10;
}

static int32_t applyAccelDeadband(int32_t centered_accel_counts)
{
    if (abs(centered_accel_counts) < MPU6050_ACCEL_DEADBAND_COUNTS)
    {
        return 0;
    }
    return centered_accel_counts;
}

static bool accelBiasCanTrack(int32_t acc_x, int32_t acc_y, int32_t acc_z, int32_t gy_x, int32_t gy_y, int32_t gy_z)
{
    return abs(acc_x) < MPU6050_STATIONARY_ACCEL_THRESHOLD_COUNTS &&
           abs(acc_y) < MPU6050_STATIONARY_ACCEL_THRESHOLD_COUNTS &&
           abs(acc_z) < MPU6050_STATIONARY_ACCEL_THRESHOLD_COUNTS &&
           abs(gy_x) < MPU6050_STATIONARY_GYRO_THRESHOLD_DPS10 &&
           abs(gy_y) < MPU6050_STATIONARY_GYRO_THRESHOLD_DPS10 &&
           abs(gy_z) < MPU6050_STATIONARY_GYRO_THRESHOLD_DPS10;
}

static long trackZeroBias(long current_zero, int16_t raw_sample)
{
    return current_zero + divideRoundNearest(static_cast<int32_t>(raw_sample) - current_zero, MPU6050_ZERO_TRACK_DIVISOR);
}

static int32_t mpu6050AccelRawToMmPs2(int32_t raw_accel)
{
    // ACCEL_CONFIG is left at the default +-2g range, which gives 16384 LSB/g.
    return divideRoundNearest(raw_accel * GRAVITY_MMPS2, MPU6050_ACCEL_LSB_PER_G);
}

//float processNoise, float measurementNoise, float estimationError, float initialValue)

KalmanFilter kalmanFilterX(1.0f, 0.002f, 1.0f, 0);
KalmanFilter kalmanFilterY(1.0f, 0.002f, 1.0f, 0);
KalmanFilter kalmanFilterZ(1.0f, 0.002f, 1.0f, 0);

ACCsensor::ACCsensor()
{
}

static void accel_task(void *pvParameters)
{
    uint32_t previous_sample_ms = millis();

    for (;;)
    {
        const uint32_t now_ms = millis();
        const int32_t dt_ms = min(static_cast<int32_t>(now_ms - previous_sample_ms), MPU6050_MAX_INTEGRATION_DT_MS);
        previous_sample_ms = now_ms;

        // Serial.print(".");
        task_safe_wire_begin(MPU6050_ADDR);
        task_safe_wire_write(0x3B); // starting with register 0x3B (ACCEL_XOUT_H)
        task_safe_wire_restart();
        task_safe_wire_request_from(MPU6050_ADDR, 14); // request a total of 14 registers

        // read accelerometer and gyroscope data
        //+++++++++++++++++++++++++++++++++
        // X direction = forward/backwards of the truck
        int16_t AcX = task_safe_wire_read() << 8 | task_safe_wire_read(); // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)
        int16_t AcY = task_safe_wire_read() << 8 | task_safe_wire_read(); // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
        int16_t AcZ = task_safe_wire_read() << 8 | task_safe_wire_read(); // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
        int16_t Tmp = task_safe_wire_read() << 8 | task_safe_wire_read(); // 0x41 (TEMP_OUT_H) & 0x42 (TEMP_OUT_L)
        int16_t GyX = task_safe_wire_read() << 8 | task_safe_wire_read(); // 0x43 (GYRO_XOUT_H) & 0x44 (GYRO_XOUT_L)
        int16_t GyY = task_safe_wire_read() << 8 | task_safe_wire_read(); // 0x45 (GYRO_YOUT_H) & 0x46 (GYRO_YOUT_L)
        int16_t GyZ_raw = task_safe_wire_read() << 8 | task_safe_wire_read(); // 0x47 (GYRO_ZOUT_H) & 0x48 (GYRO_ZOUT_L)

        long zAx = globalVar_get(zeroAx);
        long zAy = globalVar_get(zeroAy);
        long zAz = globalVar_get(zeroAz);
        const long zGz = globalVar_get(zeroGz);

        const int32_t rawGyX_dps10 = mpu6050GyroRawToDegPs10(GyX);
        const int32_t rawGyY_dps10 = mpu6050GyroRawToDegPs10(GyY);
        const int32_t rawGyZ_dps10 = -mpu6050GyroRawToDegPs10(GyZ_raw);

        const int32_t filteredGyX = applyEma(globalVar_get(cleanedGyX), rawGyX_dps10, MPU6050_AUX_EMA_ALPHA_NUMERATOR, MPU6050_AUX_EMA_ALPHA_DENOMINATOR);
        const int32_t filteredGyY = applyEma(globalVar_get(cleanedGyY), rawGyY_dps10, MPU6050_AUX_EMA_ALPHA_NUMERATOR, MPU6050_AUX_EMA_ALPHA_DENOMINATOR);
        const int32_t filteredGyZ = applyGyroDeadband(
            applyEma(globalVar_get(cleanedGyZ), -mpu6050GyroRawToDegPs10(static_cast<int16_t>(GyZ_raw - zGz)), MPU6050_AUX_EMA_ALPHA_NUMERATOR, MPU6050_AUX_EMA_ALPHA_DENOMINATOR));

        if (accelBiasCanTrack(globalVar_get(cleanedAccX), globalVar_get(cleanedAccY), globalVar_get(cleanedAccZ), filteredGyX, filteredGyY, filteredGyZ))
        {
            zAx = trackZeroBias(zAx, AcX);
            zAy = trackZeroBias(zAy, AcY);
            zAz = trackZeroBias(zAz, AcZ);
            globalVar_set(zeroAx, zAx);
            globalVar_set(zeroAy, zAy);
            globalVar_set(zeroAz, zAz);
        }

        // The Kalman env differs only in accelerometer filtering, but still
        // shares the same centered raw-data contract and stationary bias tracking.
        const int32_t filteredAcX = static_cast<int32_t>(kalmanFilterX.updateEstimate((int)AcX - zAx));
        const int32_t filteredAcY = static_cast<int32_t>(kalmanFilterY.updateEstimate(AcY - zAy));
        const int32_t filteredAcZ = static_cast<int32_t>(kalmanFilterZ.updateEstimate(AcZ - zAz));

        globalVar_set(rawAccX, AcX);
        globalVar_set(rawAccY, AcY);
        globalVar_set(rawAccZ, AcZ);
        globalVar_set(cleanedAccX, filteredAcX);
        globalVar_set(cleanedAccY, filteredAcY);
        globalVar_set(cleanedAccZ, filteredAcZ);
        (void)mpu6050AccelRawToMmPs2(filteredAcX);
        // Keep the Kalman and non-Kalman envs comparable: both should avoid
        // publishing inertial-only speed and distance until tilt-compensated
        // motion estimation exists.
        globalVar_set(calcSpeed, 0);
        globalVar_set(calcDistance, 0);
        globalVar_set(rawTemp, applyEma(globalVar_get(rawTemp), mpu6050TempRawToDegC10(Tmp), MPU6050_AUX_EMA_ALPHA_NUMERATOR, MPU6050_AUX_EMA_ALPHA_DENOMINATOR));
        globalVar_set(rawGyX, rawGyX_dps10);
        globalVar_set(rawGyY, rawGyY_dps10);
        globalVar_set(rawGyZ, rawGyZ_dps10);
        globalVar_set(cleanedGyX, filteredGyX);
        globalVar_set(cleanedGyY, filteredGyY);
        globalVar_set(cleanedGyZ, filteredGyZ);
        //-----------------------
        // Z dimension of the gyro gives us how quickly the direction of the truck changes
        const long GyZ_degps10 = filteredGyZ;
        const long old_heading = globalVar_get(calcHeading);
        globalVar_set(calcHeading, old_heading + divideRoundNearest(GyZ_degps10 * dt_ms, 1000));
        task_safe_wire_end();
        //------------
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void ACCsensor::Begin()
{

    globalVar_set(calcSpeed, 0);
    globalVar_set(calcDistance, 0);
    globalVar_set(calcHeading, 0);
    globalVar_set(cleanedAccX, 0);
    globalVar_set(cleanedAccY, 0);
    globalVar_set(cleanedAccZ, 0);
    globalVar_set(cleanedGyX, 0);
    globalVar_set(cleanedGyY, 0);
    globalVar_set(cleanedGyZ, 0);
    globalVar_set(zeroAy, 0);
    globalVar_set(zeroAz, 0);
    globalVar_set(zeroGz, 0);
    task_safe_wire_begin(MPU6050_ADDR);
    task_safe_wire_write(PWR_MGMT_1);
    task_safe_wire_write(0);
    task_safe_wire_end();
    // Set the SMPLRT_DIV register for 200 Hz sampling rate (4 = 1000 / (1 + 4) => 200 Hz)
    task_safe_wire_begin(MPU6050_ADDR);
    task_safe_wire_write(0x19); // SMPLRT_DIV register
    task_safe_wire_write(4);    // Set the divider to 4 for 200 Hz sampling rate
    task_safe_wire_end();
    // ACCEL_CONFIG register (0x1C) controls the accelerometer range
    // range should be 0 for ±2g, 1 for ±4g, 2 for ±8g, or 3 for ±16g
    task_safe_wire_begin(MPU6050_ADDR);
    task_safe_wire_write(0x1C);   // Address of ACCEL_CONFIG register
    task_safe_wire_write(0 << 3); // Shift the range value to the correct bit position 2 = 8g range
    task_safe_wire_end();

    task_safe_wire_begin(MPU6050_ADDR);
    task_safe_wire_write(0x75); // WHO_AM_I register
    task_safe_wire_restart();
    task_safe_wire_request_from(MPU6050_ADDR, 1); // Request 1 byte (WHO_AM_I)
    byte whoAmI = task_safe_wire_read();
    Serial.print("WHO_AM_I: 0x");
    Serial.println(whoAmI, HEX); // Should print 0x68 if MPU6050 is present
    task_safe_wire_end();
    vTaskDelay(pdMS_TO_TICKS(200));
    // We need to store the zero values avergaes
    long n = 0;
    long n_y = 0;
    long n_z = 0;
    long n_gz = 0;
    for (int i = 1; i < 50; i++)
    {
        task_safe_wire_begin(MPU6050_ADDR);
        task_safe_wire_write(0x3B); // starting with register 0x3B (ACCEL_XOUT_H)
        task_safe_wire_restart();
        task_safe_wire_request_from(MPU6050_ADDR, 14); // request a total of 14 registers
        int16_t tmp_AcX = task_safe_wire_read() << 8 | task_safe_wire_read();
        int16_t tmp_AcY = task_safe_wire_read() << 8 | task_safe_wire_read();
        int16_t tmp_AcZ = task_safe_wire_read() << 8 | task_safe_wire_read();
        task_safe_wire_read(); // Temp high
        task_safe_wire_read(); // Temp low
        task_safe_wire_read(); // GyX high
        task_safe_wire_read(); // GyX low
        task_safe_wire_read(); // GyY high
        task_safe_wire_read(); // GyY low
        int16_t tmp_GyZ = task_safe_wire_read() << 8 | task_safe_wire_read();
        task_safe_wire_end();
        n += tmp_AcX;
        n_y += tmp_AcY;
        n_z += tmp_AcZ;
        n_gz += tmp_GyZ;
        vTaskDelay(pdMS_TO_TICKS(21));
        globalVar_set(zeroAx, n / i);
        globalVar_set(zeroAy, n_y / i);
        globalVar_set(zeroAz, n_z / i);
        globalVar_set(zeroGz, n_gz / i);
    };

    Serial.print("Calculated offset: ");
    Serial.println(globalVar_get(zeroAx));
    Serial.print("Calculated Y offset: ");
    Serial.println(globalVar_get(zeroAy));
    Serial.print("Calculated Z offset: ");
    Serial.println(globalVar_get(zeroAz));
    Serial.print("Calculated gyro Z offset: ");
    Serial.println(globalVar_get(zeroGz));
    // globalVar_set(zeroAx,512);

    Serial.println("Starting ACCsens");
    xTaskCreate(
        accel_task,  // Task function
        "acceltask", // Task name
        2000,        // Stack size (in words, not bytes)
        NULL,        // Task input parameter
        2,           // Task priority
        NULL         // Task handle
    );
}
