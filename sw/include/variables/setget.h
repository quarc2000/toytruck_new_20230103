#ifndef GLOBAL_VAR_H
#define GLOBAL_VAR_H


// Define the number of variables
//#define NUM_VARS 4



// Shared-variable names for the task-safe setget bus.
// The detailed source-of-truth for meaning, units, semantic tiers, documentation IDs, and status
// now lives in docs/VARIABLE_MODEL.md. Current code symbols keep the historical raw/calc/fuse
// naming even though the documentation taxonomy now distinguishes raw, cleaned, calculated, and fused.
typedef enum {
    zeroAx,         // Forward accelerometer zero offset in raw MPU6050 counts.
    zeroAy,         // Lateral accelerometer zero offset in raw MPU6050 counts.
    zeroAz,         // Vertical accelerometer zero offset in raw MPU6050 counts.
    zeroGz,         // Gyro Z zero offset in raw MPU6050 counts from startup averaging.
    configExpanderPresent, // Boolean: 1 if the I2C mux/expander path is present, else 0.
    configGy271Present,    // Boolean: 1 if the GY-271 magnetometer path is present, else 0.
    configFrontLidarPresent, // Boolean: 1 if the front VL53L0X path is present, else 0.
    rawDistLeft,    // Left ultrasonic distance in cm. 199 is also timeout/out-of-range.
    rawDistFront,   // Front ultrasonic distance in cm. 199 is also timeout/out-of-range.
    rawDistRight,   // Right ultrasonic distance in cm. 199 is also timeout/out-of-range.
    rawDistBack,    // Rear ultrasonic distance in cm. 199 is also timeout/out-of-range.
    rawMagX,        // Reserved magnetometer X value. Active unit depends on future magnetometer path.
    rawMagY,        // Reserved magnetometer Y value. Active unit depends on future magnetometer path.
    rawMagZ,        // Reserved magnetometer Z value. Active unit depends on future magnetometer path.
    rawAccX,        // Forward accelerometer value in raw MPU6050 counts after zero-offset correction.
    rawAccY,        // Lateral accelerometer value in raw MPU6050 counts.
    rawAccZ,        // Vertical accelerometer value in raw MPU6050 counts.
    rawTemp,        // Stored raw-data form of temperature, currently encoded as degC * 10.
    rawGyX,         // Stored raw-data form of gyro X, encoded as deg/s * 10.
    rawGyY,         // Stored raw-data form of gyro Y, encoded as deg/s * 10.
    rawGyZ,         // Stored raw-data form of gyro Z, encoded as deg/s * 10 before cleaning.
    rawLidarFront,  // Front lidar minimum of the active left/right front VL53L0X readings in mm, or 0 if unknown.
    rawLidarFrontRight, // Front-right VL53L0X distance in mm.
    rawLidarFrontLeft,  // Front-left VL53L0X distance in mm.
    cleanedAccX,    // Filtered and centered forward accelerometer value in MPU6050 counts.
    cleanedAccY,    // Filtered and centered lateral accelerometer value in MPU6050 counts.
    cleanedAccZ,    // Filtered and centered vertical accelerometer value in MPU6050 counts.
    cleanedGyX,     // Filtered gyro X value in deg/s * 10.
    cleanedGyY,     // Filtered gyro Y value in deg/s * 10.
    cleanedGyZ,     // Filtered and bias-corrected gyro Z value in deg/s * 10.
    calcHeading,    // Current integrated heading estimate in deg * 10.
    calcSpeed,      // Current integrated forward speed estimate in mm/s.
    calcDistance,   // Current integrated forward distance estimate in mm.
    calcXpos,       // Reserved X position in world/map frame.
    calcYpos,       // Reserved Y position in world/map frame.
    calculatedMagCourse, // Calculated magnetic course (tenths of degrees, 0=north, 900=east, etc.)
    calculatedMagDisturbance, // Boolean: 1 if magnetic disturbance detected, 0 otherwise
    mapObservedPosePacked,      // Packed x,y,direction,speed bytes. -128 in any field means unknown.
    mapProgrammedPosePacked,    // Packed x,y,direction,speed bytes. -128 in any field means unknown.
    mapObservedCellSizeMm,      // Observed-map cell size in mm per cell.
    mapProgrammedCellSizeMm,    // Programmed-map cell size in mm per cell.
    fuseForwardClear,           // Forward-clearance fusion state: -1 unknown, 0 blocked, 1 clear.
    fuseTurnBias,               // Preferred front turn bias: -1 favor left, 0 neutral/unknown, 1 favor right.
    fuseHeadingDeg10,           // Absolute fused heading in deg * 10 where 0 = north and 900 = east.
    steerDirection,             // Steering command in the normalized range -100 to +100.
    //>>> Robot - DRIVER
    driver_driverActivity,      // Reserved driver-mode/activity code for the future Driver layer.
    driver_desired_direction,   // Reserved desired absolute direction for the future Driver layer.
    driver_desired_turn,        // Reserved desired relative turn for the future Driver layer.
    driver_desired_speed,       // Current desired longitudinal speed command, normalized today and later promotable to Driver-layer ownership.
    driver_desired_distance,    // Reserved desired distance for the future Driver layer.
    //<<< Robot - DRIVER
    NUM_VARS
} VarNames;

// Function prototypes
void globalVar_init(void);
void globalVar_set(VarNames varName, long value);
long globalVar_get(VarNames varName);
long globalVar_get_total(VarNames varName);
void globalVar_reset_total(VarNames varName);
long globalVar_get(VarNames varName, long *age);
long globalVar_get_delta(VarNames varName);
long globalVar_get_TOT_delta(VarNames varName);

#endif // GLOBAL_VAR_H
