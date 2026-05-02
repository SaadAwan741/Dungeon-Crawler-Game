#include "CombatSystem.h"
#include "VisibilitySystem.h"
#include <algorithm>
#include <cmath>
using namespace std;
namespace
{
    int manhattan(Point a, Point b) 
    {
        return abs(a.x - b.x) + abs(a.y - b.y);
    }
}  

bool CombatSystem::canAttack(const Entity& attacker,const Entity& target,const Grid& grid, int range) const
{
    if (!attacker.alive || !target.alive) 
        return false;
    if (manhattan(attacker.pos, target.pos) > range)
        return false;

    return VisibilitySystem::hasLineOfSight(attacker.pos, target.pos, grid);
}

void CombatSystem::attack(Entity& attacker, Entity& target)
{
    if (!attacker.alive || !target.alive)
        return;
    const int dmg = attacker.attack + rollDie(6);
    target.takeDamage(dmg);
    log.push_back(attacker.name + " hits " + target.name + " for " + to_string(dmg) + " dmg!");
    if (!target.alive) 
        log.push_back(target.name + " defeated!");
}

int CombatSystem::rollDie(int sides) const 
{
    uniform_int_distribution<int> dist(1, sides);
    return dist(m_rng);
}

