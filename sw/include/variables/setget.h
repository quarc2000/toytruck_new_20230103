#ifndef GLOBAL_VAR_H
#define GLOBAL_VAR_H


// Define the number of variables
//#define NUM_VARS 4



// Shared-variable names for the task-safe setget bus.
// The detailed source-of-truth for meaning, units, IDs, and status now lives in docs/VARIABLE_MODEL.md.
typedef enum {
    zeroAx,         // Forward accelerometer zero offset in raw MPU6050 counts.
    zeroAy,         // Lateral accelerometer zero offset in raw MPU6050 counts.
    zeroAz,         // Vertical accelerometer zero offset in raw MPU6050 counts.
    zeroGz,         // Gyro Z zero offset in raw MPU6050 counts from startup averaging.
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
    rawGyZ,         // Stored raw-data form of gyro Z, encoded as deg/s * 10 after zero-offset correction.
    rawLidarFront,  // Reserved front lidar distance, intended to be mm when activated.
    calcHeading,    // Current integrated heading estimate in deg * 10.
    calcSpeed,      // Current integrated forward speed estimate in mm/s.
    calcDistance,   // Current integrated forward distance estimate in mm.
    calcXpos,       // Reserved X position in world/map frame.
    calcYpos,       // Reserved Y position in world/map frame.
    mapObservedPosePacked,      // Packed x,y,direction,speed bytes. -128 in any field means unknown.
    mapProgrammedPosePacked,    // Packed x,y,direction,speed bytes. -128 in any field means unknown.
    mapObservedCellSizeMm,      // Observed-map cell size in mm per cell.
    mapProgrammedCellSizeMm,    // Programmed-map cell size in mm per cell.
    fuseForwardClear,           // Forward-clearance fusion state: -1 unknown, 0 blocked, 1 clear.
    steerDirection,             // Steering command in the normalized range -100 to +100.
    //>>> Robot - DRIVER
    driver_driverActivity,      // Reserved driver-mode/activity code for the future Driver layer.
    driver_desired_direction,   // Reserved desired absolute direction for the future Driver layer.
    driver_desired_turn,        // Reserved desired relative turn for the future Driver layer.
    driver_desired_speed,       // Reserved desired speed for the future Driver layer.
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
