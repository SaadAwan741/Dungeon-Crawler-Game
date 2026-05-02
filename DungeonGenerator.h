#pragma once
#include <random>
#include <vector>
#include "Grid.h"
#include "Room.h"
using namespace std;

// DungeonGenerator
// - Map creation via BSP (Binary Space Partitioning)
// - One room per BSP leaf (random size within limits)
// - Room connection via MST (Kruskal) over room centers
// - Corridors are L-shaped (horizontal + vertical) carved into the grid
class DungeonGenerator
{
    struct Rect 
    {
        int x = 0, y = 0, w = 0, h = 0;
    };
    struct BSPNode
    {
        Rect region;
        unique_ptr<BSPNode> left;
        unique_ptr<BSPNode> right;
        bool isLeaf() const { return !left && !right; }
    };
    // Generation steps
    void fillWalls();
    void buildBSP();
    void createRoomsFromLeaves();
    void connectRoomsMST();
    void placeStairs();

    // BSP helpers
    void splitNode(BSPNode& node, int depth);
    void collectLeaves(BSPNode& node, vector<BSPNode*>& outLeaves);

    // Carving helpers
    void carveRoom(const Room& r);
    void carveH(int x1, int x2, int y);
    void carveV(int x, int y1, int y2);
    void carveLCorridor(Point a, Point b);

    // Random helpers
    int randInt(int lo, int hi);
    bool chance(int numerator, int denominator);
     
    Grid m_grid;
    vector<Room> m_rooms;
    mt19937 m_rng;
    unique_ptr<BSPNode> m_root;

    // Tunables
    int m_maxDepth = 5;
    int m_minLeafSize = 10;
    int m_minRoomW = 5;
    int m_minRoomH = 4;
    int m_maxRoomW = 12;
    int m_maxRoomH = 9;
    int m_roomPadding = 1;

public:
    static constexpr int MAP_W = 60;
    static constexpr int MAP_H = 40;

    DungeonGenerator();
    // Regenerate dungeon. Leaves the object in a valid, connected state.
    void generate();

    const Grid& grid() const 
    {
        return m_grid;
    }
    Grid& grid()
    {
        return m_grid;
    }
    const vector<Room>& rooms() const
    {
        return m_rooms;
    }
};
