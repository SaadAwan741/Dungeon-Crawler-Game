#include "PathFinder.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
#include <unordered_map>

namespace
{
struct OpenItem 
{
    int x = 0;
    int y = 0;
    float f = 0.0f;
    float g = 0.0f;
};

struct ByLowestF
{
    bool operator()(const OpenItem& a, const OpenItem& b) const 
    {
        return a.f > b.f; // priority_queue is max-heap by default
    }
};

int manhattan(const Node& a, const Node& b) 
{
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

int encode(int x, int y, int w) 
{
    return y * w + x;
}

vector<Node> reconstruct(const unordered_map<int, int>& cameFrom,int startEnc,int goalEnc,int w) 
{
    vector<Node> path;
    int cur = goalEnc;
    while (cur != startEnc) 
    {
        path.push_back(Node{ cur % w, cur / w });
        auto it = cameFrom.find(cur);
        if (it == cameFrom.end()) 
            break; // no parent: unreachable / corrupted
        cur = it->second;
    }
    reverse(path.begin(), path.end());
    return path;
}
} // namespace

//A*
vector<Node> PathFinder::findPath(const Grid& grid, Node start, Node goal)
{
    if (!grid.inBounds(start.x, start.y) || !grid.inBounds(goal.x, goal.y))
        return {};
    if (grid.at(start.x, start.y).type == TileType::WALL)
        return {};
    if (grid.at(goal.x, goal.y).type == TileType::WALL) 
        return {};
    if (start == goal)
        return {};

    const int w = grid.width;

    priority_queue<OpenItem, vector<OpenItem>, ByLowestF> open;
    unordered_map<int, int> cameFrom;
    unordered_map<int, float> gScore;

    const int startEnc = encode(start.x, start.y, w);
    const int goalEnc = encode(goal.x, goal.y, w);

    gScore[startEnc] = 0.0f;
    open.push(OpenItem{ start.x, start.y, (float)manhattan(start, goal), 0.0f });

    const int dx[4] = { 1, -1, 0, 0 };
    const int dy[4] = { 0, 0, 1, -1 };

    while (!open.empty()) 
    {
        const auto cur = open.top();
        open.pop();
        const int curEnc = encode(cur.x, cur.y, w);

        if (curEnc == goalEnc) 
        {
            return reconstruct(cameFrom, startEnc, goalEnc, w);
        }

        for (int dir = 0; dir < 4; dir++)
        {
            const int nx = cur.x + dx[dir];
            const int ny = cur.y + dy[dir];
            if (!grid.inBounds(nx, ny))
                continue;
            if (grid.at(nx, ny).type == TileType::WALL) 
                continue;

            const int nEnc = encode(nx, ny, w);
            const float tentativeG = gScore[curEnc] + 1.0f;

            auto it = gScore.find(nEnc);
            if (it == gScore.end() || tentativeG < it->second)
            {
                gScore[nEnc] = tentativeG;
                cameFrom[nEnc] = curEnc;
                const float f = tentativeG + (float)manhattan(Node{ nx, ny }, goal);
                open.push(OpenItem{ nx, ny, f, tentativeG });
            }
        }
    }

    return {}; // no path
}

