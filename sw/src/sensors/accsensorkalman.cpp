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
static constexpr int32_t MPU6050_GYRO_Z_BIAS_DPS10 = 20;

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

//float processNoise, float measurementNoise, float estimationError, float initialValue)

KalmanFilter kalmanFilterX(1.0f, 0.002f, 1.0f, 0);
KalmanFilter kalmanFilterY(1.0f, 0.002f, 1.0f, 0);
KalmanFilter kalmanFilterZ(1.0f, 0.002f, 1.0f, 0);

ACCsensor::ACCsensor()
{
}

static void accel_task(void *pvParameters)
{
    for (;;)
    {
        // Serial.print(".");
        task_safe_wire_begin(MPU6050_ADDR);
        task_safe_wire_write(0x3B); // starting with register 0x3B (ACCEL_XOUT_H)
        task_safe_wire_restart();
        task_safe_wire_request_from(MPU6050_ADDR, 14); // request a total of 14 registers

        // read accelerometer and gyroscope data
        //+++++++++++++++++++++++++++++++++
        // X direction = forward/backwards of the truck
        int16_t AcX = task_safe_wire_read() << 8 | task_safe_wire_read(); // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)
        long zAx = globalVar_get(zeroAx);
        // calculate distance
        long distance_age;
        long old_distance;
        long old_speed;
        long speed_age;
        old_speed = globalVar_get(calcSpeed, &speed_age);
        old_distance = globalVar_get(calcDistance, &distance_age);
        // globalVar_get(calcDistance,old_speed,&speed_age);
        globalVar_set(calcDistance, old_distance + (old_speed * distance_age / (8 * 2048))); // Distance based on old speed, times how long since last update, in mm
        // calculate speed
        globalVar_set(calcSpeed, (int)(old_speed + (((AcX - zAx)) * speed_age))); // Speed based on old acc, times how long sonce last update
        // finally update the new accelectaion
        float filteredAcX = kalmanFilterX.updateEstimate((int)AcX - zAx);
        globalVar_set(rawAccX, (int)filteredAcX);
        //----------------------------
        int16_t AcY = task_safe_wire_read() << 8 | task_safe_wire_read(); // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
        float filteredAcY = kalmanFilterY.updateEstimate(AcY);
        globalVar_set(rawAccY, (int)filteredAcY);
        int16_t AcZ = task_safe_wire_read() << 8 | task_safe_wire_read(); // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
        float filteredAcZ = kalmanFilterZ.updateEstimate(AcZ);
        globalVar_set(rawAccZ, (int)filteredAcZ);
        int16_t Tmp = task_safe_wire_read() << 8 | task_safe_wire_read(); // 0x41 (TEMP_OUT_H) & 0x42 (TEMP_OUT_L)
        globalVar_set(rawTemp, mpu6050TempRawToDegC10(Tmp));
        int16_t GyX = task_safe_wire_read() << 8 | task_safe_wire_read(); // 0x43 (GYRO_XOUT_H) & 0x44 (GYRO_XOUT_L)
        globalVar_set(rawGyX, mpu6050GyroRawToDegPs10(GyX));
        int16_t GyY = task_safe_wire_read() << 8 | task_safe_wire_read(); // 0x45 (GYRO_YOUT_H) & 0x46 (GYRO_YOUT_L)
        globalVar_set(rawGyY, mpu6050GyroRawToDegPs10(GyY));
        //-----------------------
        // Z dimension of the gyro gives us how quickly the direction of the truck changes
        int16_t GyZ_raw = task_safe_wire_read() << 8 | task_safe_wire_read(); // 0x47 (GYRO_ZOUT_H) & 0x48 (GYRO_ZOUT_L)
        const long GyZ_degps10 = mpu6050GyroRawToDegPs10(GyZ_raw, MPU6050_GYRO_Z_BIAS_DPS10);
        // calculate heading
        long heading_age;
        long old_heading;
        old_heading = globalVar_get(calcHeading, &heading_age);
        globalVar_set(calcHeading, old_heading + (GyZ_degps10 * heading_age) / 1000); // Integrate deg/s*10 over elapsed ms into heading in deg*10.
        globalVar_set(rawGyZ, GyZ_degps10);                            // Store the latest yaw rate in deg/s*10.
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
    for (int i = 1; i < 50; i++)
    {
        task_safe_wire_begin(MPU6050_ADDR);
        task_safe_wire_write(0x3B); // starting with register 0x3B (ACCEL_XOUT_H)
        task_safe_wire_restart();
        task_safe_wire_request_from(MPU6050_ADDR, 14); // request a total of 14 registers
        int16_t tmp_AcX = task_safe_wire_read() << 8 | task_safe_wire_read();
        task_safe_wire_end();
        n += tmp_AcX;
        vTaskDelay(pdMS_TO_TICKS(21));
        globalVar_set(zeroAx, n / i);
    };

    Serial.print("Calculated offset: ");
    Serial.println(globalVar_get(zeroAx));
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
