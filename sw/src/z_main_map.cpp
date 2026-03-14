#include <Arduino.h>

#include "navigation/grid_map.h"
#include "navigation/sample_maps.h"

using namespace navigation;

static MapBundle g_maps;

static void printPackedPose(const char *label, int32_t packed_pose)
{
    const PackedGridPose pose = int2pos(packed_pose);
    Serial.print(label);
    Serial.print(" packed=0x");
    Serial.print(static_cast<uint32_t>(packed_pose), HEX);
    Serial.print(" x=");
    Serial.print(pose.x);
    Serial.print(" y=");
    Serial.print(pose.y);
    Serial.print(" dir=");
    Serial.print(pose.direction);
    Serial.print(" speed=");
    Serial.println(pose.speed);
}

static void printCellSummary(const GridMap &map, CellCoord coord, const char *label)
{
    MapCell cell = map.getCellOrDefault(coord);
    Serial.print(label);
    Serial.print(" cell (");
    Serial.print(coord.x);
    Serial.print(",");
    Serial.print(coord.y);
    Serial.print(") flags=0x");
    Serial.print(cell.flags, HEX);
    Serial.print(" confidence=");
    Serial.println(cell.confidence);
}

void setup()
{
    Serial.begin(57600);
    delay(250);

    const ProgrammedMapDefinition &definition = getSampleLabyrinthMap();
    if (!g_maps.loadProgrammedMap(definition))
    {
        Serial.println("Failed to load programmed map");
        return;
    }

    CellCoord front_obstacle = {2, 1};
    g_maps.observed.setCell(front_obstacle, makeCell(CellKnown | CellObserved | CellBlocked, 220));
    g_maps.observed_pose_packed = pos2int(1, 1, 12, 3);

    Serial.println("Map subsystem ready");
    Serial.print("Programmed size: ");
    Serial.print(g_maps.programmed.geometry().width);
    Serial.print("x");
    Serial.print(g_maps.programmed.geometry().height);
    Serial.print(" cellSizeMm=");
    Serial.println(g_maps.programmed.geometry().cell_size_mm);
    Serial.print("Observed cellSizeMm=");
    Serial.println(g_maps.observed.cellSizeMm());

    printCellSummary(g_maps.programmed, g_maps.start, "Start");
    printCellSummary(g_maps.programmed, g_maps.goal, "Goal");
    printCellSummary(g_maps.observed, front_obstacle, "Observed obstacle");
    printPackedPose("Observed pose", g_maps.observed_pose_packed);
    printPackedPose("Programmed pose", g_maps.programmed_pose_packed);
}

void loop()
{
    delay(1000);
}
