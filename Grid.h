#pragma once
#include <vector>
#include "Tile.h"
using namespace std;

// Shared grid type used by generation, pathfinding, visibility, and combat.
// Keeps algorithms decoupled from SFML types.

struct Point
{
    int x = 0;
    int y = 0;

    bool operator==(const Point& o) const
    { 
        return x == o.x && y == o.y;
    }
    bool operator!=(const Point& o) const
    {
        return !(*this == o);
    }
};

struct Grid
{
    int width = 0;
    int height = 0;
    vector<Tile> tiles; // row-major: y * width + x

    Grid() = default;
    Grid(int w, int h) : width(w), height(h), tiles((size_t)w * (size_t)h) {}

    bool inBounds(int x, int y) const 
    {
        return x >= 0 && x < width && y >= 0 && y < height;
    }
    Tile& at(int x, int y)
    { 
        return tiles[(size_t)y * (size_t)width + (size_t)x]; //row major 2D->1D
    }
    const Tile& at(int x, int y) const
    { 
        return tiles[(size_t)y * (size_t)width + (size_t)x];//row major 2D->1D
    }
};

