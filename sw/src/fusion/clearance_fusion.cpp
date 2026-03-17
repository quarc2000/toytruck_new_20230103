#include <stdint.h>
#include <fusion/clearance_fusion.h>
#include <variables/setget.h>

static constexpr int32_t FUSE_FORWARD_UNKNOWN = -1;
static constexpr int32_t FUSE_FORWARD_BLOCKED = 0;
static constexpr int32_t FUSE_FORWARD_CLEAR = 1;
static constexpr int32_t FORWARD_BLOCKED_MM_THRESHOLD = 150;
static constexpr int32_t FORWARD_CLEAR_MM_THRESHOLD = 250;
static constexpr int32_t LIDAR_MIN_VALID_MM = 40;
static constexpr int32_t FUSE_TURN_LEFT = -1;
static constexpr int32_t FUSE_TURN_NEUTRAL = 0;
static constexpr int32_t FUSE_TURN_RIGHT = 1;
static constexpr int32_t TURN_BIAS_ENGAGE_DELTA_MM = 220;
static constexpr int32_t TURN_BIAS_RELEASE_DELTA_MM = 100;

static int32_t ultrasonicFrontState(int32_t raw_dist_front_cm)
{
    // `199` is currently shared between timeout and out-of-range, so it is not
    // safe to treat that as definitely clear.
    if (raw_dist_front_cm <= 0 || raw_dist_front_cm >= 199)
    {
        return FUSE_FORWARD_UNKNOWN;
    }

    const int32_t forward_mm = raw_dist_front_cm * 10;
    if (forward_mm < FORWARD_BLOCKED_MM_THRESHOLD)
    {
        return FUSE_FORWARD_BLOCKED;
    }

    if (forward_mm >= FORWARD_CLEAR_MM_THRESHOLD)
    {
        return FUSE_FORWARD_CLEAR;
    }

    return FUSE_FORWARD_UNKNOWN;
}

static int32_t lidarForwardBiasState(int32_t raw_lidar_front_mm)
{
    if (raw_lidar_front_mm <= 0 || raw_lidar_front_mm < LIDAR_MIN_VALID_MM)
    {
        return FUSE_FORWARD_UNKNOWN;
    }

    if (raw_lidar_front_mm < FORWARD_BLOCKED_MM_THRESHOLD)
    {
        return FUSE_FORWARD_BLOCKED;
    }

    if (raw_lidar_front_mm >= FORWARD_CLEAR_MM_THRESHOLD)
    {
        return FUSE_FORWARD_CLEAR;
    }

    return FUSE_FORWARD_UNKNOWN;
}

static int32_t forwardClearHysteresis(int32_t candidate_state)
{
    static int32_t last_forward_state = FUSE_FORWARD_UNKNOWN;

    if (candidate_state != FUSE_FORWARD_UNKNOWN)
    {
        last_forward_state = candidate_state;
        return candidate_state;
    }

    return last_forward_state;
}

static int32_t lidarFrontState(int32_t raw_lidar_front_mm)
{
    return lidarForwardBiasState(raw_lidar_front_mm);
}

static int32_t turnBiasHysteresis(int32_t distance_delta_mm)
{
    static int32_t last_turn_bias = FUSE_TURN_NEUTRAL;

    if (last_turn_bias == FUSE_TURN_RIGHT)
    {
        if (distance_delta_mm < -TURN_BIAS_ENGAGE_DELTA_MM)
        {
            last_turn_bias = FUSE_TURN_LEFT;
        }
        else if (distance_delta_mm <= TURN_BIAS_RELEASE_DELTA_MM)
        {
            last_turn_bias = FUSE_TURN_NEUTRAL;
        }
        return last_turn_bias;
    }

    if (last_turn_bias == FUSE_TURN_LEFT)
    {
        if (distance_delta_mm > TURN_BIAS_ENGAGE_DELTA_MM)
        {
            last_turn_bias = FUSE_TURN_RIGHT;
        }
        else if (distance_delta_mm >= -TURN_BIAS_RELEASE_DELTA_MM)
        {
            last_turn_bias = FUSE_TURN_NEUTRAL;
        }
        return last_turn_bias;
    }

    if (distance_delta_mm > TURN_BIAS_ENGAGE_DELTA_MM)
    {
        last_turn_bias = FUSE_TURN_RIGHT;
    }
    else if (distance_delta_mm < -TURN_BIAS_ENGAGE_DELTA_MM)
    {
        last_turn_bias = FUSE_TURN_LEFT;
    }

    return last_turn_bias;
}

int fusionComputeForwardClear()
{
    const int32_t usensor_state = ultrasonicFrontState(globalVar_get(rawDistFront));
    const int32_t lidar_right_state = lidarFrontState(globalVar_get(rawLidarFrontRight));
    const int32_t lidar_left_state = lidarFrontState(globalVar_get(rawLidarFrontLeft));

    if (usensor_state == FUSE_FORWARD_BLOCKED ||
        lidar_right_state == FUSE_FORWARD_BLOCKED ||
        lidar_left_state == FUSE_FORWARD_BLOCKED)
    {
        return forwardClearHysteresis(FUSE_FORWARD_BLOCKED);
    }

    if (usensor_state == FUSE_FORWARD_CLEAR &&
        lidar_right_state == FUSE_FORWARD_CLEAR &&
        lidar_left_state == FUSE_FORWARD_CLEAR)
    {
        return forwardClearHysteresis(FUSE_FORWARD_CLEAR);
    }

    return forwardClearHysteresis(FUSE_FORWARD_UNKNOWN);
}

int fusionComputeTurnBias()
{
    const int32_t lidar_right_mm = globalVar_get(rawLidarFrontRight);
    const int32_t lidar_left_mm = globalVar_get(rawLidarFrontLeft);

    if (lidar_right_mm <= 0 || lidar_left_mm <= 0 ||
        lidar_right_mm < LIDAR_MIN_VALID_MM || lidar_left_mm < LIDAR_MIN_VALID_MM)
    {
        return FUSE_TURN_NEUTRAL;
    }

    const int32_t distance_delta_mm = lidar_right_mm - lidar_left_mm;
    return turnBiasHysteresis(distance_delta_mm);
}
