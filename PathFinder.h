#pragma once
#include <vector>
#include "Grid.h"
using namespace std;

// PathFinder
// A* pathfinding on a grid with 4-directional movement.
// - Open list: min-heap by f = g + h
// - Heuristic: Manhattan distance
// - Walls are impassable

struct Node
{
    int x = 0;
    int y = 0;

    bool operator==(const Node& o) const
    { 
        return x == o.x && y == o.y;
    }
};

class PathFinder
{
public:
    // Returns a list of nodes from start to goal (inclusive of goal, exclusive of start).
    static vector<Node> findPath(const Grid& grid, Node start, Node goal);
};
