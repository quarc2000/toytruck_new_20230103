#include "navigation/grid_map.h"

#include <new>
#include <string.h>

namespace navigation
{

CellCoord invalidCellCoord()
{
    CellCoord coord = {-1, -1};
    return coord;
}

PackedGridPose unknownPackedGridPose()
{
    PackedGridPose pose = {GRID_POSE_UNKNOWN, GRID_POSE_UNKNOWN, GRID_POSE_UNKNOWN, GRID_POSE_UNKNOWN};
    return pose;
}

int32_t pos2int(PackedGridPose pose)
{
    // Keep packed pose exchange byte-stable across tasks and storage.
    const uint32_t x = static_cast<uint8_t>(pose.x);
    const uint32_t y = static_cast<uint8_t>(pose.y);
    const uint32_t direction = static_cast<uint8_t>(pose.direction);
    const uint32_t speed = static_cast<uint8_t>(pose.speed);
    return static_cast<int32_t>(x | (y << 8) | (direction << 16) | (speed << 24));
}

PackedGridPose int2pos(int32_t packed_value)
{
    const uint32_t raw = static_cast<uint32_t>(packed_value);
    PackedGridPose pose;
    pose.x = static_cast<int8_t>(raw & 0xFF);
    pose.y = static_cast<int8_t>((raw >> 8) & 0xFF);
    pose.direction = static_cast<int8_t>((raw >> 16) & 0xFF);
    pose.speed = static_cast<int8_t>((raw >> 24) & 0xFF);
    return pose;
}

int32_t pos2int(int8_t x, int8_t y, int8_t direction, int8_t speed)
{
    PackedGridPose pose = {x, y, direction, speed};
    return pos2int(pose);
}

CellCoord packedPoseToCellCoord(int32_t packed_value)
{
    const PackedGridPose pose = int2pos(packed_value);
    CellCoord coord = {pose.x, pose.y};
    return coord;
}

MapCell makeCell(uint16_t flags, uint8_t confidence)
{
    MapCell cell = {flags, confidence, 0};
    return cell;
}

GridMap::GridMap() : cells_(NULL), configured_(false)
{
    memset(&geometry_, 0, sizeof(geometry_));
}

GridMap::~GridMap()
{
    delete[] cells_;
}

bool GridMap::reset(const MapGeometry &geometry)
{
    if (geometry.width == 0 || geometry.height == 0 || geometry.cell_size_mm == 0)
    {
        return false;
    }

    const size_t requested_cells = static_cast<size_t>(geometry.width) * static_cast<size_t>(geometry.height);
    MapCell *new_cells = new (std::nothrow) MapCell[requested_cells];
    if (new_cells == NULL)
    {
        return false;
    }

    delete[] cells_;
    cells_ = new_cells;
    geometry_ = geometry;
    configured_ = true;
    clear();
    return true;
}

void GridMap::clear()
{
    if (!configured_ || cells_ == NULL)
    {
        return;
    }

    memset(cells_, 0, sizeof(MapCell) * cellCount());
}

bool GridMap::isConfigured() const
{
    return configured_;
}

size_t GridMap::cellCount() const
{
    if (!configured_)
    {
        return 0;
    }
    return static_cast<size_t>(geometry_.width) * static_cast<size_t>(geometry_.height);
}

const MapGeometry &GridMap::geometry() const
{
    return geometry_;
}

bool GridMap::isInside(CellCoord coord) const
{
    return isInside(coord.x, coord.y);
}

bool GridMap::isInside(int16_t x, int16_t y) const
{
    if (!configured_)
    {
        return false;
    }
    return x >= 0 && y >= 0 && x < static_cast<int16_t>(geometry_.width) && y < static_cast<int16_t>(geometry_.height);
}

size_t GridMap::indexOf(CellCoord coord) const
{
    return static_cast<size_t>(coord.y) * static_cast<size_t>(geometry_.width) + static_cast<size_t>(coord.x);
}

MapCell GridMap::getCellOrDefault(CellCoord coord) const
{
    MapCell default_cell = {0, 0, 0};
    if (!isInside(coord) || cells_ == NULL)
    {
        return default_cell;
    }
    return cells_[indexOf(coord)];
}

bool GridMap::getCell(CellCoord coord, MapCell *out_cell) const
{
    if (out_cell == NULL || !isInside(coord) || cells_ == NULL)
    {
        return false;
    }

    *out_cell = cells_[indexOf(coord)];
    return true;
}

bool GridMap::setCell(CellCoord coord, const MapCell &cell)
{
    if (!isInside(coord) || cells_ == NULL)
    {
        return false;
    }

    cells_[indexOf(coord)] = cell;
    return true;
}

bool GridMap::mergeFlags(CellCoord coord, uint16_t flags)
{
    if (!isInside(coord) || cells_ == NULL)
    {
        return false;
    }

    cells_[indexOf(coord)].flags |= flags;
    return true;
}

bool GridMap::clearFlags(CellCoord coord, uint16_t flags)
{
    if (!isInside(coord) || cells_ == NULL)
    {
        return false;
    }

    cells_[indexOf(coord)].flags &= ~flags;
    return true;
}

bool GridMap::setConfidence(CellCoord coord, uint8_t confidence)
{
    if (!isInside(coord) || cells_ == NULL)
    {
        return false;
    }

    cells_[indexOf(coord)].confidence = confidence;
    return true;
}

uint16_t GridMap::cellSizeMm() const
{
    return geometry_.cell_size_mm;
}

uint32_t GridMap::distanceSquaredMm(CellCoord a, CellCoord b) const
{
    if (!isInside(a) || !isInside(b))
    {
        return 0;
    }

    // Tasks can derive physical distance from grid delta using the map's configured cell size.
    const int32_t dx_cells = static_cast<int32_t>(a.x) - static_cast<int32_t>(b.x);
    const int32_t dy_cells = static_cast<int32_t>(a.y) - static_cast<int32_t>(b.y);
    const int32_t dx_mm = dx_cells * geometry_.cell_size_mm;
    const int32_t dy_mm = dy_cells * geometry_.cell_size_mm;
    return static_cast<uint32_t>(dx_mm * dx_mm + dy_mm * dy_mm);
}

bool GridMap::loadFromDefinition(const ProgrammedMapDefinition &definition)
{
    if (definition.version != MAP_FORMAT_VERSION || definition.cells == NULL)
    {
        return false;
    }

    if (!reset(definition.geometry))
    {
        return false;
    }

    if (!isInside(definition.start) || !isInside(definition.goal))
    {
        return false;
    }

    memcpy(cells_, definition.cells, sizeof(MapCell) * cellCount());
    mergeFlags(definition.start, CellStart);
    mergeFlags(definition.goal, CellGoal);
    return true;
}

MapBundle::MapBundle()
{
    start = invalidCellCoord();
    goal = invalidCellCoord();
    observed_pose_packed = pos2int(unknownPackedGridPose());
    programmed_pose_packed = pos2int(unknownPackedGridPose());
    programmed_pose_valid = false;

    observed_to_programmed.offset_x = GRID_POSE_UNKNOWN;
    observed_to_programmed.offset_y = GRID_POSE_UNKNOWN;
    observed_to_programmed.heading_deg10 = 0;
    observed_to_programmed.confidence_permille = 0;
}

bool MapBundle::loadProgrammedMap(const ProgrammedMapDefinition &definition)
{
    if (!programmed.loadFromDefinition(definition))
    {
        return false;
    }

    if (!observed.reset(definition.geometry))
    {
        return false;
    }

    start = definition.start;
    goal = definition.goal;
    observed_to_programmed.offset_x = 0;
    observed_to_programmed.offset_y = 0;
    observed_to_programmed.heading_deg10 = 0;
    observed_to_programmed.confidence_permille = 1000;
    observed_pose_packed = pos2int(static_cast<int8_t>(start.x), static_cast<int8_t>(start.y), GRID_POSE_UNKNOWN, GRID_POSE_UNKNOWN);
    programmed_pose_packed = pos2int(static_cast<int8_t>(start.x), static_cast<int8_t>(start.y), GRID_POSE_UNKNOWN, GRID_POSE_UNKNOWN);
    programmed_pose_valid = true;
    return true;
}

void MapBundle::clearObserved()
{
    observed.clear();
    observed_pose_packed = pos2int(unknownPackedGridPose());
    observed_to_programmed.offset_x = GRID_POSE_UNKNOWN;
    observed_to_programmed.offset_y = GRID_POSE_UNKNOWN;
    observed_to_programmed.heading_deg10 = 0;
    observed_to_programmed.confidence_permille = 0;
}

} // namespace navigation
