#include <robots/driver.h>
#include <actuators/motor.h>
#include <actuators/steer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <variables/setget.h>

namespace
{
enum DriverRuntimeState
{
    DRIVER_IDLE = 0,
    DRIVER_FORWARD = 1,
    DRIVER_RECOVER_STOP = 2,
    DRIVER_RECOVER_REVERSE = 3,
    DRIVER_RECOVER_PAUSE = 4
};

static constexpr int32_t FUSE_FORWARD_CLEAR = 1;
static constexpr int32_t FUSE_FORWARD_BLOCKED = 0;
static constexpr int32_t FORWARD_DISTANCE_UNKNOWN_CM = 199;
static constexpr int32_t TURN_LEFT = -1;
static constexpr int32_t TURN_NEUTRAL = 0;
static constexpr int32_t TURN_RIGHT = 1;
static constexpr int32_t FORWARD_SPEED = 100;
static constexpr int32_t FORWARD_SLOW_SPEED = 90;
static constexpr int32_t FORWARD_SLOWDOWN_CM = 30;
static constexpr int32_t REVERSE_SPEED = -100;
static constexpr int32_t MOTOR_KICK_SPEED = 100;
static constexpr int32_t FORWARD_STEER_BIAS = 30;
static constexpr int32_t REVERSE_STEER_BIAS = 65;
static constexpr int32_t HEADING_HOLD_MAX_STEER = 35;
static constexpr int32_t HEADING_HOLD_DEG10_PER_STEER = 6;
static constexpr int32_t ADAPTIVE_TRIM_DIVISOR = 8;
static constexpr int32_t SIDE_CENTER_MAX_STEER = 20;
static constexpr int32_t SIDE_CENTER_CM_PER_STEER = 2;
static constexpr int32_t SIDE_CENTER_DEADBAND_CM = 8;
static constexpr int32_t REVERSE_BLOCK_CM = 20;
static constexpr int32_t BLOCK_CONFIRM_CYCLES = 3;
static constexpr int32_t CLEAR_CONFIRM_CYCLES = 4;
static constexpr int32_t SIDE_CLEAR_DELTA_CM = 12;
static constexpr TickType_t TURN_COMMIT_HOLD_MS = 8000;
static constexpr TickType_t FORWARD_REESTABLISHED_MS = 5000;
static constexpr TickType_t MIN_FORWARD_PULSE_MS = 500;
static constexpr TickType_t MIN_REVERSE_PULSE_MS = 700;
static constexpr TickType_t DRIVER_PERIOD_MS = 100;
static constexpr TickType_t RECOVER_STOP_MS = 200;
static constexpr TickType_t RECOVER_REVERSE_MS = 1500;
static constexpr TickType_t RECOVER_PAUSE_MS = 400;
static constexpr TickType_t MOTOR_KICK_MS = 100;
static constexpr TickType_t MOTOR_REKICK_MS = 200;
static constexpr int32_t MOTION_ACCEL_THRESHOLD_COUNTS = 40;
static constexpr int32_t TRIM_LEARN_HEADING_DEADBAND_DEG10 = 40;

Motor *drive = nullptr;
Steer *steer = nullptr;
bool driver_task_started = false;
int32_t requested_direction = 0;
int32_t requested_turn = 0;
int32_t requested_distance = 0;
int32_t requested_speed = 0;

int commandSteerFromBias(int32_t turn_bias, int magnitude)
{
    if (turn_bias > 0)
    {
        return magnitude;
    }

    if (turn_bias < 0)
    {
        return -magnitude;
    }

    return 0;
}

void publishDriverCommand(DriverRuntimeState activity, int speed, int steer_command)
{
    globalVar_set(driver_driverActivity, activity);
    globalVar_set(driver_desired_speed, speed);
    globalVar_set(driver_desired_turn, steer_command);
}

bool ultrasonicSideKnown(int32_t distance_cm)
{
    return distance_cm > 0 && distance_cm < 199;
}

int chooseRecoveryBiasFromSides(int32_t left_cm, int32_t right_cm, int32_t previous_bias)
{
    const bool left_known = ultrasonicSideKnown(left_cm);
    const bool right_known = ultrasonicSideKnown(right_cm);

    if (left_known && right_known)
    {
        const int32_t side_delta_cm = right_cm - left_cm;
        if (side_delta_cm >= SIDE_CLEAR_DELTA_CM)
        {
            return TURN_RIGHT;
        }

        if (side_delta_cm <= -SIDE_CLEAR_DELTA_CM)
        {
            return TURN_LEFT;
        }
    }
    else if (right_known && !left_known)
    {
        return TURN_RIGHT;
    }
    else if (left_known && !right_known)
    {
        return TURN_LEFT;
    }

    return previous_bias;
}

int32_t clampSteerCommand(int32_t steer_command)
{
    if (steer_command > 100)
    {
        return 100;
    }

    if (steer_command < -100)
    {
        return -100;
    }

    return steer_command;
}

int32_t wrappedHeadingErrorDeg10(int32_t target_heading_deg10, int32_t current_heading_deg10)
{
    int32_t error_deg10 = target_heading_deg10 - current_heading_deg10;
    while (error_deg10 > 1800)
    {
        error_deg10 -= 3600;
    }
    while (error_deg10 < -1800)
    {
        error_deg10 += 3600;
    }
    return error_deg10;
}

int32_t headingHoldSteerCorrection(int32_t target_heading_deg10, int32_t current_heading_deg10)
{
    const int32_t heading_error_deg10 = wrappedHeadingErrorDeg10(target_heading_deg10, current_heading_deg10);
    const int32_t correction = heading_error_deg10 / HEADING_HOLD_DEG10_PER_STEER;
    return max(-HEADING_HOLD_MAX_STEER, min(HEADING_HOLD_MAX_STEER, correction));
}

int32_t sideCenterSteerCorrection(int32_t left_cm, int32_t right_cm)
{
    if (!ultrasonicSideKnown(left_cm) || !ultrasonicSideKnown(right_cm))
    {
        return 0;
    }

    const int32_t side_delta_cm = right_cm - left_cm;
    if (abs(side_delta_cm) < SIDE_CENTER_DEADBAND_CM)
    {
        return 0;
    }
    const int32_t correction = side_delta_cm / SIDE_CENTER_CM_PER_STEER;
    return max(-SIDE_CENTER_MAX_STEER, min(SIDE_CENTER_MAX_STEER, correction));
}

int32_t maybeApplyLaunchKick(int32_t requested_speed, TickType_t phase_started_ms, TickType_t now_ms, int32_t longitudinal_acc_counts)
{
    if (requested_speed == 0)
    {
        return 0;
    }

    if (now_ms - phase_started_ms < MOTOR_KICK_MS)
    {
        return requested_speed > 0 ? MOTOR_KICK_SPEED : -MOTOR_KICK_SPEED;
    }

    if (now_ms - phase_started_ms < MOTOR_REKICK_MS &&
        abs(longitudinal_acc_counts) < MOTION_ACCEL_THRESHOLD_COUNTS)
    {
        return requested_speed > 0 ? MOTOR_KICK_SPEED : -MOTOR_KICK_SPEED;
    }

    return requested_speed;
}

int32_t chooseForwardDriveSpeed(int32_t front_distance_cm)
{
    if (front_distance_cm > 0 &&
        front_distance_cm < FORWARD_DISTANCE_UNKNOWN_CM &&
        front_distance_cm <= FORWARD_SLOWDOWN_CM)
    {
        return FORWARD_SLOW_SPEED;
    }

    return FORWARD_SPEED;
}
} // namespace

Driver::Driver(){

}

void Driver::Begin(){
  if (driver_task_started)
  {
    return;
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

  publishDriverCommand(DRIVER_IDLE, 0, 0);
  xTaskCreate(
      driverTask,
      "driverTask",
      3000,
      NULL,
      1,
      NULL);
  driver_task_started = true;
}

void Driver::drive_absolute(int direction, int distance, int speed){ // direction is the direction since start in degrees....will drift. Distance measured in mm (is relative)
   requested_direction = direction;
   requested_distance = distance;
   requested_speed = speed;
   globalVar_set(driver_desired_direction, direction);
   globalVar_set(driver_desired_distance, distance);
   globalVar_set(driver_desired_speed, speed);
}

void Driver::drive_relative(int turn, int distance, int speed){
    requested_turn = turn;
    requested_distance = distance;
    requested_speed = speed;
    globalVar_set(driver_desired_turn, turn);
    globalVar_set(driver_desired_distance, distance);
    globalVar_set(driver_desired_speed, speed);
}


void Driver::driverTask(void *pvParameters) {
    DriverRuntimeState state = DRIVER_IDLE;
    TickType_t state_started_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    int blocked_cycles = 0;
    int clear_cycles = 0;
    int recovery_turn_bias = TURN_RIGHT;
    int32_t forward_heading_target_deg10 = globalVar_get(calcHeading);
    int32_t committed_turn_bias = TURN_NEUTRAL;
    TickType_t committed_turn_started_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    TickType_t forward_started_ms = 0;
    TickType_t reverse_started_ms = 0;
    TickType_t forward_reestablished_ms = 0;

    (void)pvParameters;

    for (;;)
    {
        const int32_t forward_clear = globalVar_get(fuseForwardClear);
        const int32_t turn_bias = globalVar_get(fuseTurnBias);
        const int32_t left_distance_cm = globalVar_get(rawDistLeft);
        const int32_t right_distance_cm = globalVar_get(rawDistRight);
        const int32_t rear_distance_cm = globalVar_get(rawDistBack);
        const int32_t current_heading_deg10 = globalVar_get(calcHeading);
        const int32_t longitudinal_acc_counts = globalVar_get(cleanedAccX);
        const bool rear_blocked = rear_distance_cm > 0 && rear_distance_cm < REVERSE_BLOCK_CM;
        const TickType_t now_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;

        switch (state)
        {
        case DRIVER_IDLE:
            state = DRIVER_FORWARD;
            blocked_cycles = 0;
            clear_cycles = 0;
            forward_heading_target_deg10 = current_heading_deg10;
            forward_started_ms = now_ms;
            forward_reestablished_ms = now_ms;
            break;

        case DRIVER_FORWARD:
            if (forward_clear == FUSE_FORWARD_CLEAR)
            {
                if (clear_cycles == 0)
                {
                    forward_heading_target_deg10 = current_heading_deg10;
                    if (forward_reestablished_ms == 0)
                    {
                        forward_reestablished_ms = now_ms;
                    }
                }
                clear_cycles++;
                if (committed_turn_bias != TURN_NEUTRAL &&
                    now_ms - committed_turn_started_ms >= TURN_COMMIT_HOLD_MS &&
                    now_ms - forward_reestablished_ms >= FORWARD_REESTABLISHED_MS &&
                    turn_bias == TURN_NEUTRAL)
                {
                    committed_turn_bias = TURN_NEUTRAL;
                }

                const int32_t active_turn_bias =
                    (committed_turn_bias != TURN_NEUTRAL) ? committed_turn_bias : turn_bias;
                const int32_t heading_error_deg10 =
                    wrappedHeadingErrorDeg10(forward_heading_target_deg10, current_heading_deg10);
                const int32_t heading_correction =
                    headingHoldSteerCorrection(forward_heading_target_deg10, current_heading_deg10);
                const int32_t side_center_correction =
                    (active_turn_bias == TURN_NEUTRAL) ? sideCenterSteerCorrection(left_distance_cm, right_distance_cm) : 0;
                if (active_turn_bias == TURN_NEUTRAL &&
                    side_center_correction == 0 &&
                    abs(heading_error_deg10) >= TRIM_LEARN_HEADING_DEADBAND_DEG10)
                {
                    steer->nudgeNeutralTrim(heading_correction / ADAPTIVE_TRIM_DIVISOR);
                }

                const int32_t steer_command = clampSteerCommand(
                    commandSteerFromBias(active_turn_bias, FORWARD_STEER_BIAS) +
                    side_center_correction);
                const int32_t commanded_speed = maybeApplyLaunchKick(
                    chooseForwardDriveSpeed(globalVar_get(rawDistFront)),
                    forward_started_ms,
                    now_ms,
                    longitudinal_acc_counts);
                steer->direction(steer_command);
                drive->driving(commanded_speed);
                publishDriverCommand(DRIVER_FORWARD, commanded_speed, steer_command);
                blocked_cycles = 0;
            }
            else
            {
                if (now_ms - forward_started_ms < MIN_FORWARD_PULSE_MS)
                {
                    const int32_t active_turn_bias =
                        (committed_turn_bias != TURN_NEUTRAL) ? committed_turn_bias : turn_bias;
                    const int32_t steer_command = clampSteerCommand(
                        commandSteerFromBias(active_turn_bias, FORWARD_STEER_BIAS));
                    const int32_t commanded_speed = maybeApplyLaunchKick(
                        chooseForwardDriveSpeed(globalVar_get(rawDistFront)),
                        forward_started_ms,
                        now_ms,
                        longitudinal_acc_counts);
                    steer->direction(steer_command);
                    drive->driving(commanded_speed);
                    publishDriverCommand(DRIVER_FORWARD, commanded_speed, steer_command);
                    break;
                }

                drive->driving(0);
                steer->direction(0);
                publishDriverCommand(DRIVER_RECOVER_STOP, 0, 0);
                clear_cycles = 0;
                forward_reestablished_ms = 0;
                if (forward_clear == FUSE_FORWARD_BLOCKED)
                {
                    blocked_cycles++;
                    if (blocked_cycles >= BLOCK_CONFIRM_CYCLES)
                    {
                        if (committed_turn_bias != TURN_NEUTRAL)
                        {
                            recovery_turn_bias = committed_turn_bias;
                        }
                        else
                        {
                            recovery_turn_bias = chooseRecoveryBiasFromSides(
                                left_distance_cm,
                                right_distance_cm,
                                recovery_turn_bias);
                            committed_turn_bias = recovery_turn_bias;
                            committed_turn_started_ms = now_ms;
                        }

                        state = DRIVER_RECOVER_STOP;
                        state_started_ms = now_ms;
                        blocked_cycles = 0;
                        clear_cycles = 0;
                        forward_reestablished_ms = 0;
                    }
                }
                else
                {
                    blocked_cycles = 0;
                }
            }
            break;

        case DRIVER_RECOVER_STOP:
            drive->driving(0);
            steer->direction(commandSteerFromBias(recovery_turn_bias, REVERSE_STEER_BIAS));
            publishDriverCommand(DRIVER_RECOVER_STOP, 0, commandSteerFromBias(recovery_turn_bias, REVERSE_STEER_BIAS));
            if (now_ms - state_started_ms >= RECOVER_STOP_MS)
            {
                state = DRIVER_RECOVER_REVERSE;
                state_started_ms = now_ms;
                reverse_started_ms = now_ms;
            }
            break;

        case DRIVER_RECOVER_REVERSE:
            if (rear_blocked)
            {
                drive->driving(0);
                steer->direction(0);
                publishDriverCommand(DRIVER_RECOVER_PAUSE, 0, 0);
                state = DRIVER_RECOVER_PAUSE;
                state_started_ms = now_ms;
                clear_cycles = 0;
                forward_reestablished_ms = 0;
                break;
            }

            if (now_ms - reverse_started_ms < MIN_REVERSE_PULSE_MS)
            {
                const int32_t commanded_speed = maybeApplyLaunchKick(
                    REVERSE_SPEED,
                    reverse_started_ms,
                    now_ms,
                    longitudinal_acc_counts);
                drive->driving(commanded_speed);
                steer->direction(commandSteerFromBias(recovery_turn_bias, REVERSE_STEER_BIAS));
                publishDriverCommand(DRIVER_RECOVER_REVERSE, commanded_speed, commandSteerFromBias(recovery_turn_bias, REVERSE_STEER_BIAS));
                break;
            }

            drive->driving(REVERSE_SPEED);
            steer->direction(commandSteerFromBias(recovery_turn_bias, REVERSE_STEER_BIAS));
            publishDriverCommand(DRIVER_RECOVER_REVERSE, REVERSE_SPEED, commandSteerFromBias(recovery_turn_bias, REVERSE_STEER_BIAS));
            if (now_ms - state_started_ms >= RECOVER_REVERSE_MS)
            {
                state = DRIVER_RECOVER_PAUSE;
                state_started_ms = now_ms;
            }
            break;

        case DRIVER_RECOVER_PAUSE:
            drive->driving(0);
            steer->direction(0);
            publishDriverCommand(DRIVER_RECOVER_PAUSE, 0, 0);
            if (now_ms - state_started_ms >= RECOVER_PAUSE_MS)
            {
                if (forward_clear == FUSE_FORWARD_CLEAR)
                {
                    clear_cycles++;
                    if (clear_cycles >= CLEAR_CONFIRM_CYCLES)
                    {
                        state = DRIVER_FORWARD;
                        state_started_ms = now_ms;
                        blocked_cycles = 0;
                        clear_cycles = 0;
                        forward_heading_target_deg10 = current_heading_deg10;
                        forward_started_ms = now_ms;
                        forward_reestablished_ms = now_ms;
                    }
                }
                else if (forward_clear == FUSE_FORWARD_BLOCKED)
                {
                    state = DRIVER_RECOVER_STOP;
                    state_started_ms = now_ms;
                    blocked_cycles = 0;
                    clear_cycles = 0;
                    forward_reestablished_ms = 0;
                }
                else
                {
                    clear_cycles = 0;
                    forward_reestablished_ms = 0;
                }
            }
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(DRIVER_PERIOD_MS));
    }
}
