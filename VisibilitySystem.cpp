#include "VisibilitySystem.h"
#include <cstdlib>
using namespace std;

bool VisibilitySystem::hasLineOfSight(Point a, Point b, const Grid& grid) 
{
    if (!grid.inBounds(a.x, a.y) || !grid.inBounds(b.x, b.y))
        return false;

    // Bresenham's line algorithm.
    int x0 = a.x;
    int y0 = a.y;
    const int x1 = b.x;
    const int y1 = b.y;

    const int dx = abs(x1 - x0);
    const int dy = abs(y1 - y0);
    const int sx = (x0 < x1) ? 1 : -1;
    const int sy = (y0 < y1) ? 1 : -1;

    int err = dx - dy;

    while (true) 
    {
        // Skip the starting tile, but block if we hit a wall before reaching target.
        if (!(x0 == a.x && y0 == a.y))
        {
            if (grid.at(x0, y0).type == TileType::WALL) 
                return false;
        }

        if (x0 == x1 && y0 == y1)
            return true;

        const int e2 = 2 * err;
        if (e2 > -dy)
        {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) 
        {
            err += dx;
            y0 += sy;
        }

        if (!grid.inBounds(x0, y0))
            return false;
    }
}

