#include <Arduino.h>

#include <ESP.h>

#include "basic_telemetry/basic_logger.h"
#include "basic_telemetry/explore_web_server.h"
#include "fusion/fusion_service.h"
#include "lights/light_service.h"
#include "navigation/observed_explorer.h"
#include "sensors/accsensor.h"
#include "sensors/front_vl53l0x_service.h"
#include "sensors/gy271_service.h"
#include "sensors/usensor.h"
#include "variables/setget.h"
#include "atmio.h"

Usensor ultraSound;
ACCsensor accsensor;
FrontVl53l0xService frontLidarService;
FusionService fusionService;
ObservedExplorerService observedExplorer;
LightService lightService;
ExploreWebServerService webServer;

static String chipIdString()
{
    const uint64_t chipId = ESP.getEfuseMac();
    char buffer[17];
    snprintf(buffer, sizeof(buffer), "%08X%08X", static_cast<uint32_t>(chipId >> 32), static_cast<uint32_t>(chipId));
    return String(buffer);
}

void setup()
{
    Serial.begin(57600);
    delay(500);
    Serial.println();
    Serial.println("Observed-map exploration test");
    Serial.println("PAT004 observed map and frontier exploration runtime");

    globalVar_init();
    basic_log_init();
    globalVar_set(configExpanderPresent, 0);
    globalVar_set(configGy271Present, 0);
    globalVar_set(configFrontLidarPresent, 0);
    basic_log_info("Observed exploration boot");
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
    start_gy271_service();
    vTaskDelay(pdMS_TO_TICKS(100));
    frontLidarService.Begin();
    vTaskDelay(pdMS_TO_TICKS(100));
    fusionService.Begin();
    vTaskDelay(pdMS_TO_TICKS(100));
    observedExplorer.Begin();
    vTaskDelay(pdMS_TO_TICKS(100));
    lightService.Begin();
    vTaskDelay(pdMS_TO_TICKS(100));
    webServer.Begin(chipIdString(), &observedExplorer);
}

void loop()
{
    webServer.Loop();
    delay(10);
}
