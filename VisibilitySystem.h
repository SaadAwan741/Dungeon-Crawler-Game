#pragma once
#include "Grid.h"

// VisibilitySystem
// Line-of-sight using Bresenham's line algorithm.
// Stops when a wall is hit (walls block vision).

class VisibilitySystem
{
public:
    static bool hasLineOfSight(Point a, Point b, const Grid& grid);
};

