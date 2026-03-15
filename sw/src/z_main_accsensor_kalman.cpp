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
    Serial.print("     ms per tick: ");
    Serial.println(portTICK_PERIOD_MS);
    Serial.print("AX:");
    Serial.print(globalVar_get(rawAccX));
    Serial.print("   AY:");
    Serial.print(globalVar_get(rawAccY));
    Serial.print("   AZ:");
    Serial.println(globalVar_get(rawAccZ));
    Serial.print("Temp (stored degC10): ");
    Serial.print(globalVar_get(rawTemp));
    Serial.print("   Temp (deg C): ");
    Serial.println(globalVar_get(rawTemp) / 10.0f);
    Serial.print("GX (deg/s*10): ");
    Serial.print(globalVar_get(rawGyX));
    Serial.print("   GX (deg/s): ");
    Serial.print(globalVar_get(rawGyX) / 10.0f);
    Serial.print("   GY (deg/s*10): ");
    Serial.print(globalVar_get(rawGyY));
    Serial.print("   GY (deg/s): ");
    Serial.print(globalVar_get(rawGyY) / 10.0f);
    Serial.print("   GZ (deg/s*10): ");
    Serial.print(globalVar_get(rawGyZ));
    Serial.print("   GZ (deg/s): ");
    Serial.println(globalVar_get(rawGyZ) / 10.0f);
    Serial.println();
    Serial.print("Heading (deg*10): ");
    Serial.print(globalVar_get(calcHeading));
    Serial.print("   Heading (deg): ");
    Serial.print(globalVar_get(calcHeading) / 10.0f);
    Serial.print("   Distance (mm): ");
    Serial.print(globalVar_get(calcDistance));
    Serial.print("   Speed (mm/s): ");
    Serial.print(globalVar_get(calcSpeed));
    Serial.println();
    Serial.println();
    vTaskDelay(pdMS_TO_TICKS(270));

}
