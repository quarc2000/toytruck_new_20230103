#include <fusion/fusion_service.h>
#include <fusion/clearance_fusion.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <variables/setget.h>

static constexpr int32_t FUSE_FORWARD_UNKNOWN = -1;
static constexpr int32_t FUSE_TURN_NEUTRAL = 0;
static constexpr int32_t FUSE_HEADING_CORRECTION_MAX_STILL_DEG10 = 30;
static constexpr int32_t FUSE_HEADING_CORRECTION_MAX_TURNING_DEG10 = 8;
static constexpr int32_t FUSE_HEADING_CORRECTION_DIVISOR_STILL = 6;
static constexpr int32_t FUSE_HEADING_CORRECTION_DIVISOR_TURNING = 18;
static constexpr int32_t FUSE_HEADING_TURNING_GYRO_THRESHOLD_DPS10 = 120;
static constexpr TickType_t FUSION_FAST_PERIOD_MS = 20;
static constexpr TickType_t FUSION_SLOW_PERIOD_MS = 50;

namespace
{
int32_t wrapHeadingDeg10(int32_t heading)
{
    while (heading >= 3600) heading -= 3600;
    while (heading < 0) heading += 3600;
    return heading;
}

int32_t wrappedHeadingErrorDeg10(int32_t target, int32_t current)
{
    int32_t error = wrapHeadingDeg10(target) - wrapHeadingDeg10(current);
    if (error > 1800) error -= 3600;
    if (error < -1800) error += 3600;
    return error;
}

int32_t clampSigned(int32_t value, int32_t limit)
{
    if (value > limit) return limit;
    if (value < -limit) return -limit;
    return value;
}
}

static void fusionFastTask(void *pvParameters)
{
    for (;;)
    {
        globalVar_set(fuseForwardClear, fusionComputeForwardClear());
        globalVar_set(fuseTurnBias, fusionComputeTurnBias());
        vTaskDelay(pdMS_TO_TICKS(FUSION_FAST_PERIOD_MS));
    }
}

static void fusionSlowTask(void *pvParameters)
{
    int32_t lastGyroHeadingDeg10 = wrapHeadingDeg10(globalVar_get(calcHeading));
    int32_t fusedHeadingDeg10 = lastGyroHeadingDeg10;
    bool fusedInitialized = false;

    for (;;)
    {
        const int32_t gyroHeadingDeg10 = wrapHeadingDeg10(globalVar_get(calcHeading));
        const int32_t magHeadingDeg10 = wrapHeadingDeg10(globalVar_get(calculatedMagCourse));
        const bool gy271Present = globalVar_get(configGy271Present) != 0;
        const bool magHeadingValid = globalVar_get(configMagHeadingValid) != 0;
        const bool magDisturbed = globalVar_get(calculatedMagDisturbance) != 0;
        const int32_t yawRateDegPs10 = abs(globalVar_get(cleanedGyZ));

        if (!fusedInitialized)
        {
            // Never let the published fused heading freeze at startup zero while
            // waiting for the first real magnetic heading sample. Follow the
            // gyro immediately, then snap in the absolute magnetic reference as
            // soon as the magnetometer has published a real course.
            fusedHeadingDeg10 = gyroHeadingDeg10;
            if (gy271Present)
            {
                if (magHeadingValid)
                {
                    fusedHeadingDeg10 = magHeadingDeg10;
                    fusedInitialized = true;
                    globalVar_set(configHeadingReady, 1);
                }
            }
            else
            {
                fusedHeadingDeg10 = gyroHeadingDeg10;
                fusedInitialized = true;
                globalVar_set(configHeadingReady, 1);
            }
        }
        else
        {
            // Follow the short-term motion from the gyro so turns stay responsive.
            fusedHeadingDeg10 = wrapHeadingDeg10(fusedHeadingDeg10 + wrappedHeadingErrorDeg10(gyroHeadingDeg10, lastGyroHeadingDeg10));
        }

        if (fusedInitialized && !magDisturbed)
        {
            const int32_t headingErrorDeg10 = wrappedHeadingErrorDeg10(magHeadingDeg10, fusedHeadingDeg10);
            const bool turningQuickly = yawRateDegPs10 > FUSE_HEADING_TURNING_GYRO_THRESHOLD_DPS10;
            const int32_t correctionDivisor = turningQuickly ? FUSE_HEADING_CORRECTION_DIVISOR_TURNING : FUSE_HEADING_CORRECTION_DIVISOR_STILL;
            const int32_t correctionMax = turningQuickly ? FUSE_HEADING_CORRECTION_MAX_TURNING_DEG10 : FUSE_HEADING_CORRECTION_MAX_STILL_DEG10;
            const int32_t correctionDeg10 = clampSigned(headingErrorDeg10 / correctionDivisor, correctionMax);
            fusedHeadingDeg10 = wrapHeadingDeg10(fusedHeadingDeg10 + correctionDeg10);
        }

        lastGyroHeadingDeg10 = gyroHeadingDeg10;
        globalVar_set(fuseHeadingDeg10, fusedHeadingDeg10);
        vTaskDelay(pdMS_TO_TICKS(FUSION_SLOW_PERIOD_MS));
    }
}

FusionService::FusionService()
{
}

void FusionService::Begin()
{
    globalVar_set(fuseForwardClear, FUSE_FORWARD_UNKNOWN);
    globalVar_set(fuseTurnBias, FUSE_TURN_NEUTRAL);
    globalVar_set(fuseHeadingDeg10, wrapHeadingDeg10(globalVar_get(calcHeading)));
    if (globalVar_get(configGy271Present) == 0)
    {
        globalVar_set(configMagHeadingValid, 1);
    }
    globalVar_set(configHeadingReady, 0);

    xTaskCreate(
        fusionFastTask,
        "fusionFast",
        2000,
        NULL,
        1,
        NULL);

    xTaskCreate(
        fusionSlowTask,
        "fusionSlow",
        2000,
        NULL,
        1,
        NULL);
}
