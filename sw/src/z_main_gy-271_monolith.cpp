#include <Arduino.h>
#include <task_safe_wire.h>

#define SENSOR_ADDR 0x0D  // I2C address of the QMC5883L sensor

void readSensorData(int16_t* x, int16_t* y, int16_t* z) {
    // Request the data starting from register 0x00 (the data output register)
    task_safe_wire_begin(SENSOR_ADDR);
    task_safe_wire_write(0x00);  // Register address for data output
    task_safe_wire_restart();
    
    // Request 6 bytes of data (3 values: X, Y, Z)
    if (task_safe_wire_request_from(SENSOR_ADDR, 6) == 6) {
      *x = (task_safe_wire_read() << 8) | task_safe_wire_read();  // Combine the high and low bytes for X
      *y = (task_safe_wire_read() << 8) | task_safe_wire_read();  // Combine the high and low bytes for Y
      *z = (task_safe_wire_read() << 8) | task_safe_wire_read();  // Combine the high and low bytes for Z
    } else {
      Serial.println("Error reading sensor data!");
    }
    task_safe_wire_end();
    
}

void setup() {
  Serial.begin(57600);
  
  // Initialize the QMC5883L sensor to continuous measurement mode
  task_safe_wire_begin(SENSOR_ADDR);
  task_safe_wire_write(0x09);  // Register for mode configuration
  task_safe_wire_write(0x1D);  // Continuous measurement mode
  task_safe_wire_end();
  
  delay(100);  // Wait for sensor to initialize
}

void loop() {
  int16_t x, y, z;
  readSensorData(&x, &y, &z);

  // Output the data to the Serial Monitor
  Serial.print("X: "); Serial.print(x);
  Serial.print(" Y: "); Serial.print(y);
  Serial.print(" Z: "); Serial.println(z);

  delay(1000);  // Delay for 1 second
}


