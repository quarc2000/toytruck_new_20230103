#include <robots/driver.h>

#include <actuators/motor.h>
#include <actuators/steer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <variables/setget.h>

namespace
{
constexpr int32_t TURN_LEFT = -1;
constexpr int32_t TURN_NEUTRAL = 0;
constexpr int32_t TURN_RIGHT = 1;
// Empirical steering sign map for this truck:
// negative command steers right, positive command steers left.
constexpr int32_t STEER_SIGN_AWAY_FROM_LEFT_SIDE = -1;
constexpr int32_t STEER_SIGN_AWAY_FROM_RIGHT_SIDE = 1;

// Forward-clear fusion values.
constexpr int32_t FORWARD_CLEAR = 1;
constexpr int32_t FORWARD_BLOCKED = 0;
constexpr int32_t DIST_UNKNOWN_CM = 199;

// Nominal speed policy in normalized motor command units.
constexpr int32_t DEFAULT_FORWARD_SPEED = 100;
constexpr int32_t DEFAULT_REVERSE_SPEED = -90;
constexpr int32_t FORWARD_SLOWDOWN_CM = 30;
constexpr int32_t FORWARD_SLOW_SPEED = 90;

// Straight-driving stabilization gains.
constexpr int32_t STRAIGHT_HEADING_GAIN_DEG10 = 3;
constexpr int32_t STRAIGHT_YAW_GAIN_DPS10 = 10;
constexpr int32_t STRAIGHT_MAX_STEER = 35;

// Explicit turning authority (used for commanded and avoidance turns).
constexpr int32_t TURN_MAX_STEER = 100;
constexpr int32_t TURN_ASSIST_STEER = 20;
constexpr int32_t TURN_BIAS_STEER = 25;

// Side-wall hold behavior while driving straight.
constexpr int32_t WALL_ENTRY_CM = 30;
constexpr int32_t WALL_RELEASE_CM = 45;
constexpr int32_t WALL_TARGET_CM = 30;
constexpr int32_t WALL_DEADBAND_CM = 2;
constexpr int32_t WALL_GAIN_PER_CM = 5;
constexpr int32_t WALL_MAX_STEER = 60;
constexpr int32_t WALL_EMERGENCY_CM = 8;
constexpr int32_t SIDE_GUARD_CM = 30;
constexpr int32_t SIDE_GUARD_GAIN_PER_CM = 4;
constexpr int32_t SIDE_GUARD_MAX_STEER = 60;
constexpr int32_t NARROW_PASSAGE_WIDTH_CM = 70;
constexpr int32_t NARROW_CENTER_GAIN_PER_CM = 4;
constexpr int32_t NARROW_CENTER_MAX_STEER = 45;

// Hard safety thresholds for reverse and close-front blocking.
constexpr int32_t REVERSE_BLOCK_CM = 18;
constexpr int32_t FRONT_HARD_BLOCK_CM = 12;
constexpr int32_t FRONT_MIN_VALID_MM = 40;
constexpr int32_t FRONT_HARD_BLOCK_MM = 140;
constexpr int32_t AVOID_FLIP_DELTA_CM = 12;
constexpr int32_t AVOID_IMPOSSIBLE_SIDE_CM = 10;
constexpr int32_t AVOID_RESET_FRONT_CORNER_MM = 500;
constexpr int32_t AVOID_RESET_CLEAR_CYCLES = 2;

// Runtime timing for control loop and recovery behavior.
constexpr TickType_t LOOP_MS = 80;
constexpr TickType_t RECOVERY_REVERSE_MS = 1200;

enum DriverMode
{
    DriverModeNormal = 0,
    DriverModeRecovery = 1
};

enum DriverState
{
    DriverStateIdle = 0,
    DriverStateForward = 1,
    DriverStateReverse = 2,
    DriverStateForwardLeft = 3,
    DriverStateForwardRight = 4
};

enum WallSide
{
    WallNone = 0,
    WallLeft = 1,
    WallRight = 2
};

struct DriverControlState
{
    bool autonomous_enabled;
    bool navigation_command_mode;
    int32_t requested_turn;
    int32_t requested_distance;
    int32_t requested_speed;
};

Motor *drive = nullptr;
Steer *steer = nullptr;
bool driver_task_started = false;
SemaphoreHandle_t driver_control_mutex = nullptr;
DriverControlState driver_control = {true, false, 0, 0, 0};
int32_t requested_direction = 0;

int32_t clampSteer(int32_t value)
{
    if (value > 100)
    {
        return 100;
    }
    if (value < -100)
    {
        return -100;
    }
    return value;
}

int32_t clampSpeed(int32_t value)
{
    if (value > 100)
    {
        return 100;
    }
    if (value < -100)
    {
        return -100;
    }
    return value;
}

int32_t wrapHeadingErrorDeg10(int32_t target_deg10, int32_t current_deg10)
{
    int32_t err = target_deg10 - current_deg10;
    while (err > 1800)
    {
        err -= 3600;
    }
    while (err < -1800)
    {
        err += 3600;
    }
    return err;
}

bool ultrasonicKnown(int32_t cm)
{
    return cm > 0 && cm < DIST_UNKNOWN_CM;
}

bool lidarKnown(int32_t mm)
{
    return mm >= FRONT_MIN_VALID_MM && mm < 2999;
}

bool frontHardBlocked(int32_t front_cm, int32_t lidar_left_mm, int32_t lidar_right_mm)
{
    if (ultrasonicKnown(front_cm) && front_cm <= FRONT_HARD_BLOCK_CM)
    {
        return true;
    }
    if (lidarKnown(lidar_left_mm) && lidar_left_mm <= FRONT_HARD_BLOCK_MM)
    {
        return true;
    }
    if (lidarKnown(lidar_right_mm) && lidar_right_mm <= FRONT_HARD_BLOCK_MM)
    {
        return true;
    }
    return false;
}

int32_t navTurnToBias(int32_t turn_command)
{
    if (turn_command > 0)
    {
        return TURN_RIGHT;
    }
    if (turn_command < 0)
    {
        return TURN_LEFT;
    }
    return TURN_NEUTRAL;
}

int32_t biasToForwardSteer(int32_t bias, int32_t magnitude)
{
    if (bias == TURN_RIGHT)
    {
        return magnitude;
    }
    if (bias == TURN_LEFT)
    {
        return -magnitude;
    }
    return 0;
}

int32_t biasToReverseSteer(int32_t bias, int32_t magnitude)
{
    // Same yaw intent must flip wheel sign in reverse.
    return -biasToForwardSteer(bias, magnitude);
}

int32_t biasFromSteerSign(int32_t steer_sign)
{
    return (steer_sign >= 0) ? TURN_RIGHT : TURN_LEFT;
}

int32_t steerAwayFromLeftSide(int32_t magnitude)
{
    return STEER_SIGN_AWAY_FROM_LEFT_SIDE * magnitude;
}

int32_t steerAwayFromRightSide(int32_t magnitude)
{
    return STEER_SIGN_AWAY_FROM_RIGHT_SIDE * magnitude;
}

int32_t chooseAvoidanceDirection(int32_t left_cm, int32_t right_cm, int32_t previous)
{
    const bool left_known = ultrasonicKnown(left_cm);
    const bool right_known = ultrasonicKnown(right_cm);

    // Select avoidance direction purely from side clearances.
    if (left_known && left_cm <= AVOID_IMPOSSIBLE_SIDE_CM)
    {
        return biasFromSteerSign(steerAwayFromLeftSide(1));
    }
    if (right_known && right_cm <= AVOID_IMPOSSIBLE_SIDE_CM)
    {
        return biasFromSteerSign(steerAwayFromRightSide(1));
    }

    // Unknown/out-of-range side often means open space in this setup.
    // Prefer the known open side only when it is decisively better.
    if (!left_known && right_known)
    {
        return biasFromSteerSign(steerAwayFromRightSide(1));
    }
    if (!right_known && left_known)
    {
        return biasFromSteerSign(steerAwayFromLeftSide(1));
    }

    if (left_known && right_known)
    {
        const int32_t delta = right_cm - left_cm;
        if (delta >= AVOID_FLIP_DELTA_CM)
        {
            return biasFromSteerSign(steerAwayFromLeftSide(1));
        }
        if (delta <= -AVOID_FLIP_DELTA_CM)
        {
            return biasFromSteerSign(steerAwayFromRightSide(1));
        }
    }

    return previous;
}

int32_t maybeFlipAvoidanceDirection(int32_t current_direction, int32_t left_cm, int32_t right_cm)
{
    const bool left_known = ultrasonicKnown(left_cm);
    const bool right_known = ultrasonicKnown(right_cm);
    const int32_t avoid_left_side_direction = biasFromSteerSign(steerAwayFromLeftSide(1));
    const int32_t avoid_right_side_direction = biasFromSteerSign(steerAwayFromRightSide(1));

    // Keep committed direction unless that path becomes impossible.
    if (current_direction == avoid_left_side_direction)
    {
        // Avoid-left means steering away from left side toward right path.
        // Flip only if right path becomes impossible.
        if (right_known && right_cm <= AVOID_IMPOSSIBLE_SIDE_CM)
        {
            return avoid_right_side_direction;
        }
        return avoid_left_side_direction;
    }

    if (current_direction == avoid_right_side_direction)
    {
        // Avoid-right means steering away from right side toward left path.
        // Flip only if left path becomes impossible.
        if (left_known && left_cm <= AVOID_IMPOSSIBLE_SIDE_CM)
        {
            return avoid_left_side_direction;
        }
        return avoid_right_side_direction;
    }

    return chooseAvoidanceDirection(left_cm, right_cm, avoid_right_side_direction);
}

int32_t narrowPassageCenterSteer(int32_t left_cm, int32_t right_cm)
{
    if (!ultrasonicKnown(left_cm) || !ultrasonicKnown(right_cm))
    {
        return 0;
    }
    if ((left_cm + right_cm) > NARROW_PASSAGE_WIDTH_CM)
    {
        return 0;
    }
    const int32_t delta = right_cm - left_cm;
    return clampSteer(max(-NARROW_CENTER_MAX_STEER, min(NARROW_CENTER_MAX_STEER, delta * NARROW_CENTER_GAIN_PER_CM)));
}

int32_t turningCornerLidarMmForDirection(int32_t direction, int32_t lidar_left_mm, int32_t lidar_right_mm)
{
    if (direction == TURN_LEFT) return lidar_left_mm;
    if (direction == TURN_RIGHT) return lidar_right_mm;
    return 0;
}

int32_t chooseForwardSpeed(int32_t requested_speed, int32_t front_cm)
{
    if (requested_speed != 0)
    {
        return clampSpeed(requested_speed);
    }

    if (ultrasonicKnown(front_cm) && front_cm <= FORWARD_SLOWDOWN_CM)
    {
        return FORWARD_SLOW_SPEED;
    }

    return DEFAULT_FORWARD_SPEED;
}

int32_t computeStraightSteer(int32_t heading_target_deg10, int32_t heading_deg10, int32_t yaw_degps10)
{
    const int32_t heading_term = wrapHeadingErrorDeg10(heading_target_deg10, heading_deg10) / STRAIGHT_HEADING_GAIN_DEG10;
    const int32_t yaw_term = -yaw_degps10 / STRAIGHT_YAW_GAIN_DPS10;
    return max(-STRAIGHT_MAX_STEER, min(STRAIGHT_MAX_STEER, heading_term + yaw_term));
}

void maybeCaptureWall(int32_t left_cm, int32_t right_cm, WallSide &side, int32_t &target_cm)
{
    if (side != WallNone)
    {
        return;
    }

    const bool left_near = ultrasonicKnown(left_cm) && left_cm <= WALL_ENTRY_CM;
    const bool right_near = ultrasonicKnown(right_cm) && right_cm <= WALL_ENTRY_CM;

    if (left_near && (!right_near || left_cm <= right_cm))
    {
        side = WallLeft;
        target_cm = WALL_TARGET_CM;
        return;
    }

    if (right_near)
    {
        side = WallRight;
        target_cm = WALL_TARGET_CM;
    }
}

void maybeReleaseWall(int32_t left_cm, int32_t right_cm, WallSide &side, int32_t &target_cm)
{
    if (side == WallLeft)
    {
        if (!ultrasonicKnown(left_cm) || left_cm > WALL_RELEASE_CM)
        {
            side = WallNone;
            target_cm = 0;
        }
    }
    else if (side == WallRight)
    {
        if (!ultrasonicKnown(right_cm) || right_cm > WALL_RELEASE_CM)
        {
            side = WallNone;
            target_cm = 0;
        }
    }
}

int32_t wallFollowSteer(int32_t left_cm, int32_t right_cm, WallSide side, int32_t target_cm)
{
    if (side == WallLeft && ultrasonicKnown(left_cm))
    {
        const int32_t error_cm = left_cm - target_cm;
        if (abs(error_cm) <= WALL_DEADBAND_CM)
        {
            return 0;
        }
        if (error_cm > 0)
        {
            // Too far from left wall: steer toward left wall.
            return clampSteer(-steerAwayFromLeftSide(min(WALL_MAX_STEER, error_cm * WALL_GAIN_PER_CM)));
        }
        // Too close to left wall: steer away from left wall.
        return clampSteer(steerAwayFromLeftSide(min(WALL_MAX_STEER, (-error_cm) * WALL_GAIN_PER_CM)));
    }

    if (side == WallRight && ultrasonicKnown(right_cm))
    {
        const int32_t error_cm = right_cm - target_cm;
        if (abs(error_cm) <= WALL_DEADBAND_CM)
        {
            return 0;
        }
        if (error_cm > 0)
        {
            // Too far from right wall: steer toward right wall.
            return clampSteer(-steerAwayFromRightSide(min(WALL_MAX_STEER, error_cm * WALL_GAIN_PER_CM)));
        }
        // Too close to right wall: steer away from right wall.
        return clampSteer(steerAwayFromRightSide(min(WALL_MAX_STEER, (-error_cm) * WALL_GAIN_PER_CM)));
    }

    return 0;
}

int32_t sideEmergencySteer(int32_t left_cm, int32_t right_cm)
{
    if (ultrasonicKnown(left_cm) && left_cm <= WALL_EMERGENCY_CM)
    {
        return steerAwayFromLeftSide(TURN_MAX_STEER);
    }
    if (ultrasonicKnown(right_cm) && right_cm <= WALL_EMERGENCY_CM)
    {
        return steerAwayFromRightSide(TURN_MAX_STEER);
    }
    return 0;
}

int32_t sideGuardSteer(int32_t left_cm, int32_t right_cm)
{
    int32_t steer = 0;

    if (ultrasonicKnown(left_cm) && left_cm < SIDE_GUARD_CM)
    {
        const int32_t deficit_cm = SIDE_GUARD_CM - left_cm;
        steer += steerAwayFromLeftSide(min(SIDE_GUARD_MAX_STEER, deficit_cm * SIDE_GUARD_GAIN_PER_CM));
    }
    if (ultrasonicKnown(right_cm) && right_cm < SIDE_GUARD_CM)
    {
        const int32_t deficit_cm = SIDE_GUARD_CM - right_cm;
        steer += steerAwayFromRightSide(min(SIDE_GUARD_MAX_STEER, deficit_cm * SIDE_GUARD_GAIN_PER_CM));
    }

    return clampSteer(steer);
}

DriverControlState readDriverControl()
{
    if (driver_control_mutex == nullptr)
    {
        return driver_control;
    }

    xSemaphoreTake(driver_control_mutex, portMAX_DELAY);
    DriverControlState snapshot = driver_control;
    xSemaphoreGive(driver_control_mutex);
    return snapshot;
}

void publishDriverCommand(DriverMode mode, DriverState state, int32_t speed, int32_t steer_command)
{
    // Keep one compact activity code for diagnostics: 10*mode + state.
    globalVar_set(driver_driverActivity, mode * 10 + state);
    globalVar_set(driver_desired_speed, speed);
    globalVar_set(driver_desired_turn, steer_command);
}

} // namespace

Driver::Driver()
{
}

void Driver::Begin()
{
    if (driver_task_started)
    {
        return;
    }

    if (driver_control_mutex == nullptr)
    {
        driver_control_mutex = xSemaphoreCreateMutex();
    }

    if (drive == nullptr)
    {
        drive = new Motor();
    }
    drive->Begin();

    if (steer == nullptr)
    {
        steer = new Steer();
    }
    steer->Begin();

    publishDriverCommand(DriverModeNormal, DriverStateIdle, 0, 0);
    xTaskCreate(driverTask, "driverTask", 3000, NULL, 1, NULL);
    driver_task_started = true;
}

void Driver::drive_absolute(int direction, int distance, int speed)
{
    requested_direction = direction;
    if (driver_control_mutex != nullptr)
    {
        xSemaphoreTake(driver_control_mutex, portMAX_DELAY);
        driver_control.navigation_command_mode = true;
        driver_control.requested_turn = direction;
        driver_control.requested_distance = distance;
        driver_control.requested_speed = clampSpeed(speed);
        xSemaphoreGive(driver_control_mutex);
    }

    globalVar_set(driver_desired_direction, direction);
    globalVar_set(driver_desired_distance, distance);
    globalVar_set(driver_desired_speed, clampSpeed(speed));
}

void Driver::drive_relative(int turn, int distance, int speed)
{
    if (driver_control_mutex != nullptr)
    {
        xSemaphoreTake(driver_control_mutex, portMAX_DELAY);
        driver_control.navigation_command_mode = true;
        driver_control.requested_turn = turn;
        driver_control.requested_distance = distance;
        driver_control.requested_speed = clampSpeed(speed);
        xSemaphoreGive(driver_control_mutex);
    }

    globalVar_set(driver_desired_turn, turn);
    globalVar_set(driver_desired_distance, distance);
    globalVar_set(driver_desired_speed, clampSpeed(speed));
}

void Driver::stop()
{
    if (driver_control_mutex != nullptr)
    {
        xSemaphoreTake(driver_control_mutex, portMAX_DELAY);
        driver_control.navigation_command_mode = true;
        driver_control.requested_turn = 0;
        driver_control.requested_distance = 0;
        driver_control.requested_speed = 0;
        xSemaphoreGive(driver_control_mutex);
    }

    globalVar_set(driver_desired_turn, 0);
    globalVar_set(driver_desired_distance, 0);
    globalVar_set(driver_desired_speed, 0);
}

void Driver::set_autonomous_enabled(bool enabled)
{
    if (driver_control_mutex != nullptr)
    {
        xSemaphoreTake(driver_control_mutex, portMAX_DELAY);
        driver_control.autonomous_enabled = enabled;
        if (!enabled)
        {
            driver_control.requested_speed = 0;
        }
        xSemaphoreGive(driver_control_mutex);
    }
}

void Driver::driverTask(void *pvParameters)
{
    (void)pvParameters;

    DriverMode mode = DriverModeNormal;
    DriverState state = DriverStateIdle;
    DriverState previous_state = DriverStateIdle;
    int32_t avoidance_direction = TURN_RIGHT;
    bool avoidance_committed = false;
    TickType_t recovery_started_ms = 0;
    bool forward_turn_boost_active = false;
    int32_t avoid_reset_clear_cycles = 0;
    WallSide wall_side = WallNone;
    int32_t wall_target_cm = 0;
    int32_t forward_heading_target_deg10 = globalVar_get(calcHeading);

    for (;;)
    {
        const TickType_t now_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
        const DriverControlState control = readDriverControl();
        const int32_t front_cm = globalVar_get(rawDistFront);
        const int32_t left_cm = globalVar_get(rawDistLeft);
        const int32_t right_cm = globalVar_get(rawDistRight);
        const int32_t rear_cm = globalVar_get(rawDistBack);
        const int32_t lidar_left_mm = globalVar_get(rawLidarFrontLeft);
        const int32_t lidar_right_mm = globalVar_get(rawLidarFrontRight);
        const int32_t heading_deg10 = globalVar_get(calcHeading);
        const int32_t yaw_degps10 = globalVar_get(cleanedGyZ);
        const int32_t fused_turn_bias = globalVar_get(fuseTurnBias);
        const int32_t forward_clear = globalVar_get(fuseForwardClear);

        const bool rear_blocked = ultrasonicKnown(rear_cm) && rear_cm < REVERSE_BLOCK_CM;
        const bool hard_front_blocked = frontHardBlocked(front_cm, lidar_left_mm, lidar_right_mm);
        const bool front_blocked = (forward_clear == FORWARD_BLOCKED) || hard_front_blocked || (forward_clear != FORWARD_CLEAR);

        if (!control.autonomous_enabled)
        {
            mode = DriverModeNormal;
            state = DriverStateIdle;
            previous_state = DriverStateIdle;
            drive->driving(0);
            steer->direction(0);
            publishDriverCommand(mode, state, 0, 0);
            vTaskDelay(pdMS_TO_TICKS(LOOP_MS));
            continue;
        }

        const int32_t requested_speed = control.navigation_command_mode ? clampSpeed(control.requested_speed) : 0;
        const int32_t raw_turn_command = control.navigation_command_mode ? control.requested_turn : fused_turn_bias;
        int32_t requested_bias = navTurnToBias(raw_turn_command);
        const bool explicit_sharp_turn = control.navigation_command_mode && (abs(raw_turn_command) > 1);

        if (mode == DriverModeNormal)
        {
            if (requested_speed == 0 && control.navigation_command_mode)
            {
                state = DriverStateIdle;
            }
            else if (requested_speed < 0)
            {
                state = DriverStateReverse;
            }
            else if (explicit_sharp_turn && requested_bias == TURN_LEFT)
            {
                state = DriverStateForwardLeft;
            }
            else if (explicit_sharp_turn && requested_bias == TURN_RIGHT)
            {
                state = DriverStateForwardRight;
            }
            else
            {
                state = DriverStateForward;
            }

            if (state == DriverStateForward || state == DriverStateForwardLeft || state == DriverStateForwardRight)
            {
                if (front_blocked)
                {
                    mode = DriverModeRecovery;
                    state = DriverStateReverse;
                    if (!avoidance_committed)
                    {
                        avoidance_direction = chooseAvoidanceDirection(left_cm, right_cm, avoidance_direction);
                        avoidance_committed = true;
                    }
                    else
                    {
                        avoidance_direction = maybeFlipAvoidanceDirection(avoidance_direction, left_cm, right_cm);
                    }
                    recovery_started_ms = now_ms;
                    wall_side = WallNone;
                    wall_target_cm = 0;
                }
                else if (forward_turn_boost_active)
                {
                    state = (avoidance_direction == TURN_LEFT) ? DriverStateForwardLeft : DriverStateForwardRight;
                    requested_bias = avoidance_direction;
                }
            }
        }
        else
        {
            state = DriverStateReverse;
            if (rear_blocked || (now_ms - recovery_started_ms) >= RECOVERY_REVERSE_MS)
            {
                mode = DriverModeNormal;
                forward_turn_boost_active = true;
                avoid_reset_clear_cycles = 0;
                state = (avoidance_direction == TURN_LEFT) ? DriverStateForwardLeft : DriverStateForwardRight;
                requested_bias = avoidance_direction;
                forward_heading_target_deg10 = heading_deg10;
            }
        }

        if (state != previous_state)
        {
            if (state == DriverStateForward)
            {
                forward_heading_target_deg10 = heading_deg10;
                wall_side = WallNone;
                wall_target_cm = 0;
            }
            previous_state = state;
        }

        int32_t output_speed = 0;
        int32_t output_steer = 0;

        switch (state)
        {
        case DriverStateIdle:
            output_speed = 0;
            output_steer = 0;
            break;

        case DriverStateForward:
        {
            output_speed = chooseForwardSpeed(requested_speed, front_cm);
            const int32_t straight = computeStraightSteer(forward_heading_target_deg10, heading_deg10, yaw_degps10);
            maybeCaptureWall(left_cm, right_cm, wall_side, wall_target_cm);
            maybeReleaseWall(left_cm, right_cm, wall_side, wall_target_cm);
            const int32_t wall = max(-WALL_MAX_STEER, min(WALL_MAX_STEER, wallFollowSteer(left_cm, right_cm, wall_side, wall_target_cm)));
            const int32_t side_guard = sideGuardSteer(left_cm, right_cm);
            const int32_t narrow_center = narrowPassageCenterSteer(left_cm, right_cm);
            const int32_t turn_bias_component =
                (control.navigation_command_mode && requested_bias != TURN_NEUTRAL) ?
                biasToForwardSteer(requested_bias, TURN_BIAS_STEER) :
                0;
            const int32_t emergency = sideEmergencySteer(left_cm, right_cm);
            if (wall_side != WallNone)
            {
                // Wall-follow state owns steering while active.
                output_steer = clampSteer(wall + side_guard + narrow_center);
            }
            else
            {
                output_steer = clampSteer(straight + side_guard + narrow_center + turn_bias_component);
            }
            if (emergency != 0)
            {
                output_steer = emergency;
            }
            break;
        }

        case DriverStateForwardLeft:
            output_speed = chooseForwardSpeed(requested_speed, front_cm);
            output_steer = clampSteer(-TURN_MAX_STEER + computeStraightSteer(forward_heading_target_deg10, heading_deg10, yaw_degps10) / TURN_ASSIST_STEER);
            break;

        case DriverStateForwardRight:
            output_speed = chooseForwardSpeed(requested_speed, front_cm);
            output_steer = clampSteer(TURN_MAX_STEER + computeStraightSteer(forward_heading_target_deg10, heading_deg10, yaw_degps10) / TURN_ASSIST_STEER);
            break;

        case DriverStateReverse:
            if (mode == DriverModeRecovery)
            {
                output_speed = DEFAULT_REVERSE_SPEED;
                output_steer = biasToReverseSteer(avoidance_direction, TURN_MAX_STEER);
            }
            else
            {
                output_speed = requested_speed < 0 ? requested_speed : DEFAULT_REVERSE_SPEED;
                const int32_t reverse_bias = (requested_bias != TURN_NEUTRAL) ? requested_bias : avoidance_direction;
                output_steer = biasToReverseSteer(reverse_bias, TURN_MAX_STEER);
            }

            if (rear_blocked)
            {
                output_speed = 0;
                output_steer = 0;
            }
            break;
        }

        output_speed = clampSpeed(output_speed);
        output_steer = clampSteer(output_steer);

        if (mode == DriverModeNormal &&
            (state == DriverStateForward || state == DriverStateForwardLeft || state == DriverStateForwardRight))
        {
            const int32_t turning_corner_lidar_mm =
                turningCornerLidarMmForDirection(avoidance_direction, lidar_left_mm, lidar_right_mm);
            const bool turning_corner_open =
                lidarKnown(turning_corner_lidar_mm) && turning_corner_lidar_mm >= AVOID_RESET_FRONT_CORNER_MM;
            if (forward_turn_boost_active && turning_corner_open)
            {
                avoid_reset_clear_cycles++;
                if (avoid_reset_clear_cycles >= AVOID_RESET_CLEAR_CYCLES)
                {
                    forward_turn_boost_active = false;
                    avoidance_committed = false;
                    avoid_reset_clear_cycles = 0;
                }
            }
            else if (forward_turn_boost_active)
            {
                avoid_reset_clear_cycles = 0;
            }
        }

        drive->driving(output_speed);
        steer->direction(output_steer);
        publishDriverCommand(mode, state, output_speed, output_steer);

        vTaskDelay(pdMS_TO_TICKS(LOOP_MS));
    }
}
