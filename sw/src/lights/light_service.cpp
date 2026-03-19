#include <lights/light_service.h>

#include <basic_telemetry/basic_logger.h>
#include <task_safe_wire.h>
#include <variables/setget.h>

namespace {
constexpr uint8_t TCA9548_ADDR = 0x70;
constexpr uint8_t MAIN_LIGHT_PIN = 0;
constexpr uint8_t BRAKE_LIGHT_PIN = 1;
constexpr uint8_t LEFT_INDICATOR_PIN = 8;
constexpr uint8_t RIGHT_INDICATOR_PIN = 9;
constexpr uint8_t REVERSE_LIGHT_PIN = 11;

constexpr long INDICATOR_STEER_THRESHOLD = 20;
constexpr long BRAKE_DECEL_THRESHOLD_COUNTS = -120;
constexpr unsigned long INDICATOR_BLINK_PERIOD_MS = 250;
constexpr unsigned long BRAKE_HOLD_PERIOD_MS = 1000;
constexpr TickType_t LIGHT_TASK_PERIOD_TICKS = pdMS_TO_TICKS(50);

bool deviceResponds(uint8_t address)
{
    task_safe_wire_begin(address);
    const uint8_t result = task_safe_wire_end();
    return result == 0;
}
} // namespace

LightService::LightService()
    : expander(0x70, 0x20), begun(false), blinkState(false), lastBlinkToggleMs(0), brakeHoldUntilMs(0) {}

void LightService::Begin()
{
    if (begun) {
        return;
    }
    const bool expanderPresent = deviceResponds(TCA9548_ADDR);
    globalVar_set(configExpanderPresent, expanderPresent ? 1 : globalVar_get(configExpanderPresent));
    if (!expanderPresent) {
        basic_log_info("LightService mux not detected, light path disabled");
        begun = true;
        return;
    }
    basic_log_info("LightService mux detected");
    expander.initSwitch();
    expander.initGPIO();
    applyOutputs(false, false, false, false, false);
    xTaskCreate(taskEntry, "LightService", 4096, this, 1, nullptr);
    begun = true;
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

        if (longitudinalAcc <= BRAKE_DECEL_THRESHOLD_COUNTS) {
            brakeHoldUntilMs = nowMs + BRAKE_HOLD_PERIOD_MS;
        }

        const bool brakeOn = nowMs <= brakeHoldUntilMs;
        const bool leftRequested = steer < -INDICATOR_STEER_THRESHOLD;
        const bool rightRequested = steer > INDICATOR_STEER_THRESHOLD;
        const bool mainOn = desiredSpeed > 0;
        const bool reverseOn = desiredSpeed < 0;

        applyOutputs(
            mainOn,
            brakeOn,
            leftRequested && blinkState,
            rightRequested && blinkState,
            reverseOn);

        vTaskDelay(LIGHT_TASK_PERIOD_TICKS);
    }
}

void LightService::applyOutputs(bool mainOn, bool brakeOn, bool leftOn, bool rightOn, bool reverseOn)
{
    expander.writeGPIO(MAIN_LIGHT_PIN, mainOn);
    expander.writeGPIO(BRAKE_LIGHT_PIN, brakeOn);
    expander.writeGPIO(LEFT_INDICATOR_PIN, leftOn);
    expander.writeGPIO(RIGHT_INDICATOR_PIN, rightOn);
    expander.writeGPIO(REVERSE_LIGHT_PIN, reverseOn);
}
