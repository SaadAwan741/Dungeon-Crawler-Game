#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <algorithm>
#include <iostream>
#include <string> 
#include <vector>
#include <cmath>
using namespace std;

#include "CombatSystem.h"
#include "DungeonGenerator.h"
#include "Entity.h"
#include "VisibilitySystem.h"

// ─────────────────────────────────────────
//  Constants
// ─────────────────────────────────────────
static constexpr int TILE_SIZE = 20;
static constexpr int SIDEBAR_W = 220;
static constexpr int WINDOW_W = DungeonGenerator::MAP_W * TILE_SIZE + SIDEBAR_W;
static constexpr int WINDOW_H = DungeonGenerator::MAP_H * TILE_SIZE;

// Colors
static const sf::Color C_WALL(30, 20, 40);
static const sf::Color C_FLOOR(60, 55, 80);
static const sf::Color C_FLOOR_VIS(80, 75, 100);
static const sf::Color C_PLAYER(100, 220, 255);
static const sf::Color C_GOBLIN(100, 220, 80);
static const sf::Color C_SKELETON(220, 210, 190);
static const sf::Color C_TROLL(180, 80, 80);
static const sf::Color C_STAIRS(255, 215, 0);
static const sf::Color C_FOG(10, 8, 15);
static const sf::Color C_SIDEBAR_BG(20, 15, 30);
static const sf::Color C_TEXT(200, 195, 220);
static const sf::Color C_HP_BAR(200, 60, 60);
static const sf::Color C_XP_BAR(80, 160, 255);

static void updateFOV(Grid& grid, Point playerPos, int radius = 8) {
    // Reset current visibility
    for (int y = 0; y < grid.height; y++)
        for (int x = 0; x < grid.width; x++)
            grid.at(x, y).visible = false;

    // Reveal tiles within radius, but only if line-of-sight is clear.
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            if (dx * dx + dy * dy > radius * radius) continue;
            const int nx = playerPos.x + dx;
            const int ny = playerPos.y + dy;
            if (!grid.inBounds(nx, ny)) continue;

            if (VisibilitySystem::hasLineOfSight(playerPos, Point{ nx, ny }, grid)) {
                grid.at(nx, ny).visible = true;
                grid.at(nx, ny).visited = true;
            }
        }
    }
}

static void drawBar(sf::RenderWindow& win, float x, float y, float w, float h,
                    float ratio, sf::Color fill, sf::Color bg) {
    sf::RectangleShape bg_rect({ w, h });
    bg_rect.setPosition(x, y);
    bg_rect.setFillColor(bg);
    win.draw(bg_rect);

    sf::RectangleShape fill_rect({ w * std::max(0.0f, std::min(1.0f, ratio)), h });
    fill_rect.setPosition(x, y);
    fill_rect.setFillColor(fill);
    win.draw(fill_rect);
}

static sf::Color enemyColor(const Enemy& e) {
    return (e.type == EnemyType::GOBLIN)   ? C_GOBLIN
         : (e.type == EnemyType::SKELETON) ? C_SKELETON
                                           : C_TROLL;
}

static int manhattanDist(Point a, Point b) {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

int main() {
    sf::RenderWindow window(sf::VideoMode(WINDOW_W, WINDOW_H), "Dungeon Crawler");
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.loadFromFile("C:/Windows/Fonts/arial.ttf")) {
        if (!font.loadFromFile("C:/Windows/Fonts/consola.ttf")) {
            std::cerr << "Could not load font!\n";
            return 1;
        }
    }

    DungeonGenerator gen;
    Player player(Point{ gen.rooms().front().center().x, gen.rooms().front().center().y });

    // Spawn enemies: 1-2 per room (skip first room = spawn room)
    vector<Enemy> enemies;
    for (int i = 1; i < (int)gen.rooms().size(); i++) {
        const int roll = rand() % 3;
        const EnemyType type = (roll == 0) ? EnemyType::GOBLIN
                             : (roll == 1) ? EnemyType::SKELETON
                                           : EnemyType::TROLL;
        auto c = gen.rooms()[i].center();
        enemies.emplace_back(type, Point{ c.x, c.y });
        if (rand() % 2 == 0 && gen.rooms()[i].width > 6)
            enemies.emplace_back(EnemyType::GOBLIN, Point{ c.x + 1, c.y });
    }

    CombatSystem combat;
    vector<string> messageLog;
    messageLog.push_back("Welcome, adventurer!");
    messageLog.push_back("WASD / Arrows to move");
    messageLog.push_back("Walk into enemies to melee");

    int lastXPGained = 0;
    int floor = 1;
    bool gameOver = false;
    string gameOverMsg;

    // Visual cue for "who attacked".
    int attackCueFrames = 0;
    bool attackCueFromEnemy = false;
    Point attackCueAttackerPos{};
    Point attackCueTargetPos{};

    updateFOV(gen.grid(), player.pos);

    sf::RectangleShape tileShape({ (float)TILE_SIZE - 1, (float)TILE_SIZE - 1 });
    sf::CircleShape entityDot(TILE_SIZE * 0.35f);
    entityDot.setOrigin(TILE_SIZE * 0.35f, TILE_SIZE * 0.35f);

    while (window.isOpen()) {
        // ─── Events ───
        sf::Event event;
        while (window.pollEvent(event)) 
        {
            if (event.type == sf::Event::Closed)
                window.close();
            if (event.type == sf::Event::KeyPressed && !gameOver)
            {
                Point move{ 0, 0 };
                if (event.key.code == sf::Keyboard::W || event.key.code == sf::Keyboard::Up) move.y = -1;
                if (event.key.code == sf::Keyboard::S || event.key.code == sf::Keyboard::Down) move.y = 1;
                if (event.key.code == sf::Keyboard::A || event.key.code == sf::Keyboard::Left) move.x = -1;
                if (event.key.code == sf::Keyboard::D || event.key.code == sf::Keyboard::Right) move.x = 1;

                if (move.x == 0 && move.y == 0) continue;
                const Point newPos{ player.pos.x + move.x, player.pos.y + move.y };
                if (!gen.grid().inBounds(newPos.x, newPos.y)) continue;
                if (gen.grid().at(newPos.x, newPos.y).type == TileType::WALL) continue;

                // If stepping onto an enemy -> melee attack (range 1) if LOS is clear.
                bool attacked = false;
                for (auto& e : enemies)
                {
                    if (e.alive && e.pos == newPos) 
                    {
                        combat.log.clear();
                        if (combat.canAttack(player, e, gen.grid(), 1)) 
                        {
                            combat.attack(player, e);
                            attackCueFrames = 14;
                            attackCueFromEnemy = false;
                            attackCueAttackerPos = player.pos;
                            attackCueTargetPos = e.pos;
                            if (!e.alive)
                            {
                                player.gainXP(e.xpReward);
                                lastXPGained = e.xpReward;
                            }
                        } 
                        else 
                        {
                            combat.log.push_back("No clear line-of-sight!");
                        }

                        for (auto& msg : combat.log) 
                            messageLog.push_back(msg);
                        while ((int)messageLog.size() > 8)
                            messageLog.erase(messageLog.begin());
                        attacked = true;
                        break;
                    }
                }

                if (!attacked) 
                    player.pos = newPos;

                // Check stairs
                if (gen.grid().at(player.pos.x, player.pos.y).type == TileType::STAIRS_DOWN) 
                {
                    floor++;
                    gen.generate();
                    player.pos = Point{ gen.rooms().front().center().x, gen.rooms().front().center().y };
                    lastXPGained = 0;

                    enemies.clear();
                    for (int i = 1; i < (int)gen.rooms().size(); i++) 
                    {
                        const int roll = rand() % 3;
                        const EnemyType type = (floor >= 3 && roll == 2) ? EnemyType::TROLL
                                             : (roll == 1) ? EnemyType::SKELETON
                                                           : EnemyType::GOBLIN;
                        auto c = gen.rooms()[i].center();
                        enemies.emplace_back(type, Point{ c.x, c.y });
                    }

                    messageLog.push_back("Floor " + std::to_string(floor) + "!");
                    while ((int)messageLog.size() > 8) messageLog.erase(messageLog.begin());
                }

                updateFOV(gen.grid(), player.pos);

                // ── Enemy turn ──
                for (auto& e : enemies) 
                {
                    if (!e.alive) 
                        continue;

                    // If adjacent and LOS -> attack, else chase (vision-based).
                    if (combat.canAttack(e, player, gen.grid(), 1)) 
                    {
                        combat.log.clear();
                        combat.attack(e, player);
                        attackCueFrames = 14;
                        attackCueFromEnemy = true;
                        attackCueAttackerPos = e.pos;
                        attackCueTargetPos = player.pos;
                        for (auto& msg : combat.log) messageLog.push_back(msg);
                        while ((int)messageLog.size() > 8) messageLog.erase(messageLog.begin());
                    } else {
                        // Avoid updating if enemy is in unexplored fog (simple perf/behavior choice).
                        if (!gen.grid().at(e.pos.x, e.pos.y).visible) continue;
                        e.updateAI(gen.grid(), player);
                    }
                }

                if (!player.alive) {
                    gameOver = true;
                    gameOverMsg = "YOU DIED  -  Floor: " + std::to_string(floor);
                }

                updateFOV(gen.grid(), player.pos);
            }
        }

        // ─── Draw ───
        window.clear(C_FOG);

        // Sidebar background
        sf::RectangleShape sidebar({ (float)SIDEBAR_W, (float)WINDOW_H });
        sidebar.setPosition(DungeonGenerator::MAP_W * TILE_SIZE, 0);
        sidebar.setFillColor(C_SIDEBAR_BG);
        window.draw(sidebar);

        // Draw tiles
        for (int x = 0; x < DungeonGenerator::MAP_W; x++) 
        {
            for (int y = 0; y < DungeonGenerator::MAP_H; y++) 
            {
                auto& tile = gen.grid().at(x, y);
                tileShape.setPosition((float)x * TILE_SIZE, (float)y * TILE_SIZE);

                if (!tile.visited) 
                {
                    tileShape.setFillColor(C_FOG);
                } 
                else if (tile.type == TileType::WALL)
                {
                    tileShape.setFillColor(tile.visible ? sf::Color(50, 40, 65) : sf::Color(25, 20, 35));
                }
                else if (tile.type == TileType::STAIRS_DOWN) 
                {
                    tileShape.setFillColor(C_STAIRS);
                } 
                else
                {
                    tileShape.setFillColor(tile.visible ? C_FLOOR_VIS : C_FLOOR);
                }
                window.draw(tileShape);
            }
        }

        // Draw enemies
        for (auto& e : enemies) 
        {
            if (!e.alive) 
                continue;
            if (!gen.grid().at(e.pos.x, e.pos.y).visible) 
                continue;

            // Brief highlight on the enemy that just attacked.
            sf::Color col = enemyColor(e);
            if (attackCueFrames > 0 && attackCueFromEnemy && e.pos == attackCueAttackerPos) {
                col = sf::Color(255, 120, 120);
            }
            // Brief highlight on the enemy the player just hit.
            if (attackCueFrames > 0 && !attackCueFromEnemy && e.pos == attackCueTargetPos) {
                col = sf::Color(90, 170, 255);
            }
            entityDot.setFillColor(col);
            entityDot.setPosition(e.pos.x * TILE_SIZE + TILE_SIZE * 0.5f,
                                  e.pos.y * TILE_SIZE + TILE_SIZE * 0.5f);
            window.draw(entityDot);

            // Mini HP bar above enemy
            const float hpRatio = (float)e.hp / (float)e.maxHp;
            drawBar(window,
                    e.pos.x * TILE_SIZE, e.pos.y * TILE_SIZE - 4,
                    TILE_SIZE - 1, 3, hpRatio,
                    sf::Color(220, 60, 60), sf::Color(60, 20, 20));
        }

        // Draw player
        // Brief flash when player is hit.
        if (attackCueFrames > 0 && attackCueFromEnemy) entityDot.setFillColor(sf::Color(255, 80, 80));
        else entityDot.setFillColor(C_PLAYER);
        entityDot.setPosition(player.pos.x * TILE_SIZE + TILE_SIZE * 0.5f,
                              player.pos.y * TILE_SIZE + TILE_SIZE * 0.5f);
        window.draw(entityDot);

        // Draw a short-lived attack line (attacker -> target).
        if (attackCueFrames > 0) {
            const sf::Color lineCol = attackCueFromEnemy ? sf::Color(255, 90, 90, 200) : sf::Color(90, 170, 255, 200);
            const sf::Vector2f a((attackCueAttackerPos.x + 0.5f) * TILE_SIZE, (attackCueAttackerPos.y + 0.5f) * TILE_SIZE);
            const sf::Vector2f b((attackCueTargetPos.x + 0.5f) * TILE_SIZE, (attackCueTargetPos.y + 0.5f) * TILE_SIZE);
            sf::Vertex line[] = { sf::Vertex(a, lineCol), sf::Vertex(b, lineCol) };
            window.draw(line, 2, sf::Lines);
        }

        // ── Sidebar UI ──
        float sx = DungeonGenerator::MAP_W * TILE_SIZE + 12.f;
        float sy = 14.f;

        auto drawText = [&](const std::string& str, float x, float y,
                            int size = 13, sf::Color c = C_TEXT) {
            sf::Text t(str, font, size);
            t.setPosition(x, y);
            t.setFillColor(c);
            window.draw(t);
        };

        drawText("DUNGEON CRAWLER", sx, sy, 14, sf::Color(180, 140, 255));
        drawText("Floor: " + std::to_string(floor), sx, sy + 20, 12, sf::Color(255, 215, 0));
        sy += 48;

        drawText(player.name, sx, sy, 13, C_PLAYER);
        drawText("Lv." + std::to_string(player.level), sx + 80, sy, 13, sf::Color(200, 200, 100));
        sy += 20;

        drawText("HP: " + std::to_string(player.hp) + "/" + std::to_string(player.maxHp), sx, sy, 11);
        sy += 14;
        drawBar(window, sx, sy, SIDEBAR_W - 24, 10,
                (float)player.hp / (float)player.maxHp, C_HP_BAR, sf::Color(60, 20, 20));
        sy += 18;

        drawText("XP: " + std::to_string(player.xp) + "/" + std::to_string(player.xpToNext), sx, sy, 11);
        sy += 14;
        drawBar(window, sx, sy, SIDEBAR_W - 24, 10,
                (float)player.xp / (float)player.xpToNext, C_XP_BAR, sf::Color(20, 30, 60));
        sy += 18;

        drawText("Last XP: +" + std::to_string(lastXPGained), sx, sy, 11, sf::Color(160, 190, 255));
        sy += 18;

        drawText("ATK: " + std::to_string(player.attack) +
                 "  DEF: " + std::to_string(player.defense), sx, sy, 11);
        sy += 22;

        int aliveCount = 0;
        for (auto& e : enemies) if (e.alive) aliveCount++;
        drawText("Enemies: " + std::to_string(aliveCount), sx, sy, 11, sf::Color(255, 120, 120));
        sy += 28;

        // ── Debug Enemy Tracking ──
        // Find nearest alive enemy (Manhattan distance).
        Enemy* nearest = nullptr;
        int nearestDist = 0;
        for (auto& e : enemies) 
        {
            if (!e.alive)
                continue;
            const int d = manhattanDist(player.pos, e.pos);
            if (!nearest || d < nearestDist) 
            {
                nearest = &e;
                nearestDist = d;
            }
        }

        drawText("Player Pos: (" + std::to_string(player.pos.x) + "," + std::to_string(player.pos.y) + ")", sx, sy, 11);
        sy += 14;
        if (nearest) 
        {
            drawText("Nearest: " + nearest->name + " (" + std::to_string(nearest->pos.x) + "," + std::to_string(nearest->pos.y) + ")", sx, sy, 11);
            sy += 14;
            drawText("Dist (Manhattan): " + std::to_string(nearestDist), sx, sy, 11);
            sy += 14;

            const bool chase = nearest->aggroState() == Enemy::AggroState::Chase;
            drawText(std::string("Aggro: ") + (chase ? "Chase" : "Idle"), sx, sy, 11, chase ? sf::Color(255, 170, 120) : sf::Color(160, 160, 170));
            sy += 14;

            if (chase)
            {
                drawText(std::string("A* Path: ") + (nearest->hasValidPathToPlayer() ? "Valid" : "None"), sx, sy, 11,
                         nearest->hasValidPathToPlayer() ? sf::Color(170, 255, 170) : sf::Color(255, 140, 140));
                sy += 14;
            }
        } 
        else
        {
            drawText("Nearest: (none)", sx, sy, 11);
            sy += 14;
        }

        sf::RectangleShape div({ (float)SIDEBAR_W - 24, 1 });
        div.setPosition(sx, sy);
        div.setFillColor(sf::Color(80, 70, 100));
        window.draw(div);
        sy += 8;

        drawText("Combat Log", sx, sy, 11, sf::Color(140, 130, 160));
        sy += 18;
        for (auto& msg : messageLog)
            drawText(msg, sx, sy += 14, 10, sf::Color(180, 175, 200));

        if (gameOver) 
        {
            sf::RectangleShape overlay({ (float)WINDOW_W, (float)WINDOW_H });
            overlay.setFillColor(sf::Color(0, 0, 0, 160));
            window.draw(overlay);
            drawText(gameOverMsg, WINDOW_W / 2 - 120, WINDOW_H / 2 - 20, 18, sf::Color(255, 80, 80));
            drawText("Close window to exit.", WINDOW_W / 2 - 90, WINDOW_H / 2 + 10, 13);
        }

        if (attackCueFrames > 0) attackCueFrames--;
        window.display();
    }

    return 0;
}

