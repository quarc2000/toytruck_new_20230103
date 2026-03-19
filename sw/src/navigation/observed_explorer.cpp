#include <navigation/observed_explorer.h>

#include <math.h>
#include <string.h>

#include <actuators/motor.h>
#include <actuators/steer.h>
#include <basic_telemetry/basic_logger.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <variables/setget.h>

namespace
{
using namespace navigation;

enum ExplorerState
{
    ExplorerIdle = 0,
    ExplorerForward = 1,
    ExplorerRecoverStop = 2,
    ExplorerRecoverReverse = 3,
    ExplorerRecoverPause = 4,
    ExplorerFinished = 5
};

enum ControlMode
{
    ControlAutonomous = 0,
    ControlRemote = 1,
    ControlRemoteTimedOut = 2
};

enum CardinalDir
{
    DirNorth = 0,
    DirEast = 1,
    DirSouth = 2,
    DirWest = 3
};

struct FrontierPlan
{
    bool reachable;
    bool finished;
    int bias;
    CellCoord target;
};

struct RemoteControlState
{
    ControlMode mode;
    int32_t speed;
    int32_t steer;
    TickType_t timeoutMs;
    TickType_t lastCommandMs;
};

constexpr int16_t GRID_W = 100;
constexpr int16_t GRID_H = 100;
constexpr int32_t GRID_N = GRID_W * GRID_H;
constexpr int16_t GRID_CENTER_X = GRID_W / 2;
constexpr int16_t GRID_CENTER_Y = GRID_H / 2;
constexpr int32_t CELL_MM = 100;

constexpr int32_t TURN_LEFT = -1;
constexpr int32_t TURN_NEUTRAL = 0;
constexpr int32_t TURN_RIGHT = 1;
constexpr int32_t FORWARD_CLEAR = 1;
constexpr int32_t FORWARD_BLOCKED = 0;
constexpr int32_t FORWARD_SPEED = 100;
constexpr int32_t FORWARD_SLOW_SPEED = 90;
constexpr int32_t FORWARD_SLOWDOWN_CM = 30;
constexpr int32_t REVERSE_SPEED = -100;
constexpr int32_t MOTOR_KICK_SPEED = 100;
constexpr int32_t FORWARD_STEER = 30;
constexpr int32_t FORWARD_STEER_COMMITTED = 85;
constexpr int32_t REVERSE_STEER = 65;
constexpr int32_t HEADING_HOLD_MAX = 60;
constexpr int32_t HEADING_DEG10_PER_STEER = 2;
constexpr int32_t YAW_DAMP_MAX = 30;
constexpr int32_t YAW_DPS10_PER_STEER = 8;
constexpr int32_t TRUCK_LENGTH_MM = 410;
constexpr int32_t FRONT_LIDAR_SPACING_MM = 105;
constexpr int32_t FRONT_WALL_RESPONSE_MM = TRUCK_LENGTH_MM + 200;
constexpr int32_t FRONT_WALL_STEER_MAX = 35;
constexpr int32_t FRONT_WALL_MIN_VALID_MM = 40;
constexpr int32_t FRONT_WALL_TURN_COMMIT_MM = TRUCK_LENGTH_MM + 120;
constexpr int32_t FRONT_WALL_DEG10_PER_STEER_FAR = 30;
constexpr int32_t FRONT_WALL_DEG10_PER_STEER_NEAR = 18;
constexpr int32_t FRONT_WALL_NEAR_MM = 250;
constexpr int32_t SIDE_CENTER_MAX = 20;
constexpr int32_t SIDE_CENTER_CM_PER_STEER = 2;
constexpr int32_t SIDE_CENTER_DEADBAND_CM = 8;
constexpr int32_t SIDE_TARGET_CM = 10;
constexpr int32_t SIDE_MIN_CM = 5;
constexpr int32_t SIDE_NEAR_FIELD_CM = 40;
constexpr int32_t SIDE_AVOID_PER_CM = 4;
constexpr int32_t SIDE_AVOID_MAX = 40;
constexpr int32_t SIDE_AVOID_EMERGENCY = 55;
constexpr int32_t ADAPTIVE_TRIM_DIVISOR = 8;
constexpr int32_t REVERSE_BLOCK_CM = 20;
constexpr int32_t SIDE_CLEAR_DELTA_CM = 12;
constexpr int32_t TRIM_LEARN_ERROR_DEG10 = 40;
constexpr int32_t MOTION_ACCEL_THRESHOLD = 40;
constexpr int32_t MAX_SENSOR_MM = 2000;
constexpr TickType_t LOOP_MS = 100;
constexpr TickType_t RECOVER_STOP_MS = 200;
constexpr TickType_t RECOVER_REVERSE_MS = 1500;
constexpr TickType_t RECOVER_PAUSE_MS = 400;
constexpr TickType_t RECOVER_UNKNOWN_RETRY_MS = 1200;
constexpr TickType_t MOTOR_KICK_MS = 100;
constexpr TickType_t MOTOR_REKICK_MS = 200;
constexpr TickType_t TURN_COMMIT_HOLD_MS = 8000;
constexpr TickType_t FRONTIER_REPLAN_MS = 500;
constexpr TickType_t REMOTE_CONTROL_TIMEOUT_DEFAULT_MS = 1500;
constexpr TickType_t REMOTE_CONTROL_TIMEOUT_MIN_MS = 250;
constexpr TickType_t REMOTE_CONTROL_TIMEOUT_MAX_MS = 10000;

SemaphoreHandle_t explorerMutex = nullptr;
Motor *drive = nullptr;
Steer *steer = nullptr;
GridMap observedMap;
bool begun = false;
bool finished = false;
ExplorerState state = ExplorerIdle;
TickType_t stateStartedMs = 0;
TickType_t phaseStartedMs = 0;
TickType_t committedBiasStartedMs = 0;
TickType_t lastPlanMs = 0;
TickType_t lastPoseUpdateMs = 0;
int32_t headingZeroDeg10 = 0;
int32_t headingTargetDeg10 = 0;
int32_t poseXmm = 0;
int32_t poseYmm = 0;
int32_t lastCommandedSpeed = 0;
int32_t lastFrontObstacleMm = 0;
int32_t lastRearObstacleMm = 0;
int32_t committedReverseBias = TURN_RIGHT;
int32_t committedForwardBias = TURN_NEUTRAL;
FrontierPlan frontierPlan = {true, false, TURN_NEUTRAL, {GRID_CENTER_X, GRID_CENTER_Y}};
RemoteControlState remoteControl = {ControlAutonomous, 0, 0, REMOTE_CONTROL_TIMEOUT_DEFAULT_MS, 0};
uint16_t bfsQueue[GRID_N];
int16_t bfsParent[GRID_N];
uint8_t bfsSeen[GRID_N];

int clampSteer(int steerCommand)
{
    return max(-100, min(100, steerCommand));
}

int clampSpeed(int speedCommand)
{
    return max(-100, min(100, speedCommand));
}

TickType_t clampRemoteTimeoutMs(int32_t timeoutMs)
{
    if (timeoutMs <= 0)
    {
        return REMOTE_CONTROL_TIMEOUT_DEFAULT_MS;
    }
    return static_cast<TickType_t>(max<int32_t>(REMOTE_CONTROL_TIMEOUT_MIN_MS, min<int32_t>(REMOTE_CONTROL_TIMEOUT_MAX_MS, timeoutMs)));
}

int32_t wrapHeadingDeg10(int32_t heading)
{
    while (heading >= 3600) heading -= 3600;
    while (heading < 0) heading += 3600;
    return heading;
}

int32_t relativeHeadingDeg10()
{
    return wrapHeadingDeg10(globalVar_get(calcHeading) - headingZeroDeg10);
}

int32_t wrappedHeadingErrorDeg10(int32_t target, int32_t current)
{
    int32_t error = target - current;
    while (error > 1800) error -= 3600;
    while (error < -1800) error += 3600;
    return error;
}

CardinalDir cardinalFromHeading(int32_t headingDeg10)
{
    return static_cast<CardinalDir>(((wrapHeadingDeg10(headingDeg10 + 450)) / 900) % 4);
}

CardinalDir leftOf(CardinalDir dir) { return static_cast<CardinalDir>((dir + 3) % 4); }
CardinalDir rightOf(CardinalDir dir) { return static_cast<CardinalDir>((dir + 1) % 4); }
CardinalDir backOf(CardinalDir dir) { return static_cast<CardinalDir>((dir + 2) % 4); }

CellCoord stepCell(CellCoord cell, CardinalDir dir, int steps = 1)
{
    switch (dir)
    {
    case DirNorth: cell.y -= steps; break;
    case DirEast: cell.x += steps; break;
    case DirSouth: cell.y += steps; break;
    case DirWest: cell.x -= steps; break;
    }
    return cell;
}

CellCoord currentCell()
{
    return {
        static_cast<int16_t>(GRID_CENTER_X + lroundf(static_cast<float>(poseXmm) / CELL_MM)),
        static_cast<int16_t>(GRID_CENTER_Y - lroundf(static_cast<float>(poseYmm) / CELL_MM))};
}

bool ultrasonicKnown(int32_t distCm)
{
    return distCm > 0 && distCm < 199;
}

bool lidarKnown(int32_t distMm)
{
    return distMm >= FRONT_WALL_MIN_VALID_MM && distMm < 2999;
}

int32_t nearestFrontObstacleMm(int32_t lidarLeftMm, int32_t lidarRightMm, int32_t frontCm)
{
    int32_t nearest = 3000;
    if (lidarKnown(lidarLeftMm)) nearest = min(nearest, lidarLeftMm);
    if (lidarKnown(lidarRightMm)) nearest = min(nearest, lidarRightMm);
    if (ultrasonicKnown(frontCm)) nearest = min(nearest, frontCm * 10);
    return nearest;
}

bool knownFree(CellCoord cell)
{
    const MapCell c = observedMap.getCellOrDefault(cell);
    return (c.flags & CellKnown) != 0 && (c.flags & CellBlocked) == 0;
}

bool frontierCell(CellCoord cell)
{
    if (!knownFree(cell))
    {
        return false;
    }
    for (int dir = 0; dir < 4; ++dir)
    {
        const CellCoord n = stepCell(cell, static_cast<CardinalDir>(dir));
        if (!observedMap.isInside(n))
        {
            continue;
        }
        if ((observedMap.getCellOrDefault(n).flags & CellKnown) == 0)
        {
            return true;
        }
    }
    return false;
}

void markVisited(CellCoord cell)
{
    if (!observedMap.isInside(cell))
    {
        return;
    }
    observedMap.mergeFlags(cell, CellKnown | CellObserved | CellVisited);
    observedMap.clearFlags(cell, CellBlocked | CellTemporaryBlock);
    observedMap.setConfidence(cell, 255);
}

void projectRay(CellCoord origin, CardinalDir dir, int32_t distanceMm, bool blocked)
{
    if (!observedMap.isInside(origin) || distanceMm <= 0)
    {
        return;
    }

    const int32_t trustedMm = min(distanceMm, MAX_SENSOR_MM);
    const int32_t freeCells = max(0L, trustedMm / CELL_MM - (blocked ? 1L : 0L));
    for (int32_t i = 1; i <= freeCells; ++i)
    {
        const CellCoord cell = stepCell(origin, dir, i);
        if (!observedMap.isInside(cell))
        {
            break;
        }
        observedMap.mergeFlags(cell, CellKnown | CellObserved);
        observedMap.clearFlags(cell, CellBlocked | CellTemporaryBlock);
        observedMap.setConfidence(cell, 220);
    }

    if (blocked)
    {
        const CellCoord hit = stepCell(origin, dir, max<int32_t>(1, trustedMm / CELL_MM));
        if (observedMap.isInside(hit))
        {
            observedMap.mergeFlags(hit, CellKnown | CellObserved | CellBlocked);
            observedMap.setConfidence(hit, 255);
        }
    }
}

void updateObservedMap()
{
    const CellCoord robot = currentCell();
    const CardinalDir forward = cardinalFromHeading(relativeHeadingDeg10());
    const CardinalDir left = leftOf(forward);
    const CardinalDir right = rightOf(forward);
    const CardinalDir back = backOf(forward);
    const int32_t usFront = globalVar_get(rawDistFront);
    const int32_t usLeft = globalVar_get(rawDistLeft);
    const int32_t usRight = globalVar_get(rawDistRight);
    const int32_t usBack = globalVar_get(rawDistBack);
    const int32_t lidarLeft = globalVar_get(rawLidarFrontLeft);
    const int32_t lidarRight = globalVar_get(rawLidarFrontRight);

    markVisited(robot);
    if (ultrasonicKnown(usFront)) projectRay(robot, forward, usFront * 10L, true);
    if (ultrasonicKnown(usLeft)) projectRay(robot, left, usLeft * 10L, true);
    if (ultrasonicKnown(usRight)) projectRay(robot, right, usRight * 10L, true);
    if (ultrasonicKnown(usBack)) projectRay(robot, back, usBack * 10L, true);
    // The front lidars are mounted across the front bumper and both point forward.
    // Treat 2999 mm as "no reliable hit inside range", so it only clears space and does not
    // synthesize a blocked endpoint at the far edge of the trusted projection distance.
    if (lidarLeft > 0) projectRay(stepCell(robot, left), forward, lidarLeft, lidarLeft < 2999);
    if (lidarRight > 0) projectRay(stepCell(robot, right), forward, lidarRight, lidarRight < 2999);
}

void publishPose()
{
    const CellCoord cell = currentCell();
    globalVar_set(mapObservedCellSizeMm, CELL_MM);
    globalVar_set(mapProgrammedCellSizeMm, CELL_MM);
    globalVar_set(mapObservedPosePacked, pos2int(static_cast<int8_t>(cell.x), static_cast<int8_t>(cell.y), static_cast<int8_t>(relativeHeadingDeg10() / 50), static_cast<int8_t>(lastCommandedSpeed)));
    globalVar_set(mapProgrammedPosePacked, pos2int(unknownPackedGridPose()));
}

int dirToBias(CardinalDir currentDir, CardinalDir targetDir)
{
    if (targetDir == currentDir) return TURN_NEUTRAL;
    if (targetDir == leftOf(currentDir)) return TURN_LEFT;
    if (targetDir == rightOf(currentDir)) return TURN_RIGHT;
    return committedReverseBias;
}

FrontierPlan computeFrontierPlan()
{
    FrontierPlan plan = {false, false, TURN_NEUTRAL, currentCell()};
    const CellCoord start = currentCell();
    if (!observedMap.isInside(start) || !knownFree(start))
    {
        return plan;
    }

    memset(bfsSeen, 0, sizeof(bfsSeen));
    for (int i = 0; i < GRID_N; ++i) bfsParent[i] = -1;
    uint16_t head = 0;
    uint16_t tail = 0;
    const uint16_t startIndex = static_cast<uint16_t>(start.y * GRID_W + start.x);
    bfsQueue[tail++] = startIndex;
    bfsSeen[startIndex] = 1;

    int16_t targetIndex = -1;
    while (head != tail)
    {
        const uint16_t index = bfsQueue[head++];
        const CellCoord cell = {static_cast<int16_t>(index % GRID_W), static_cast<int16_t>(index / GRID_W)};
        if (frontierCell(cell))
        {
            targetIndex = static_cast<int16_t>(index);
            break;
        }

        for (int dir = 0; dir < 4; ++dir)
        {
            const CellCoord neighbor = stepCell(cell, static_cast<CardinalDir>(dir));
            if (!observedMap.isInside(neighbor) || !knownFree(neighbor))
            {
                continue;
            }
            const uint16_t neighborIndex = static_cast<uint16_t>(neighbor.y * GRID_W + neighbor.x);
            if (bfsSeen[neighborIndex] != 0)
            {
                continue;
            }
            bfsSeen[neighborIndex] = 1;
            bfsParent[neighborIndex] = static_cast<int16_t>(index);
            bfsQueue[tail++] = neighborIndex;
        }
    }

    if (targetIndex < 0)
    {
        plan.finished = true;
        return plan;
    }

    plan.reachable = true;
    plan.target = {static_cast<int16_t>(targetIndex % GRID_W), static_cast<int16_t>(targetIndex / GRID_W)};
    int16_t stepIndex = targetIndex;
    while (bfsParent[stepIndex] >= 0 && bfsParent[stepIndex] != static_cast<int16_t>(startIndex))
    {
        stepIndex = bfsParent[stepIndex];
    }

    const CardinalDir currentDir = cardinalFromHeading(relativeHeadingDeg10());
    if (stepIndex == static_cast<int16_t>(startIndex))
    {
        const CardinalDir order[] = {currentDir, leftOf(currentDir), rightOf(currentDir), backOf(currentDir)};
        for (CardinalDir dir : order)
        {
            const CellCoord n = stepCell(start, dir);
            if (observedMap.isInside(n) && (observedMap.getCellOrDefault(n).flags & CellKnown) == 0)
            {
                plan.bias = dirToBias(currentDir, dir);
                return plan;
            }
        }
        return plan;
    }

    const CellCoord next = {static_cast<int16_t>(stepIndex % GRID_W), static_cast<int16_t>(stepIndex / GRID_W)};
    CardinalDir targetDir = currentDir;
    if (next.x > start.x) targetDir = DirEast;
    else if (next.x < start.x) targetDir = DirWest;
    else if (next.y > start.y) targetDir = DirSouth;
    else if (next.y < start.y) targetDir = DirNorth;
    plan.bias = dirToBias(currentDir, targetDir);
    return plan;
}

void updatePose(TickType_t nowMs)
{
    const TickType_t dtMs = (lastPoseUpdateMs == 0) ? LOOP_MS : (nowMs - lastPoseUpdateMs);
    lastPoseUpdateMs = nowMs;
    if (lastCommandedSpeed == 0)
    {
        return;
    }

    int32_t distanceMm = abs(lastCommandedSpeed) * 4L * dtMs / 100;
    const int32_t frontMm = ultrasonicKnown(globalVar_get(rawDistFront)) ? globalVar_get(rawDistFront) * 10L : 0;
    const int32_t backMm = ultrasonicKnown(globalVar_get(rawDistBack)) ? globalVar_get(rawDistBack) * 10L : 0;
    if (lastCommandedSpeed > 0 && frontMm > 0 && lastFrontObstacleMm > 0)
    {
        const int32_t delta = lastFrontObstacleMm - frontMm;
        if (delta > 0 && delta < 250) distanceMm = delta;
        lastFrontObstacleMm = frontMm;
    }
    else if (lastCommandedSpeed < 0 && backMm > 0 && lastRearObstacleMm > 0)
    {
        const int32_t delta = lastRearObstacleMm - backMm;
        if (delta > 0 && delta < 250) distanceMm = delta;
        lastRearObstacleMm = backMm;
    }
    else
    {
        lastFrontObstacleMm = frontMm;
        lastRearObstacleMm = backMm;
    }

    if (lastCommandedSpeed < 0) distanceMm = -distanceMm;
    const float headingRad = (relativeHeadingDeg10() / 10.0f) * 3.14159265f / 180.0f;
    poseXmm += lroundf(sinf(headingRad) * distanceMm);
    poseYmm += lroundf(cosf(headingRad) * distanceMm);
}

int32_t maybeApplyKick(int32_t requestedSpeed, TickType_t startedMs, TickType_t nowMs, int32_t longitudinalAcc)
{
    if (requestedSpeed == 0) return 0;
    if (nowMs - startedMs < MOTOR_KICK_MS) return requestedSpeed > 0 ? MOTOR_KICK_SPEED : -MOTOR_KICK_SPEED;
    if (nowMs - startedMs < MOTOR_REKICK_MS && abs(longitudinalAcc) < MOTION_ACCEL_THRESHOLD) return requestedSpeed > 0 ? MOTOR_KICK_SPEED : -MOTOR_KICK_SPEED;
    return requestedSpeed;
}

void publishDriverCommand(ExplorerState activity, int speed, int steerCommand)
{
    globalVar_set(driver_driverActivity, activity);
    globalVar_set(driver_desired_speed, speed);
    globalVar_set(driver_desired_turn, steerCommand);
}

int32_t headingHoldCorrection(int32_t targetHeadingDeg10, int32_t currentHeadingDeg10)
{
    return max(-HEADING_HOLD_MAX, min(HEADING_HOLD_MAX, wrappedHeadingErrorDeg10(targetHeadingDeg10, currentHeadingDeg10) / HEADING_DEG10_PER_STEER));
}

int32_t yawDampingCorrection(int32_t yawRateDegPs10)
{
    return max(-YAW_DAMP_MAX, min(YAW_DAMP_MAX, -yawRateDegPs10 / YAW_DPS10_PER_STEER));
}

int32_t frontWallAngleDeg10(int32_t lidarLeftMm, int32_t lidarRightMm)
{
    if (!lidarKnown(lidarLeftMm) || !lidarKnown(lidarRightMm))
    {
        return 0;
    }

    const int32_t nearestWallMm = min(lidarLeftMm, lidarRightMm);
    if (nearestWallMm > FRONT_WALL_RESPONSE_MM)
    {
        return 0;
    }

    const float angleRad = atanf(static_cast<float>(lidarRightMm - lidarLeftMm) / static_cast<float>(FRONT_LIDAR_SPACING_MM));
    return lroundf(angleRad * 1800.0f / 3.14159265f);
}

int32_t frontWallAngleCorrection(int32_t lidarLeftMm, int32_t lidarRightMm)
{
    if (!lidarKnown(lidarLeftMm) || !lidarKnown(lidarRightMm))
    {
        return 0;
    }

    const int32_t nearestWallMm = min(lidarLeftMm, lidarRightMm);
    if (nearestWallMm > FRONT_WALL_RESPONSE_MM)
    {
        return 0;
    }

    const int32_t wallAngleDeg10 = frontWallAngleDeg10(lidarLeftMm, lidarRightMm);
    const int32_t divisor = nearestWallMm <= FRONT_WALL_NEAR_MM ? FRONT_WALL_DEG10_PER_STEER_NEAR : FRONT_WALL_DEG10_PER_STEER_FAR;
    return max(-FRONT_WALL_STEER_MAX, min(FRONT_WALL_STEER_MAX, wallAngleDeg10 / divisor));
}

int32_t sideWallCorrection(int32_t leftCm, int32_t rightCm)
{
    int32_t correction = 0;
    const bool leftKnown = ultrasonicKnown(leftCm);
    const bool rightKnown = ultrasonicKnown(rightCm);

    if (leftKnown && rightKnown && (leftCm <= SIDE_NEAR_FIELD_CM || rightCm <= SIDE_NEAR_FIELD_CM))
    {
        const int32_t delta = rightCm - leftCm;
        if (abs(delta) >= SIDE_CENTER_DEADBAND_CM)
        {
            correction += max(-SIDE_CENTER_MAX, min(SIDE_CENTER_MAX, delta / SIDE_CENTER_CM_PER_STEER));
        }
    }

    if (leftKnown && leftCm < SIDE_TARGET_CM)
    {
        correction += min(SIDE_AVOID_MAX, (SIDE_TARGET_CM - leftCm) * SIDE_AVOID_PER_CM);
    }
    if (rightKnown && rightCm < SIDE_TARGET_CM)
    {
        correction -= min(SIDE_AVOID_MAX, (SIDE_TARGET_CM - rightCm) * SIDE_AVOID_PER_CM);
    }

    if (leftKnown && leftCm <= SIDE_MIN_CM)
    {
        correction = max(correction, SIDE_AVOID_EMERGENCY);
    }
    if (rightKnown && rightCm <= SIDE_MIN_CM)
    {
        correction = min(correction, -SIDE_AVOID_EMERGENCY);
    }

    return clampSteer(correction);
}

int32_t constrainBiasAwayFromWall(int32_t bias, int32_t leftCm, int32_t rightCm)
{
    if (ultrasonicKnown(leftCm) && leftCm <= SIDE_MIN_CM)
    {
        return TURN_RIGHT;
    }
    if (ultrasonicKnown(rightCm) && rightCm <= SIDE_MIN_CM)
    {
        return TURN_LEFT;
    }
    if (ultrasonicKnown(leftCm) && leftCm <= SIDE_TARGET_CM && bias == TURN_LEFT)
    {
        return TURN_NEUTRAL;
    }
    if (ultrasonicKnown(rightCm) && rightCm <= SIDE_TARGET_CM && bias == TURN_RIGHT)
    {
        return TURN_NEUTRAL;
    }
    return bias;
}

int32_t forwardBiasSteer(int32_t bias, int32_t nearestFrontMm, bool committedBiasActive)
{
    if (bias == TURN_NEUTRAL)
    {
        return 0;
    }

    const int32_t magnitude =
        (committedBiasActive && nearestFrontMm <= FRONT_WALL_TURN_COMMIT_MM) ?
        FORWARD_STEER_COMMITTED :
        FORWARD_STEER;
    return bias > 0 ? magnitude : -magnitude;
}

const char *controlModeLabel(ControlMode mode)
{
    switch (mode)
    {
    case ControlAutonomous: return "auto";
    case ControlRemote: return "remote";
    case ControlRemoteTimedOut: return "remote_timeout";
    }
    return "unknown";
}

void applyStoppedOutputs(ExplorerState activity)
{
    if (steer != nullptr) steer->direction(0);
    if (drive != nullptr) drive->driving(0);
    lastCommandedSpeed = 0;
    publishDriverCommand(activity, 0, 0);
}

void enterAutonomousControlMode()
{
    remoteControl.mode = ControlAutonomous;
    remoteControl.speed = 0;
    remoteControl.steer = 0;
    remoteControl.lastCommandMs = xTaskGetTickCount() * portTICK_PERIOD_MS;
    state = ExplorerIdle;
    stateStartedMs = 0;
    phaseStartedMs = 0;
    committedBiasStartedMs = 0;
    lastFrontObstacleMm = 0;
    lastRearObstacleMm = 0;
    applyStoppedOutputs(ExplorerIdle);
}

void enterRemoteControlMode(TickType_t timeoutMs)
{
    remoteControl.mode = ControlRemote;
    remoteControl.speed = 0;
    remoteControl.steer = 0;
    remoteControl.timeoutMs = timeoutMs;
    remoteControl.lastCommandMs = xTaskGetTickCount() * portTICK_PERIOD_MS;
    state = ExplorerIdle;
    stateStartedMs = 0;
    phaseStartedMs = 0;
    committedBiasStartedMs = 0;
    lastFrontObstacleMm = 0;
    lastRearObstacleMm = 0;
    applyStoppedOutputs(ExplorerIdle);
}

void enterRemoteTimeoutMode()
{
    remoteControl.mode = ControlRemoteTimedOut;
    remoteControl.speed = 0;
    remoteControl.steer = 0;
    applyStoppedOutputs(ExplorerIdle);
    basic_log_warn("Remote control watchdog expired; truck stopped");
}

int32_t chooseForwardSpeed()
{
    const int32_t front = globalVar_get(rawDistFront);
    if (front > 0 && front < 199 && front <= FORWARD_SLOWDOWN_CM)
    {
        return FORWARD_SLOW_SPEED;
    }
    return FORWARD_SPEED;
}

int32_t chooseRecoveryBias(int32_t leftCm, int32_t rightCm, int32_t previous)
{
    const bool leftKnown = ultrasonicKnown(leftCm);
    const bool rightKnown = ultrasonicKnown(rightCm);
    if (leftKnown && rightKnown)
    {
        const int32_t delta = rightCm - leftCm;
        if (delta >= SIDE_CLEAR_DELTA_CM) return TURN_RIGHT;
        if (delta <= -SIDE_CLEAR_DELTA_CM) return TURN_LEFT;
    }
    else if (rightKnown && !leftKnown) return TURN_RIGHT;
    else if (leftKnown && !rightKnown) return TURN_LEFT;
    return previous;
}

char glyphAt(CellCoord cell)
{
    const CellCoord robot = currentCell();
    if (cell.x == robot.x && cell.y == robot.y) return 'R';
    const MapCell c = observedMap.getCellOrDefault(cell);
    if ((c.flags & CellKnown) == 0) return '?';
    if ((c.flags & CellBlocked) != 0) return '#';
    if ((c.flags & CellVisited) != 0) return 'v';
    return '.';
}

String buildMapJson()
{
    const CellCoord robot = currentCell();
    const int32_t posePacked = globalVar_get(mapObservedPosePacked);
    String out;
    out.reserve(11400);
    out = "{";
    out += "\"apiVersion\":1,";
    out += "\"width\":" + String(GRID_W) + ",";
    out += "\"height\":" + String(GRID_H) + ",";
    out += "\"cellSizeMm\":" + String(CELL_MM) + ",";
    out += "\"finished\":" + String(finished ? "true" : "false") + ",";
    out += "\"state\":" + String(static_cast<int>(state)) + ",";
    out += "\"frontierReachable\":" + String(frontierPlan.reachable ? "true" : "false") + ",";
    out += "\"frontierBias\":" + String(frontierPlan.bias) + ",";
    out += "\"posePacked\":" + String(posePacked) + ",";
    out += "\"controlMode\":\"" + String(controlModeLabel(remoteControl.mode)) + "\",";
    out += "\"robot\":{";
    out += "\"x\":" + String(robot.x) + ",";
    out += "\"y\":" + String(robot.y) + ",";
    out += "\"heading\":" + String(relativeHeadingDeg10()) + ",";
    out += "\"speed\":" + String(lastCommandedSpeed);
    out += "},";
    out += "\"rows\":[";
    for (int16_t y = 0; y < GRID_H; ++y)
    {
        if (y > 0) out += ",";
        out += "\"";
        for (int16_t x = 0; x < GRID_W; ++x) out += glyphAt({x, y});
        out += "\"";
    }
    out += "]}";
    return out;
}

} // namespace

ObservedExplorerService::ObservedExplorerService()
{
}

void ObservedExplorerService::Begin()
{
    if (begun)
    {
        return;
    }

    explorerMutex = xSemaphoreCreateMutex();
    if (drive == nullptr) drive = new Motor();
    if (steer == nullptr) steer = new Steer();
    drive->Begin();
    steer->Begin();

    MapGeometry geometry = {static_cast<uint16_t>(GRID_W), static_cast<uint16_t>(GRID_H), static_cast<uint16_t>(CELL_MM), 0, 0};
    observedMap.reset(geometry);
    headingZeroDeg10 = globalVar_get(calcHeading);
    enterAutonomousControlMode();
    publishPose();
    basic_log_info("Observed explorer started on 100x100 grid");

    xTaskCreate(taskEntry, "obsExplorer", 6000, this, 1, nullptr);
    begun = true;
}

void ObservedExplorerService::taskEntry(void *parameter)
{
    static_cast<ObservedExplorerService *>(parameter)->runTask();
}

void ObservedExplorerService::runTask()
{
    for (;;)
    {
        const TickType_t nowMs = xTaskGetTickCount() * portTICK_PERIOD_MS;
        const int32_t forwardClear = globalVar_get(fuseForwardClear);
        const int32_t sensorTurnBias = globalVar_get(fuseTurnBias);
        const int32_t leftCm = globalVar_get(rawDistLeft);
        const int32_t rightCm = globalVar_get(rawDistRight);
        const int32_t rearCm = globalVar_get(rawDistBack);
        const int32_t headingDeg10 = relativeHeadingDeg10();
        const int32_t yawRateDegPs10 = globalVar_get(cleanedGyZ);
        const int32_t lidarLeftMm = globalVar_get(rawLidarFrontLeft);
        const int32_t lidarRightMm = globalVar_get(rawLidarFrontRight);
        const int32_t nearestFrontMm = nearestFrontObstacleMm(lidarLeftMm, lidarRightMm, globalVar_get(rawDistFront));
        const int32_t longitudinalAcc = globalVar_get(cleanedAccX);
        const bool rearBlocked = rearCm > 0 && rearCm < REVERSE_BLOCK_CM;

        xSemaphoreTake(explorerMutex, portMAX_DELAY);
        updatePose(nowMs);
        updateObservedMap();
        publishPose();
        if (nowMs - lastPlanMs >= FRONTIER_REPLAN_MS)
        {
            frontierPlan = computeFrontierPlan();
            lastPlanMs = nowMs;
        }

        if (remoteControl.mode == ControlRemote)
        {
            if (remoteControl.timeoutMs > 0 && nowMs - remoteControl.lastCommandMs >= remoteControl.timeoutMs)
            {
                enterRemoteTimeoutMode();
            }
            else
            {
                steer->direction(remoteControl.steer);
                drive->driving(remoteControl.speed);
                lastCommandedSpeed = remoteControl.speed;
                publishDriverCommand(ExplorerIdle, remoteControl.speed, remoteControl.steer);
            }
        }

        if (remoteControl.mode != ControlAutonomous)
        {
            xSemaphoreGive(explorerMutex);
            vTaskDelay(pdMS_TO_TICKS(LOOP_MS));
            continue;
        }

        if (frontierPlan.finished)
        {
            finished = true;
            state = ExplorerFinished;
        }

        switch (state)
        {
        case ExplorerIdle:
            state = ExplorerForward;
            stateStartedMs = nowMs;
            phaseStartedMs = nowMs;
            headingTargetDeg10 = headingDeg10;
            break;

        case ExplorerForward:
            if (finished)
            {
                steer->direction(0);
                drive->driving(0);
                lastCommandedSpeed = 0;
                publishDriverCommand(ExplorerFinished, 0, 0);
                break;
            }

            if (forwardClear == FORWARD_CLEAR)
            {
                const int32_t plannedBias = frontierPlan.bias != TURN_NEUTRAL ? frontierPlan.bias : sensorTurnBias;
                const bool committedBiasActive =
                    committedForwardBias != TURN_NEUTRAL &&
                    (nowMs - committedBiasStartedMs) < TURN_COMMIT_HOLD_MS;
                const int32_t rawBias = committedBiasActive ? committedForwardBias : plannedBias;
                const int32_t bias = constrainBiasAwayFromWall(rawBias, leftCm, rightCm);
                if (lastCommandedSpeed <= 0)
                {
                    // Capture the forward heading target at the actual motion start, not only when the
                    // state changes, so the truck resists early drift in the first few centimeters.
                    headingTargetDeg10 = headingDeg10;
                    phaseStartedMs = nowMs;
                }
                const int32_t headingError = wrappedHeadingErrorDeg10(headingTargetDeg10, headingDeg10);
                const int32_t headingCorrection = headingHoldCorrection(headingTargetDeg10, headingDeg10);
                const int32_t yawCorrection = yawDampingCorrection(yawRateDegPs10);
                const int32_t wallAngleCorrection = frontWallAngleCorrection(lidarLeftMm, lidarRightMm);
                const int32_t wallCorrection = sideWallCorrection(leftCm, rightCm);
                if (bias == TURN_NEUTRAL && wallCorrection == 0 && abs(headingError) >= TRIM_LEARN_ERROR_DEG10)
                {
                    steer->nudgeNeutralTrim(headingCorrection / ADAPTIVE_TRIM_DIVISOR);
                }
                const int32_t steerCommand = clampSteer(
                    forwardBiasSteer(bias, nearestFrontMm, committedBiasActive) +
                    headingCorrection +
                    yawCorrection +
                    wallAngleCorrection +
                    wallCorrection);
                const int32_t speed = maybeApplyKick(chooseForwardSpeed(), phaseStartedMs, nowMs, longitudinalAcc);
                steer->direction(steerCommand);
                drive->driving(speed);
                lastCommandedSpeed = speed;
                publishDriverCommand(ExplorerForward, speed, steerCommand);
            }
            else
            {
                steer->direction(0);
                drive->driving(0);
                lastCommandedSpeed = 0;
                publishDriverCommand(ExplorerRecoverStop, 0, 0);
                committedReverseBias = chooseRecoveryBias(leftCm, rightCm, committedReverseBias);
                committedForwardBias = committedReverseBias;
                committedBiasStartedMs = nowMs;
                state = ExplorerRecoverStop;
                stateStartedMs = nowMs;
            }
            break;

        case ExplorerRecoverStop:
        {
            const int32_t steerCommand = committedReverseBias > 0 ? REVERSE_STEER : -REVERSE_STEER;
            steer->direction(steerCommand);
            drive->driving(0);
            lastCommandedSpeed = 0;
            publishDriverCommand(ExplorerRecoverStop, 0, steerCommand);
            if (nowMs - stateStartedMs >= RECOVER_STOP_MS)
            {
                state = ExplorerRecoverReverse;
                stateStartedMs = nowMs;
                phaseStartedMs = nowMs;
            }
            break;
        }

        case ExplorerRecoverReverse:
            if (rearBlocked)
            {
                steer->direction(0);
                drive->driving(0);
                lastCommandedSpeed = 0;
                publishDriverCommand(ExplorerRecoverPause, 0, 0);
                state = ExplorerRecoverPause;
                stateStartedMs = nowMs;
                break;
            }
            else
            {
                const int32_t steerCommand = committedReverseBias > 0 ? REVERSE_STEER : -REVERSE_STEER;
                const int32_t speed = maybeApplyKick(REVERSE_SPEED, phaseStartedMs, nowMs, longitudinalAcc);
                steer->direction(steerCommand);
                drive->driving(speed);
                lastCommandedSpeed = speed;
                publishDriverCommand(ExplorerRecoverReverse, speed, steerCommand);
                if (nowMs - stateStartedMs >= RECOVER_REVERSE_MS)
                {
                    state = ExplorerRecoverPause;
                    stateStartedMs = nowMs;
                }
            }
            break;

        case ExplorerRecoverPause:
            steer->direction(0);
            drive->driving(0);
            lastCommandedSpeed = 0;
            publishDriverCommand(ExplorerRecoverPause, 0, 0);
            if (nowMs - stateStartedMs >= RECOVER_PAUSE_MS)
            {
                if (forwardClear == FORWARD_CLEAR)
                {
                    if (nowMs - committedBiasStartedMs >= TURN_COMMIT_HOLD_MS)
                    {
                        committedForwardBias = TURN_NEUTRAL;
                    }
                    state = ExplorerForward;
                    stateStartedMs = nowMs;
                    phaseStartedMs = nowMs;
                    headingTargetDeg10 = headingDeg10;
                }
                else if (forwardClear == FORWARD_BLOCKED)
                {
                    state = ExplorerRecoverStop;
                    stateStartedMs = nowMs;
                }
                else if (nowMs - phaseStartedMs >= RECOVER_UNKNOWN_RETRY_MS)
                {
                    // Sensor uncertainty should not leave the exploration runtime parked forever.
                    // Re-enter recovery and keep the committed reverse bias until a clear forward
                    // corridor or a true finished map state appears.
                    state = ExplorerRecoverStop;
                    stateStartedMs = nowMs;
                    phaseStartedMs = nowMs;
                }
            }
            break;

        case ExplorerFinished:
            steer->direction(0);
            drive->driving(0);
            lastCommandedSpeed = 0;
            publishDriverCommand(ExplorerFinished, 0, 0);
            break;
        }

        xSemaphoreGive(explorerMutex);
        vTaskDelay(pdMS_TO_TICKS(LOOP_MS));
    }
}

String ObservedExplorerService::renderStatusJson() const
{
    if (explorerMutex == nullptr)
    {
        return "{}";
    }
    xSemaphoreTake(explorerMutex, portMAX_DELAY);
    String out = "{";
    out += "\"finished\":" + String(finished ? "true" : "false") + ",";
    out += "\"state\":" + String(static_cast<int>(state)) + ",";
    out += "\"frontierReachable\":" + String(frontierPlan.reachable ? "true" : "false") + ",";
    out += "\"frontierBias\":" + String(frontierPlan.bias) + ",";
    out += "\"posePacked\":" + String(globalVar_get(mapObservedPosePacked)) + ",";
    out += "\"heading\":" + String(relativeHeadingDeg10()) + ",";
    out += "\"controlMode\":\"" + String(controlModeLabel(remoteControl.mode)) + "\",";
    out += "\"remoteTimeoutMs\":" + String(remoteControl.timeoutMs) + ",";
    out += "\"remoteAgeMs\":" + String((xTaskGetTickCount() * portTICK_PERIOD_MS) - remoteControl.lastCommandMs) + ",";
    out += "\"driverSpeed\":" + String(globalVar_get(driver_desired_speed)) + ",";
    out += "\"driverTurn\":" + String(globalVar_get(driver_desired_turn));
    out += "}";
    xSemaphoreGive(explorerMutex);
    return out;
}

String ObservedExplorerService::renderMapJson() const
{
    if (explorerMutex == nullptr)
    {
        return "{}";
    }
    xSemaphoreTake(explorerMutex, portMAX_DELAY);
    const String out = buildMapJson();
    xSemaphoreGive(explorerMutex);
    return out;
}

String ObservedExplorerService::renderControlJson() const
{
    if (explorerMutex == nullptr)
    {
        return "{}";
    }
    xSemaphoreTake(explorerMutex, portMAX_DELAY);
    const TickType_t nowMs = xTaskGetTickCount() * portTICK_PERIOD_MS;
    String out = "{";
    out += "\"controlMode\":\"" + String(controlModeLabel(remoteControl.mode)) + "\",";
    out += "\"remoteTimeoutMs\":" + String(remoteControl.timeoutMs) + ",";
    out += "\"remoteAgeMs\":" + String(nowMs - remoteControl.lastCommandMs) + ",";
    out += "\"speed\":" + String(remoteControl.speed) + ",";
    out += "\"steer\":" + String(remoteControl.steer) + ",";
    out += "\"autonomyAvailable\":true";
    out += "}";
    xSemaphoreGive(explorerMutex);
    return out;
}

String ObservedExplorerService::renderSummaryHtml() const
{
    if (explorerMutex == nullptr)
    {
        return "<p>Explorer not started.</p>";
    }
    xSemaphoreTake(explorerMutex, portMAX_DELAY);
    String html = "<ul>";
    html += "<li>Finished: <code>" + String(finished ? "yes" : "no") + "</code></li>";
    html += "<li>State: <code>" + String(static_cast<int>(state)) + "</code></li>";
    html += "<li>Frontier reachable: <code>" + String(frontierPlan.reachable ? "yes" : "no") + "</code></li>";
    html += "<li>Frontier bias: <code>" + String(frontierPlan.bias) + "</code></li>";
    html += "<li>Control mode: <code>" + String(controlModeLabel(remoteControl.mode)) + "</code></li>";
    html += "<li>Observed pose: <code>" + String(globalVar_get(mapObservedPosePacked)) + "</code></li>";
    html += "</ul>";
    xSemaphoreGive(explorerMutex);
    return html;
}

bool ObservedExplorerService::enableRemoteControl(int32_t timeoutMs)
{
    if (explorerMutex == nullptr)
    {
        return false;
    }
    xSemaphoreTake(explorerMutex, portMAX_DELAY);
    enterRemoteControlMode(clampRemoteTimeoutMs(timeoutMs));
    xSemaphoreGive(explorerMutex);
    basic_log_info("Remote control enabled");
    return true;
}

bool ObservedExplorerService::disableRemoteControl()
{
    if (explorerMutex == nullptr)
    {
        return false;
    }
    xSemaphoreTake(explorerMutex, portMAX_DELAY);
    enterAutonomousControlMode();
    xSemaphoreGive(explorerMutex);
    basic_log_info("Remote control disabled; autonomous explorer resumed");
    return true;
}

bool ObservedExplorerService::remoteDrive(int speed, int steerCommand, int32_t timeoutMs)
{
    if (explorerMutex == nullptr)
    {
        return false;
    }
    xSemaphoreTake(explorerMutex, portMAX_DELAY);
    if (remoteControl.mode != ControlRemote)
    {
        xSemaphoreGive(explorerMutex);
        return false;
    }
    remoteControl.speed = clampSpeed(speed);
    remoteControl.steer = clampSteer(steerCommand);
    remoteControl.timeoutMs = clampRemoteTimeoutMs(timeoutMs);
    remoteControl.lastCommandMs = xTaskGetTickCount() * portTICK_PERIOD_MS;
    xSemaphoreGive(explorerMutex);
    return true;
}

bool ObservedExplorerService::remoteStop(int32_t timeoutMs)
{
    if (explorerMutex == nullptr)
    {
        return false;
    }
    xSemaphoreTake(explorerMutex, portMAX_DELAY);
    if (remoteControl.mode != ControlRemote)
    {
        xSemaphoreGive(explorerMutex);
        return false;
    }
    remoteControl.speed = 0;
    remoteControl.steer = 0;
    remoteControl.timeoutMs = clampRemoteTimeoutMs(timeoutMs);
    remoteControl.lastCommandMs = xTaskGetTickCount() * portTICK_PERIOD_MS;
    applyStoppedOutputs(ExplorerIdle);
    xSemaphoreGive(explorerMutex);
    return true;
}
