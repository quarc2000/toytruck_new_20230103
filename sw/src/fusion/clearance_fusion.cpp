#include <stdint.h>
#include <fusion/clearance_fusion.h>
#include <variables/setget.h>

static constexpr int32_t FUSE_FORWARD_UNKNOWN = -1;
static constexpr int32_t FUSE_FORWARD_BLOCKED = 0;
static constexpr int32_t FUSE_FORWARD_CLEAR = 1;
static constexpr int32_t FORWARD_CLEARANCE_MM_THRESHOLD = 250;

static int32_t ultrasonicFrontState(int32_t raw_dist_front_cm)
{
    // `199` is currently shared between timeout and out-of-range, so it is not
    // safe to treat that as definitely clear.
    if (raw_dist_front_cm <= 0 || raw_dist_front_cm >= 199)
    {
        return FUSE_FORWARD_UNKNOWN;
    }

    if (raw_dist_front_cm * 10 < FORWARD_CLEARANCE_MM_THRESHOLD)
    {
        return FUSE_FORWARD_BLOCKED;
    }

    return FUSE_FORWARD_CLEAR;
}

static int32_t lidarFrontState(int32_t raw_lidar_front_mm)
{
    if (raw_lidar_front_mm <= 0)
    {
        return FUSE_FORWARD_UNKNOWN;
    }

    if (raw_lidar_front_mm < FORWARD_CLEARANCE_MM_THRESHOLD)
    {
        return FUSE_FORWARD_BLOCKED;
    }

    return FUSE_FORWARD_CLEAR;
}

int fusionComputeForwardClear()
{
    const int32_t usensor_state = ultrasonicFrontState(globalVar_get(rawDistFront));
    const int32_t lidar_state = lidarFrontState(globalVar_get(rawLidarFront));

    if (usensor_state == FUSE_FORWARD_BLOCKED || lidar_state == FUSE_FORWARD_BLOCKED)
    {
        return FUSE_FORWARD_BLOCKED;
    }

    if (usensor_state == FUSE_FORWARD_CLEAR || lidar_state == FUSE_FORWARD_CLEAR)
    {
        return FUSE_FORWARD_CLEAR;
    }

    return FUSE_FORWARD_UNKNOWN;
}
