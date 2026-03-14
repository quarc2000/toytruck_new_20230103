#include "navigation/sample_maps.h"

namespace navigation
{

static const uint16_t P = CellKnown | CellProgrammed;
static const uint16_t B = CellKnown | CellProgrammed | CellBlocked;

static const MapCell SAMPLE_LABYRINTH_CELLS[] = {
    {B, 255, 0}, {B, 255, 0}, {B, 255, 0}, {B, 255, 0}, {B, 255, 0}, {B, 255, 0}, {B, 255, 0}, {B, 255, 0},
    {B, 255, 0}, {P, 255, 0}, {P, 255, 0}, {P, 255, 0}, {B, 255, 0}, {P, 255, 0}, {P, 255, 0}, {B, 255, 0},
    {B, 255, 0}, {P, 255, 0}, {B, 255, 0}, {P, 255, 0}, {B, 255, 0}, {P, 255, 0}, {B, 255, 0}, {B, 255, 0},
    {B, 255, 0}, {P, 255, 0}, {B, 255, 0}, {P, 255, 0}, {P, 255, 0}, {P, 255, 0}, {B, 255, 0}, {B, 255, 0},
    {B, 255, 0}, {P, 255, 0}, {B, 255, 0}, {B, 255, 0}, {B, 255, 0}, {P, 255, 0}, {P, 255, 0}, {B, 255, 0},
    {B, 255, 0}, {P, 255, 0}, {P, 255, 0}, {P, 255, 0}, {B, 255, 0}, {P, 255, 0}, {B, 255, 0}, {B, 255, 0},
    {B, 255, 0}, {B, 255, 0}, {B, 255, 0}, {P, 255, 0}, {P, 255, 0}, {P, 255, 0}, {P, 255, 0}, {B, 255, 0},
    {B, 255, 0}, {B, 255, 0}, {B, 255, 0}, {B, 255, 0}, {B, 255, 0}, {B, 255, 0}, {B, 255, 0}, {B, 255, 0},
};

static const ProgrammedMapDefinition SAMPLE_LABYRINTH_MAP = {
    MAP_FORMAT_VERSION,
    {8, 8, 100, 0, 0},
    {1, 1},
    {6, 6},
    SAMPLE_LABYRINTH_CELLS};

const ProgrammedMapDefinition &getSampleLabyrinthMap()
{
    return SAMPLE_LABYRINTH_MAP;
}

} // namespace navigation
