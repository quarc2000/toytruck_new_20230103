#ifndef NAVIGATION_GRID_MAP_H
#define NAVIGATION_GRID_MAP_H

#include <stddef.h>
#include <stdint.h>

namespace navigation
{

static const uint16_t MAP_FORMAT_VERSION = 1;

enum CellFlags : uint16_t
{
    CellKnown = 1 << 0,
    CellBlocked = 1 << 1,
    CellVisited = 1 << 2,
    CellPlannedPath = 1 << 3,
    CellStart = 1 << 4,
    CellGoal = 1 << 5,
    CellProgrammed = 1 << 6,
    CellObserved = 1 << 7,
    CellTemporaryBlock = 1 << 8,
    CellLowConfidence = 1 << 9
};

struct CellCoord
{
    int16_t x;
    int16_t y;
};

struct PackedGridPose
{
    int8_t x;          // Grid X coordinate in cells. Positive is east. 0 is the map-frame origin.
    int8_t y;          // Grid Y coordinate in cells. Positive is north. 0 is the map-frame origin.
    int8_t direction;  // Heading in 5 degree clockwise steps from north. 0 = north, 18 = east.
    int8_t speed;      // Signed speed in cm/s for compact pose exchange. Positive is forward.
};

static const int8_t GRID_POSE_UNKNOWN = static_cast<int8_t>(-128);

struct MapGeometry
{
    uint16_t width;
    uint16_t height;
    uint16_t cell_size_mm;
    int32_t origin_x_mm;
    int32_t origin_y_mm;
};

struct MapCell
{
    uint16_t flags;
    uint8_t confidence;
    uint8_t reserved;
};

struct MapAlignment
{
    int8_t offset_x;   // Grid-cell offset from observed-map origin to programmed-map origin. -128 = unknown.
    int8_t offset_y;   // Grid-cell offset from observed-map origin to programmed-map origin. -128 = unknown.
    int16_t heading_deg10;
    uint16_t confidence_permille;
};

struct ProgrammedMapDefinition
{
    uint16_t version;
    MapGeometry geometry;
    CellCoord start;
    CellCoord goal;
    const MapCell *cells;
};

class GridMap
{
public:
    GridMap();
    ~GridMap();

    GridMap(const GridMap &) = delete;
    GridMap &operator=(const GridMap &) = delete;

    bool reset(const MapGeometry &geometry);
    void clear();
    bool isConfigured() const;
    size_t cellCount() const;
    const MapGeometry &geometry() const;

    bool isInside(CellCoord coord) const;
    bool isInside(int16_t x, int16_t y) const;

    MapCell getCellOrDefault(CellCoord coord) const;
    bool getCell(CellCoord coord, MapCell *out_cell) const;
    bool setCell(CellCoord coord, const MapCell &cell);
    bool mergeFlags(CellCoord coord, uint16_t flags);
    bool clearFlags(CellCoord coord, uint16_t flags);
    bool setConfidence(CellCoord coord, uint8_t confidence);
    uint16_t cellSizeMm() const;
    uint32_t distanceSquaredMm(CellCoord a, CellCoord b) const;

    bool loadFromDefinition(const ProgrammedMapDefinition &definition);

private:
    size_t indexOf(CellCoord coord) const;

    MapGeometry geometry_;
    MapCell *cells_;
    bool configured_;
};

class MapBundle
{
public:
    MapBundle();

    MapBundle(const MapBundle &) = delete;
    MapBundle &operator=(const MapBundle &) = delete;

    bool loadProgrammedMap(const ProgrammedMapDefinition &definition);
    void clearObserved();

    GridMap programmed;
    GridMap observed;
    CellCoord start;
    CellCoord goal;
    int32_t observed_pose_packed;
    int32_t programmed_pose_packed;
    bool programmed_pose_valid;
    MapAlignment observed_to_programmed;
};

CellCoord invalidCellCoord();
MapCell makeCell(uint16_t flags, uint8_t confidence);
PackedGridPose unknownPackedGridPose();
// Packs x, y, direction, and speed into one signed 32-bit integer using four signed bytes.
// Field conventions:
//   x: grid X coordinate, positive east
//   y: grid Y coordinate, positive north
//   direction: 5 degree clockwise steps from north
//   speed: signed cm/s for compact shared state
// GRID_POSE_UNKNOWN (-128) is reserved as unknown for any field.
int32_t pos2int(PackedGridPose pose);
PackedGridPose int2pos(int32_t packed_value);
int32_t pos2int(int8_t x, int8_t y, int8_t direction, int8_t speed);
CellCoord packedPoseToCellCoord(int32_t packed_value);

} // namespace navigation

#endif
