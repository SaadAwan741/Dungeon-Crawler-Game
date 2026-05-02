#pragma once
#include <string>
#include <vector>
#include "Grid.h"
using namespace std;

class Entity
{
protected:
    static int clampMinDamage(int dmgMinusDefense);
public:
    string name;
    Point pos{};
    int hp = 1;
    int maxHp = 1;
    int attack = 1;
    int defense = 0;
    bool alive = true;

    Entity(string n, Point p, int hp, int atk, int def);
    virtual ~Entity() = default;

    void takeDamage(int dmg);

    // Simple movement primitive (does not pathfind).
    bool tryMove(Point delta, const Grid& grid);
};

class Player : public Entity
{
public:
    int level = 1;
    int xp = 0;
    int xpToNext = 20;
    int gold = 0;

    explicit Player(Point p);
    void gainXP(int amount);
};
 
//  Enemy types
enum class EnemyType { GOBLIN, SKELETON, TROLL };

class Enemy : public Entity
{
public:
    enum class AggroState { Idle, Chase };

    EnemyType type = EnemyType::GOBLIN;
    int xpReward = 0;
    int aggroRange = 10;
    int pathRecalcTicks = 3;
    int ticksUntilRecalc = 0;

    Enemy(EnemyType t, Point p);

    // Enemy AI:
    // - If player visible -> chase with A*
    // - Else -> idle (or simple patrol hook)
    void updateAI(const Grid& grid, const Player& player);

    // Debug/telemetry helpers (used by the debug UI).
    AggroState aggroState() const
    {
        return m_aggroState;
    }
    bool hasValidPathToPlayer() const 
    {
        return m_hasValidPath;
    }

private:
    vector<Point> m_cachedPath;
    AggroState m_aggroState = AggroState::Idle;
    bool m_hasValidPath = false;

    static string typeName(EnemyType t) 
    {
        switch (t) 
        {
        case EnemyType::GOBLIN:   return "Goblin";
        case EnemyType::SKELETON: return "Skeleton";
        case EnemyType::TROLL:    return "Troll";
        }
        return "?";
    }
    static int typeHp(EnemyType t)
    { 
        return t == EnemyType::TROLL ? 20 : t == EnemyType::SKELETON ? 10 : 8;
    }
    static int typeAtk(EnemyType t) 
    {
        return t == EnemyType::TROLL ? 6 : t == EnemyType::SKELETON ? 4 : 3; 
    }
    static int typeDef(EnemyType t) 
    {
        return t == EnemyType::TROLL ? 2 : t == EnemyType::SKELETON ? 1 : 0;
    }
    static int typeXP(EnemyType t) 
    {
        return t == EnemyType::TROLL ? 15 : t == EnemyType::SKELETON ? 8 : 5; 
    }
};
