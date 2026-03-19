#include "sensors/front_vl53l0x_service.h"

#include <Arduino.h>
#include <basic_telemetry/basic_logger.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "expander.h"
#include "task_safe_wire.h"
#include "sensors/vl53l0x_basic.h"
#include "variables/setget.h"

namespace
{
constexpr uint8_t TCA9548_ADDR = 0x70;
constexpr uint8_t MCP23017_ADDR = 0x20;
constexpr uint8_t VL53L0X_ADDR = 0x29;
constexpr uint8_t FRONT_RIGHT_CHANNEL = 1;
constexpr uint8_t FRONT_LEFT_CHANNEL = 2;
constexpr TickType_t FRONT_LIDAR_PERIOD_MS = 100;
constexpr TickType_t INIT_RETRY_MS = 1000;
constexpr int32_t LIDAR_NO_RETURN_MM = 2999;
constexpr int32_t LIDAR_MIN_PLAUSIBLE_MM = 40;
constexpr int32_t LIDAR_MAX_PLAUSIBLE_MM = 2999;

EXPANDER front_lidar_expander(TCA9548_ADDR, MCP23017_ADDR);
Vl53l0xBasic front_right_sensor(VL53L0X_ADDR);
Vl53l0xBasic front_left_sensor(VL53L0X_ADDR);
bool front_right_ready = false;
bool front_left_ready = false;
bool front_lidar_task_started = false;
bool front_lidar_hw_present = false;

bool deviceResponds(uint8_t address)
{
    task_safe_wire_begin(address);
    const uint8_t result = task_safe_wire_end();
    return result == 0;
}

int32_t sanitizeDistanceMm(uint16_t distance_mm)
{
    if (distance_mm < LIDAR_MIN_PLAUSIBLE_MM)
    {
        return LIDAR_NO_RETURN_MM;
    }

    if (distance_mm > LIDAR_MAX_PLAUSIBLE_MM)
    {
        return LIDAR_NO_RETURN_MM;
    }

    return distance_mm;
}

bool initSensor(uint8_t channel, Vl53l0xBasic &sensor)
{
    task_safe_wire_lock();
    front_lidar_expander.setChannel(channel);
    const bool ok = sensor.probe() && sensor.init();
    front_lidar_expander.setChannel(0);
    task_safe_wire_unlock();
    return ok;
}

bool readSensor(uint8_t channel, Vl53l0xBasic &sensor, uint16_t &distance_mm)
{
    task_safe_wire_lock();
    front_lidar_expander.setChannel(channel);
    const bool ok = sensor.readDistanceMm(distance_mm);
    front_lidar_expander.setChannel(0);
    task_safe_wire_unlock();
    return ok;
}

void publishFrontLidarValues(int32_t right_mm, int32_t left_mm)
{
    globalVar_set(rawLidarFrontRight, right_mm);
    globalVar_set(rawLidarFrontLeft, left_mm);

    if (right_mm > 0 && left_mm > 0)
    {
        globalVar_set(rawLidarFront, right_mm < left_mm ? right_mm : left_mm);
    }
    else if (right_mm > 0)
    {
        globalVar_set(rawLidarFront, right_mm);
    }
    else if (left_mm > 0)
    {
        globalVar_set(rawLidarFront, left_mm);
    }
    else
    {
        globalVar_set(rawLidarFront, 0);
    }
}

void frontLidarTask(void *pvParameters)
{
    TickType_t last_init_attempt_ms = 0;
    (void)pvParameters;

    for (;;)
    {
        const TickType_t now_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
        if ((!front_right_ready || !front_left_ready) &&
            (now_ms - last_init_attempt_ms >= INIT_RETRY_MS))
        {
            if (!front_right_ready)
            {
                front_right_ready = initSensor(FRONT_RIGHT_CHANNEL, front_right_sensor);
            }
            if (!front_left_ready)
            {
                front_left_ready = initSensor(FRONT_LEFT_CHANNEL, front_left_sensor);
            }
            last_init_attempt_ms = now_ms;
        }

        int32_t right_mm = 0;
        int32_t left_mm = 0;

        if (front_right_ready)
        {
            uint16_t distance_mm = 0;
            if (readSensor(FRONT_RIGHT_CHANNEL, front_right_sensor, distance_mm))
            {
                right_mm = sanitizeDistanceMm(distance_mm);
            }
            else
            {
                front_right_ready = false;
            }
        }

        if (front_left_ready)
        {
            uint16_t distance_mm = 0;
            if (readSensor(FRONT_LEFT_CHANNEL, front_left_sensor, distance_mm))
            {
                left_mm = sanitizeDistanceMm(distance_mm);
            }
            else
            {
                front_left_ready = false;
            }
        }

        publishFrontLidarValues(right_mm, left_mm);
        vTaskDelay(pdMS_TO_TICKS(FRONT_LIDAR_PERIOD_MS));
    }
}
} // namespace

FrontVl53l0xService::FrontVl53l0xService()
{
}

void FrontVl53l0xService::Begin()
{
    if (front_lidar_task_started)
    {
        return;
    }

    globalVar_set(rawLidarFront, 0);
    globalVar_set(rawLidarFrontRight, 0);
    globalVar_set(rawLidarFrontLeft, 0);
    globalVar_set(configFrontLidarPresent, 0);

    front_lidar_hw_present = deviceResponds(TCA9548_ADDR);
    globalVar_set(configExpanderPresent, front_lidar_hw_present ? 1 : globalVar_get(configExpanderPresent));
    if (!front_lidar_hw_present)
    {
        Serial.println("Front VL53L0X expander not present, lidar path disabled");
        basic_log_info("Front VL53L0X mux not detected, lidar path disabled");
        front_lidar_task_started = true;
        return;
    }

    front_lidar_expander.initSwitch();
    front_right_ready = initSensor(FRONT_RIGHT_CHANNEL, front_right_sensor);
    front_left_ready = initSensor(FRONT_LEFT_CHANNEL, front_left_sensor);

    Serial.print("Front VL53L0X right init: ");
    Serial.println(front_right_ready ? "OK" : "FAIL");
    Serial.print("Front VL53L0X left init: ");
    Serial.println(front_left_ready ? "OK" : "FAIL");
    globalVar_set(configFrontLidarPresent, (front_right_ready || front_left_ready) ? 1 : 0);
    basic_log_info(String("Front VL53L0X detected: ") + ((front_right_ready || front_left_ready) ? "yes" : "no"));

    xTaskCreate(
        frontLidarTask,
        "frontLidar",
        3000,
        NULL,
        1,
        NULL);
    front_lidar_task_started = true;
}
