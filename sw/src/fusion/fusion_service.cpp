#include <fusion/fusion_service.h>
#include <fusion/clearance_fusion.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <variables/setget.h>

static constexpr int32_t FUSE_FORWARD_UNKNOWN = -1;
static constexpr int32_t FUSE_TURN_NEUTRAL = 0;
static constexpr TickType_t FUSION_FAST_PERIOD_MS = 100;
static constexpr TickType_t FUSION_SLOW_PERIOD_MS = 1000;

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
    for (;;)
    {
        // The first minimal fusion increment only has fast forward-clearance
        // logic. Keep the slow task alive as the future home for pose- and
        // map-related fusion so the package structure matches the architecture.
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
