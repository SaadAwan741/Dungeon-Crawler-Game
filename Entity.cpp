#include "Entity.h"
#include "PathFinder.h"
#include "VisibilitySystem.h"
#include <algorithm>
#include <cmath>
using namespace std;

namespace 
{
int distManhattan(Point a, Point b) {
    return abs(a.x - b.x) + abs(a.y - b.y);
}
} // namespace

Entity::Entity(string n, Point p, int hp_, int atk, int def)
    : name(move(n)), pos(p), hp(hp_), maxHp(hp_), attack(atk), defense(def) {}

int Entity::clampMinDamage(int dmgMinusDefense)
{
    return max(1, dmgMinusDefense);
}

void Entity::takeDamage(int dmg) 
{
    const int actual = clampMinDamage(dmg - defense);
    hp -= actual;
    if (hp <= 0) {
        hp = 0;
        alive = false;
    }
}

bool Entity::tryMove(Point delta, const Grid& grid) 
{
    const Point np{ pos.x + delta.x, pos.y + delta.y };
    if (!grid.inBounds(np.x, np.y)) 
        return false;
    if (grid.at(np.x, np.y).type == TileType::WALL) 
        return false;
    pos = np;
    return true;
}

Player::Player(Point p)
    : Entity("Hero", p, 30, 8, 2) {}

void Player::gainXP(int amount) 
{
    xp += amount;
    while (xp >= xpToNext) 
    {
        xp -= xpToNext;
        level++;
        xpToNext = level * 25;
        maxHp += 5;
        hp = maxHp;
        attack += 2;
        // Defense scaling: every 2 levels, gain +1 DEF.
        if (level % 2 == 0)
            defense += 1;
    }
}

Enemy::Enemy(EnemyType t, Point p)
    : Entity(typeName(t), p, typeHp(t), typeAtk(t), typeDef(t))
    , type(t)
    , xpReward(typeXP(t)) {}

void Enemy::updateAI(const Grid& grid, const Player& player)
{
    if (!alive)
        return;

    // Basic "vision-based aggro": if player is in range AND line-of-sight, chase.
    const bool inRange = distManhattan(pos, player.pos) <= aggroRange;
    const bool seesPlayer = inRange && VisibilitySystem::hasLineOfSight(pos, player.pos, grid);

    if (!seesPlayer)
    {
        // Idle / patrol hook:
        m_aggroState = AggroState::Idle;
        m_hasValidPath = false;
        m_cachedPath.clear();
        ticksUntilRecalc = 0;
        return;
    }

    m_aggroState = AggroState::Chase;

    // Recalculate cached path every N ticks.
    if (ticksUntilRecalc <= 0 || m_cachedPath.empty())
    {
        ticksUntilRecalc = pathRecalcTicks;
        const auto nodes = PathFinder::findPath(grid, Node{ pos.x, pos.y }, Node{ player.pos.x, player.pos.y });
        m_cachedPath.clear();
        m_cachedPath.reserve(nodes.size());
        for (const auto& n : nodes) 
            m_cachedPath.push_back(Point{ n.x, n.y });
        m_hasValidPath = !m_cachedPath.empty();
    } 
    else
    {
        ticksUntilRecalc--;
    }

    // Take one step along the path.
    if (!m_cachedPath.empty()) 
    {
        const Point next = m_cachedPath.front();
        m_cachedPath.erase(m_cachedPath.begin());
        pos = next;
    }
}

