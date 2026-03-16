#include <Arduino.h>

#include <basic_telemetry/basic_logger.h>
#include <basic_telemetry/basic_web_server.h>
#include <variables/setget.h>

BasicWebServerService basicWebServerMin;

unsigned long lastHeartbeatMs = 0;
unsigned long lastVarUpdateMs = 0;
long fakeHeading = 0;
long fakeFrontDist = 120;
long fakeAccX = 0;

void setup()
{
    Serial.begin(57600);
    globalVar_init();
    basic_log_init();

    const uint64_t chipMac = ESP.getEfuseMac();
    const String chipId = String((uint32_t)(chipMac >> 32), HEX) + String((uint32_t)chipMac, HEX);

    basic_log_info("Starting minimal basic telemetry test");
    basic_log_info("Chip ID " + chipId);

    globalVar_set(rawDistFront, fakeFrontDist);
    globalVar_set(rawDistBack, 140);
    globalVar_set(rawAccX, fakeAccX);
    globalVar_set(rawAccY, 0);
    globalVar_set(rawAccZ, 0);
    globalVar_set(cleanedAccX, fakeAccX);
    globalVar_set(cleanedAccY, 0);
    globalVar_set(cleanedAccZ, 0);
    globalVar_set(rawGyZ, 0);
    globalVar_set(cleanedGyZ, 0);
    globalVar_set(calcHeading, fakeHeading);
    globalVar_set(fuseForwardClear, 1);
    globalVar_set(steerDirection, 0);
    globalVar_set(driver_desired_speed, 0);

    basicWebServerMin.Begin(chipId);
}

void loop()
{
    basicWebServerMin.Loop();

    const unsigned long nowMs = millis();
    if (nowMs - lastVarUpdateMs >= 1000) {
        lastVarUpdateMs = nowMs;
        fakeHeading = (fakeHeading + 15) % 3600;
        fakeFrontDist -= 5;
        if (fakeFrontDist < 40) {
            fakeFrontDist = 140;
        }
        fakeAccX = (fakeAccX == 0) ? 25 : -25;

        globalVar_set(rawDistFront, fakeFrontDist);
        globalVar_set(rawDistBack, fakeFrontDist + 20);
        globalVar_set(rawAccX, fakeAccX);
        globalVar_set(cleanedAccX, fakeAccX);
        globalVar_set(rawGyZ, fakeAccX);
        globalVar_set(cleanedGyZ, fakeAccX);
        globalVar_set(calcHeading, fakeHeading);
        globalVar_set(fuseForwardClear, fakeFrontDist > 60 ? 1 : 0);
    }

    if (nowMs - lastHeartbeatMs >= 3000) {
        lastHeartbeatMs = nowMs;
        basic_log_info(
            "Heartbeat heading=" + String(globalVar_get(calcHeading)) +
            " front=" + String(globalVar_get(rawDistFront)) +
            " clear=" + String(globalVar_get(fuseForwardClear)));
    }

    delay(10);
}
