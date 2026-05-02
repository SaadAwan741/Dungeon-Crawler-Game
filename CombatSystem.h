#pragma once
#include <random>
#include <string>
#include <vector>
#include "Entity.h"
using namespace std;

// CombatSystem
// - Checks range and line-of-sight before allowing attacks
// - Applies damage and maintains a small text log for UI

class CombatSystem 
{
    int rollDie(int sides) const;
    mutable mt19937 m_rng;

public:
    CombatSystem() : m_rng(random_device{}()) 
    {}
    vector<string> log;

    // Only allow attacks if target is in range AND line-of-sight is clear.
    bool canAttack(const Entity& attacker, const Entity& target, const Grid& grid, int range) const;

    // Applies damage + logs message (does not perform range/LOS checks).
    void attack(Entity& attacker, Entity& target);

};
