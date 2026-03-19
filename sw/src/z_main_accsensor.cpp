#include <sensors/accsensor.h>
#include <Arduino.h>
#include <variables/setget.h>

namespace
{
  int32_t wrapHeadingDeg10(int32_t heading)
  {
    while (heading >= 3600) heading -= 3600;
    while (heading < 0) heading += 3600;
    return heading;
  }
}

ACCsensor asens;

void setup(){
  Serial.begin(57600);
  globalVar_init();
  asens.Begin();
  vTaskDelay(pdMS_TO_TICKS(100));
}


void loop(){
    static uint32_t lastPrintMs = millis();
    static int32_t previousDistanceMm = 0;
    static int32_t diagnosticXmm = 0;
    static int32_t diagnosticYmm = 0;

    const uint32_t nowMs = millis();
    const int32_t dtMs = static_cast<int32_t>(nowMs - lastPrintMs);
    lastPrintMs = nowMs;
    const int32_t headingDeg10 = wrapHeadingDeg10(globalVar_get(calcHeading));
    const int32_t distanceMm = globalVar_get(calcDistance);
    const int32_t deltaDistanceMm = distanceMm - previousDistanceMm;
    previousDistanceMm = distanceMm;
    const float headingRad = (headingDeg10 / 10.0f) * 3.14159265f / 180.0f;
    diagnosticXmm += lroundf(sinf(headingRad) * deltaDistanceMm);
    diagnosticYmm += lroundf(cosf(headingRad) * deltaDistanceMm);

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
    Serial.print(headingDeg10);
    Serial.print("   Heading (deg): ");
    Serial.print(headingDeg10 / 10.0f);
    Serial.print("   Distance (mm, plain estimator): ");
    Serial.print(distanceMm);
    Serial.print("   Speed (mm/s, plain estimator): ");
    Serial.print(globalVar_get(calcSpeed));
    Serial.print("   dDist(mm): ");
    Serial.print(deltaDistanceMm);
    Serial.print("   dt(ms): ");
    Serial.print(dtMs);
    Serial.println();
    Serial.print("Diag X east (mm): ");
    Serial.print(diagnosticXmm);
    Serial.print("   Diag Y north (mm): ");
    Serial.print(diagnosticYmm);
    Serial.println();
    Serial.println();
    vTaskDelay(pdMS_TO_TICKS(270));

}
