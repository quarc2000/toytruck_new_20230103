#include <Arduino.h>

#include "actuators/motor.h"
#include "atmio.h"
#include "actuators/steer.h"
#include "fusion/fusion_service.h"
#include "lights/light_service.h"
#include "robots/driver.h"
#include "sensors/accsensor.h"
#include "sensors/front_vl53l0x_service.h"
#include "sensors/usensor.h"
#include "variables/setget.h"

Usensor ultraSound;
ACCsensor accsensor;
FrontVl53l0xService frontLidarService;
FusionService fusionService;
Driver driver;
LightService lightService;

void setup()
{
    Serial.begin(57600);
    delay(500);
    Serial.println();
    Serial.println("Front obstacle-avoidance test");
    Serial.println("PAT004 front fusion with driver runtime");

    globalVar_init();
    vTaskDelay(pdMS_TO_TICKS(100));

    Serial.print("US front: ");
    Serial.println(ultraSound.open(TRIGGER_PIN1, ECHO_PIN1, rawDistFront));
    Serial.print("US right: ");
    Serial.println(ultraSound.open(TRIGGER_PIN2, ECHO_PIN2, rawDistRight));
    Serial.print("US left: ");
    Serial.println(ultraSound.open(TRIGGER_PIN3, ECHO_PIN3, rawDistLeft));
    Serial.print("US back: ");
    Serial.println(ultraSound.open(TRIGGER_PIN4, ECHO_PIN4, rawDistBack));

    vTaskDelay(pdMS_TO_TICKS(100));
    accsensor.Begin();
    vTaskDelay(pdMS_TO_TICKS(100));
    frontLidarService.Begin();
    vTaskDelay(pdMS_TO_TICKS(100));
    fusionService.Begin();
    vTaskDelay(pdMS_TO_TICKS(100));
    driver.Begin();
    vTaskDelay(pdMS_TO_TICKS(100));
    lightService.Begin();
}

void loop()
{
    Serial.print("USf=");
    Serial.print(globalVar_get(rawDistFront));
    Serial.print("cm  L=");
    Serial.print(globalVar_get(rawLidarFrontLeft));
    Serial.print("mm  R=");
    Serial.print(globalVar_get(rawLidarFrontRight));
    Serial.print("mm  F=");
    Serial.print(globalVar_get(fuseForwardClear));
    Serial.print("  B=");
    Serial.print(globalVar_get(fuseTurnBias));
    Serial.print("  V=");
    Serial.print(globalVar_get(driver_desired_speed));
    Serial.print("  S=");
    Serial.println(globalVar_get(steerDirection));
    delay(1000);
}
