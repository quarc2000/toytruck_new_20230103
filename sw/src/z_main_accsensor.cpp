#include <sensors/accsensor.h>
#include <Arduino.h>
#include <variables/setget.h>


ACCsensor asens;

void setup(){
  Serial.begin(57600);
  globalVar_init();
  asens.Begin();
  vTaskDelay(pdMS_TO_TICKS(100));
}


void loop(){
    Serial.print("zeroAx: ");
    Serial.print(globalVar_get(zeroAx));
    Serial.print("   zeroAy: ");
    Serial.print(globalVar_get(zeroAy));
    Serial.print("   zeroAz: ");
    Serial.print(globalVar_get(zeroAz));
    Serial.print("     ms per tick: ");
    Serial.println(portTICK_PERIOD_MS);
    Serial.print("AX raw:");
    Serial.print(globalVar_get(rawAccX));
    Serial.print("   AY raw:");
    Serial.print(globalVar_get(rawAccY));
    Serial.print("   AZ raw:");
    Serial.println(globalVar_get(rawAccZ));
    Serial.print("AX cleaned:");
    Serial.print(globalVar_get(cleanedAccX));
    Serial.print("   AY cleaned:");
    Serial.print(globalVar_get(cleanedAccY));
    Serial.print("   AZ cleaned:");
    Serial.println(globalVar_get(cleanedAccZ));
    Serial.print("Temp rawData (degC*10): ");
    Serial.print(globalVar_get(rawTemp));
    Serial.print("   Temp (deg C): ");
    Serial.println(globalVar_get(rawTemp) / 10.0f);
    Serial.print("GX rawData (deg/s*10): ");
    Serial.print(globalVar_get(rawGyX));
    Serial.print("   GX cleaned (deg/s*10): ");
    Serial.print(globalVar_get(cleanedGyX));
    Serial.print("   GY rawData (deg/s*10): ");
    Serial.print(globalVar_get(rawGyY));
    Serial.print("   GY cleaned (deg/s*10): ");
    Serial.print(globalVar_get(cleanedGyY));
    Serial.print("   GZ rawData (deg/s*10): ");
    Serial.print(globalVar_get(rawGyZ));
    Serial.print("   GZ cleaned (deg/s*10): ");
    Serial.println(globalVar_get(cleanedGyZ));
    Serial.println();
    Serial.print("Heading (deg*10): ");
    Serial.print(globalVar_get(calcHeading));
    Serial.print("   Heading (deg): ");
    Serial.print(globalVar_get(calcHeading) / 10.0f);
    Serial.print("   Distance (mm, plain estimator): ");
    Serial.print(globalVar_get(calcDistance));
    Serial.print("   Speed (mm/s, plain estimator): ");
    Serial.print(globalVar_get(calcSpeed));
    Serial.println();
    Serial.println();
    vTaskDelay(pdMS_TO_TICKS(270));

}
