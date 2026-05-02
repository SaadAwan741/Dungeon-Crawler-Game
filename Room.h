#pragma once
#include <SFML/Graphics.hpp>

struct Room
{
    int x, y, width, height;

    Room(int x, int y, int w, int h)
        : x(x), y(y), width(w), height(h)
    {}

    sf::Vector2i center() const 
    {
        return { x + width / 2, y + height / 2 };
    }

    bool intersects(const Room& other) const
    {
        return (x <= other.x + other.width &&
            x + width >= other.x &&
            y <= other.y + other.height &&
            y + height >= other.y);
    }
};
