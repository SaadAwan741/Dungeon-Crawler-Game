#pragma once

enum class TileType 
{
    WALL,
    FLOOR,
    DOOR,
    STAIRS_DOWN,
    STAIRS_UP
};

struct Tile 
{
    TileType type = TileType::WALL;
    bool visited = false;
    bool visible = false;  // for fog of war
};
