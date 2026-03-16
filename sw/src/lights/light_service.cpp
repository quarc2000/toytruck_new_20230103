#include <lights/light_service.h>

#include <variables/setget.h>

namespace {
constexpr uint8_t MAIN_LIGHT_PIN = 0;
constexpr uint8_t BRAKE_LIGHT_PIN = 1;
constexpr uint8_t LEFT_INDICATOR_PIN = 8;
constexpr uint8_t RIGHT_INDICATOR_PIN = 9;
constexpr uint8_t REVERSE_LIGHT_PIN = 11;

constexpr long INDICATOR_STEER_THRESHOLD = 10;
constexpr long BRAKE_DECEL_THRESHOLD_COUNTS = -120;
constexpr unsigned long INDICATOR_BLINK_PERIOD_MS = 500;
constexpr TickType_t LIGHT_TASK_PERIOD_TICKS = pdMS_TO_TICKS(50);
} // namespace

LightService::LightService()
    : expander(0x70, 0x20), blinkState(false), lastBlinkToggleMs(0) {}

void LightService::Begin()
{
    expander.initSwitch();
    expander.initGPIO();
    applyOutputs(false, false, false, false);
    xTaskCreate(taskEntry, "LightService", 4096, this, 1, nullptr);
}

void LightService::taskEntry(void *parameter)
{
    static_cast<LightService *>(parameter)->runTask();
}

void LightService::runTask()
{
    for (;;) {
        const unsigned long nowMs = millis();
        if (nowMs - lastBlinkToggleMs >= INDICATOR_BLINK_PERIOD_MS) {
            blinkState = !blinkState;
            lastBlinkToggleMs = nowMs;
        }

        const long steer = globalVar_get(steerDirection);
        const long desiredSpeed = globalVar_get(driver_desired_speed);
        const long longitudinalAcc = globalVar_get(cleanedAccX);

        const bool brakeOn = longitudinalAcc <= BRAKE_DECEL_THRESHOLD_COUNTS;
        const bool leftRequested = steer < -INDICATOR_STEER_THRESHOLD;
        const bool rightRequested = steer > INDICATOR_STEER_THRESHOLD;
        const bool reverseOn = desiredSpeed < 0;

        applyOutputs(
            brakeOn,
            leftRequested && blinkState,
            rightRequested && blinkState,
            reverseOn);

        vTaskDelay(LIGHT_TASK_PERIOD_TICKS);
    }
}

void LightService::applyOutputs(bool brakeOn, bool leftOn, bool rightOn, bool reverseOn)
{
    // Main light policy is not decided yet; keep it off while validating the
    // automatic outputs so brake, indicator, and reverse behavior stay obvious.
    expander.writeGPIO(MAIN_LIGHT_PIN, false);
    expander.writeGPIO(BRAKE_LIGHT_PIN, brakeOn);
    expander.writeGPIO(LEFT_INDICATOR_PIN, leftOn);
    expander.writeGPIO(RIGHT_INDICATOR_PIN, rightOn);
    expander.writeGPIO(REVERSE_LIGHT_PIN, reverseOn);
}
