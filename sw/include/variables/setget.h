#ifndef GLOBAL_VAR_H
#define GLOBAL_VAR_H


// Define the number of variables
//#define NUM_VARS 4



// Shared-variable names for the task-safe setget bus.
// The detailed source-of-truth for meaning, units, IDs, and status now lives in docs/VARIABLE_MODEL.md.
typedef enum {
    zeroAx,         // Forward accelerometer zero offset in raw MPU6050 counts.
    zeroGz,         // Reserved gyro zero offset. Not actively driven today.
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
    rawTemp,        // Temperature estimate, currently stored as tenths of degrees C.
    rawGyX,         // Gyro X value in the current project-specific stored scale. See VARIABLE_MODEL.md.
    rawGyY,         // Gyro Y value in the current project-specific stored scale. See VARIABLE_MODEL.md.
    rawGyZ,         // Gyro Z yaw-rate value in the current project-specific stored scale with current biasing.
    rawLidarFront,  // Reserved front lidar distance, intended to be mm when activated.
    calcHeading,    // Current integrated heading estimate. Stored scale is still being formalized.
    calcSpeed,      // Current integrated forward speed estimate. Stored scale is still being formalized.
    calcDistance,   // Current integrated forward distance estimate. Intended to represent travel distance.
    calcXpos,       // Reserved X position in world/map frame.
    calcYpos,       // Reserved Y position in world/map frame.
    mapObservedPosePacked,      // Packed x,y,direction,speed bytes. -128 in any field means unknown.
    mapProgrammedPosePacked,    // Packed x,y,direction,speed bytes. -128 in any field means unknown.
    mapObservedCellSizeMm,      // Observed-map cell size in mm per cell.
    mapProgrammedCellSizeMm,    // Programmed-map cell size in mm per cell.
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
