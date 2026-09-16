#pragma once

#include <SFML/Graphics.hpp>

#include "FallingBall.h"

class FallingBall;

class Pin {
private:
    double radius;
    sf::Vector2<double> position;

public:
    Pin(sf::Vector2<double> pos, double r) : position(pos), radius(r) {}

    bool checkCollision(const FallingBall& ball) const;

    double getRadius() const;
    sf::Vector2<double> getPosition() const;
};