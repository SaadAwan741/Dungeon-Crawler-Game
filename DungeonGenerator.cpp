#include "DungeonGenerator.h"
#include <algorithm>
#include <numeric>
using namespace std;
namespace {
struct Edge {
    int a = 0;
    int b = 0;
    int w = 0;
};

struct DSU {
    vector<int> p;
    vector<int> r;

    explicit DSU(int n) : p(n), r(n, 0) 
    {
        iota(p.begin(), p.end(), 0);
    }

    int find(int x) {
        if (p[x] == x) 
            return x;
        p[x] = find(p[x]);
        return p[x];
    }

    bool unite(int a, int b) {
        a = find(a);
        b = find(b);
        if (a == b) 
            return false;
        if (r[a] < r[b]) 
            swap(a, b);
        p[b] = a;
        if (r[a] == r[b]) 
            r[a]++;

        return true;
    }
};

int manhattan(Point a, Point b) {
    return abs(a.x - b.x) + abs(a.y - b.y);
}
} // namespace

DungeonGenerator::DungeonGenerator()
    : m_grid(MAP_W, MAP_H), m_rng(random_device{}()) {
    generate();
}

int DungeonGenerator::randInt(int lo, int hi) {
    uniform_int_distribution<int> dist(lo, hi);
    return dist(m_rng);
}

bool DungeonGenerator::chance(int numerator, int denominator) {
    return randInt(1, denominator) <= numerator;
}

void DungeonGenerator::fillWalls() 
{
    for (int y = 0; y < m_grid.height; y++) 
    {
        for (int x = 0; x < m_grid.width; x++)
        {
            auto& t = m_grid.at(x, y);
            t.type = TileType::WALL;
            t.visible = false;
            t.visited = false;
        }
    }
}

void DungeonGenerator::generate() 
{
    fillWalls();
    m_rooms.clear();

    buildBSP();
    createRoomsFromLeaves();
    connectRoomsMST();
    placeStairs();
}

void DungeonGenerator::buildBSP() 
{
    m_root = make_unique<BSPNode>();
    m_root->region = { 1, 1, MAP_W - 2, MAP_H - 2 }; // keep a wall border
    splitNode(*m_root, 0);
}

void DungeonGenerator::splitNode(BSPNode& node, int depth) 
{
    if (depth >= m_maxDepth) 
        return;

    const int minSize = m_minLeafSize;
    const bool canSplitH = node.region.h >= minSize * 2;
    const bool canSplitV = node.region.w >= minSize * 2;
    if (!canSplitH && !canSplitV)
        return;

    // Bias split direction based on aspect ratio, but keep randomness.
    bool splitHoriz = false;
    if (canSplitH && canSplitV) 
    {
        if (node.region.w > node.region.h)
            splitHoriz = false;
        else if (node.region.h > node.region.w) 
            splitHoriz = true;
        else 
            splitHoriz = chance(1, 2);
    } 
    else
    {
        splitHoriz = canSplitH;
    }

    if (splitHoriz)
    {
        const int splitY = randInt(node.region.y + minSize, node.region.y + node.region.h - minSize);
        node.left = make_unique<BSPNode>();
        node.right = make_unique<BSPNode>();
        node.left->region = { node.region.x, node.region.y, node.region.w, splitY - node.region.y };
        node.right->region = { node.region.x, splitY, node.region.w, node.region.y + node.region.h - splitY };
    } 
    else
    {
        const int splitX = randInt(node.region.x + minSize, node.region.x + node.region.w - minSize);
        node.left = make_unique<BSPNode>();
        node.right = make_unique<BSPNode>();
        node.left->region = { node.region.x, node.region.y, splitX - node.region.x, node.region.h };
        node.right->region = { splitX, node.region.y, node.region.x + node.region.w - splitX, node.region.h };
    }

    splitNode(*node.left, depth + 1);
    splitNode(*node.right, depth + 1);
}

void DungeonGenerator::collectLeaves(BSPNode& node, vector<BSPNode*>& outLeaves)
{
    if (node.isLeaf()) 
    {
        outLeaves.push_back(&node);
        return;
    }
    if (node.left)
        collectLeaves(*node.left, outLeaves);
    if (node.right)
        collectLeaves(*node.right, outLeaves);
}

void DungeonGenerator::createRoomsFromLeaves() 
{
    if (!m_root) 
        return;

    vector<BSPNode*> leaves;
    collectLeaves(*m_root, leaves);

    for (auto* leaf : leaves)
    {
        // Shrink by padding so corridors can exist around rooms.
        const int rxMin = leaf->region.x + m_roomPadding;
        const int ryMin = leaf->region.y + m_roomPadding;
        const int rxMax = leaf->region.x + leaf->region.w - m_roomPadding;
        const int ryMax = leaf->region.y + leaf->region.h - m_roomPadding;

        const int maxW = min(m_maxRoomW, rxMax - rxMin);
        const int maxH = min(m_maxRoomH, ryMax - ryMin);

        if (maxW < m_minRoomW || maxH < m_minRoomH) 
            continue;

        const int rw = randInt(m_minRoomW, maxW);
        const int rh = randInt(m_minRoomH, maxH);
        const int rx = randInt(rxMin, rxMax - rw);
        const int ry = randInt(ryMin, ryMax - rh);

        Room r(rx, ry, rw, rh);
        carveRoom(r);
        m_rooms.push_back(r);
    }

    // Safety: ensure at least one room exists.
    if (m_rooms.empty())
    {
        Room r(2, 2, 10, 8);
        carveRoom(r);
        m_rooms.push_back(r);
    }
}

void DungeonGenerator::carveRoom(const Room& r) 
{
    for (int y = r.y; y < r.y + r.height; y++)
    {
        for (int x = r.x; x < r.x + r.width; x++)
        {
            if (m_grid.inBounds(x, y)) 
                m_grid.at(x, y).type = TileType::FLOOR;
        }
    }
}

void DungeonGenerator::carveH(int x1, int x2, int y)
{
    if (!m_grid.inBounds(0, y))
        return;
    const int a = min(x1, x2);
    const int b = max(x1, x2);
    for (int x = a; x <= b; x++) 
    {
        if (m_grid.inBounds(x, y)) 
            m_grid.at(x, y).type = TileType::FLOOR;
    }
}

void DungeonGenerator::carveV(int x, int y1, int y2) 
{
    if (!m_grid.inBounds(x, 0)) 
        return;
    const int a = min(y1, y2);
    const int b = max(y1, y2);
    for (int y = a; y <= b; y++)
    {
        if (m_grid.inBounds(x, y))
            m_grid.at(x, y).type = TileType::FLOOR;
    }
}

void DungeonGenerator::carveLCorridor(Point a, Point b) 
{
    if (chance(1, 2))
    {
        carveH(a.x, b.x, a.y);
        carveV(b.x, a.y, b.y);
    } 
    else 
    {
        carveV(a.x, a.y, b.y);
        carveH(a.x, b.x, b.y);
    }
}
//Kruskal's Algorithm
void DungeonGenerator::connectRoomsMST()
{
    const int n = (int)m_rooms.size();
    if (n <= 1) 
        return;

    vector<Point> centers;
    centers.reserve(m_rooms.size());
    for (const auto& r : m_rooms) 
    {
        auto c = r.center();
        centers.push_back({ c.x, c.y });
    }

    // Complete graph edges (OK for typical room counts). Weighted by Manhattan distance.
    vector<Edge> edges;
    edges.reserve((size_t)n * (size_t)(n - 1) / 2);
    for (int i = 0; i < n; i++)
    {
        for (int j = i + 1; j < n; j++)
        {
            edges.push_back({ i, j, manhattan(centers[i], centers[j]) });
        }
    }

    sort(edges.begin(), edges.end(), [](const Edge& e1, const Edge& e2) {
        return e1.w < e2.w;
    });

    DSU dsu(n);
    int used = 0;
    for (const auto& e : edges)
    {
        if (dsu.unite(e.a, e.b))
        {
            carveLCorridor(centers[e.a], centers[e.b]);
            used++;
            if (used == n - 1) 
                break;
        }
    }

    // Optional: add a few extra connections for loops (reduces dead-ends).
    for (const auto& e : edges)
    {
        if (chance(1, 18))
        {
            carveLCorridor(centers[e.a], centers[e.b]);
        }
    }
}

void DungeonGenerator::placeStairs()
{
    if (m_rooms.empty()) 
        return;
    auto c = m_rooms.back().center();
    if (m_grid.inBounds(c.x, c.y))
        m_grid.at(c.x, c.y).type = TileType::STAIRS_DOWN;
}
