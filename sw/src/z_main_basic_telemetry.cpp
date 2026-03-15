#include <Arduino.h>

#include <basic_telemetry/basic_logger.h>
#include <basic_telemetry/basic_web_server.h>
#include <fusion/fusion_service.h>
#include <sensors/accsensor.h>
#include <sensors/usensor.h>
#include <variables/setget.h>
#include <atmio.h>

int triggerPins[NUM_SENSORS] = {TRIGGER_PIN1, TRIGGER_PIN2, TRIGGER_PIN3, TRIGGER_PIN4};
int echoPins[NUM_SENSORS] = {ECHO_PIN1, ECHO_PIN2, ECHO_PIN3, ECHO_PIN4};

Usensor ultraSound;
ACCsensor accsensor;
FusionService fusionService;
BasicWebServerService basicWebServer;

unsigned long lastHeartbeatMs = 0;

void setup()
{
    Serial.begin(57600);
    globalVar_init();
    basic_log_init();

    const uint64_t chipMac = ESP.getEfuseMac();
    const String chipId = String((uint32_t)(chipMac >> 32), HEX) + String((uint32_t)chipMac, HEX);

    basic_log_info("Starting basic telemetry test");
    basic_log_info("Chip ID " + chipId);

    accsensor.Begin();
    basic_log_info("MPU6050 path started");

    ultraSound.open(TRIGGER_PIN1, ECHO_PIN1, rawDistFront);
    ultraSound.open(TRIGGER_PIN2, ECHO_PIN2, rawDistRight);
    ultraSound.open(TRIGGER_PIN3, ECHO_PIN3, rawDistLeft);
    ultraSound.open(TRIGGER_PIN4, ECHO_PIN4, rawDistBack);
    basic_log_info("Ultrasonic tasks opened");

    fusionService.Begin();
    basic_log_info("Fusion service started");

    globalVar_set(driver_desired_speed, 0);
    globalVar_set(steerDirection, 0);

    basicWebServer.Begin(chipId);
}

void loop()
{
    basicWebServer.Loop();

    const unsigned long nowMs = millis();
    if (nowMs - lastHeartbeatMs >= 3000) {
        lastHeartbeatMs = nowMs;
        basic_log_info(
            "Heartbeat heading=" + String(globalVar_get(calcHeading)) +
            " front=" + String(globalVar_get(rawDistFront)) +
            " clear=" + String(globalVar_get(fuseForwardClear)));
    }

    delay(20);
}
