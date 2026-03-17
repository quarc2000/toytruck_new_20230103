// Minimal test entry point for GY-271 (QMC5883L) service
// Only starts the GY-271 service and prints calculatedMagCourse and calculatedMagDisturbance
// Safe for bench use (no motors, steer, or lights)

#include <Arduino.h>
#include <variables/setget.h>
#include <sensors/gy271_service.h>
#include <expander.h>

// Create the global expander instance (addresses may need adjustment)
EXPANDER expander(0x70, 0x20);

void setup() {
    Serial.begin(57600);
    expander.initSwitch();
    expander.initGPIO();
    globalVar_init();
    delay(100);
    start_gy271_service();
    Serial.println("GY-271 service started.");
}

void loop() {
    delay(1000);
}
