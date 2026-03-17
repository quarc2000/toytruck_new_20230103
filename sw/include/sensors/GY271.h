#ifndef GY271_H
#define GY271_H

#include <Arduino.h>

class GY271 {
public:
    GY271(uint8_t address = 0x0D);  // Constructor with default address
    bool begin();  // Initialize the sensor
    void readData(int16_t &x, int16_t &y, int16_t &z);  // Read magnetometer data
    bool readDataBlock(int16_t &x, int16_t &y, int16_t &z);  // Read one full XYZ sample, returns false on bus failure
    int16_t getX();  // Get X-axis data
    int16_t getY();  // Get Y-axis data
    int16_t getZ();  // Get Z-axis data

    int getCompassDirection();  // Get the compass direction in tenths of degrees
    bool readStatus(uint8_t &status);
    bool readControl1(uint8_t &value);
    bool readControl2(uint8_t &value);
    bool readSetResetPeriod(uint8_t &value);
    bool readChipId(uint8_t &value);
    bool isDataReady();


private:
    uint8_t _address;  // I2C address
    int16_t _x, _y, _z;  // Sensor data

    void updateData();  // Internal function to read and store data
    bool writeRegister(uint8_t reg, uint8_t value);
    bool readRegister(uint8_t reg, uint8_t &value);
};

#endif
