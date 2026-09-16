#pragma once

#include <atomic>
#include <vector>

#include <SFML/Graphics.hpp>

#include "Pin.h"

class Pin;

class FallingBall {
private:
    double radius;
    double loseCoeff;
    double margin;
    const sf::Vector2<double> g;
    std::atomic<bool> active;

    sf::Vector2<double> position;
    sf::Vector2<double> velocity;

public:
    FallingBall(sf::Vector2<double> InitPos, double r, double LOSE_COEFF, double PHYSICS_WIDTH) :
        position(InitPos), radius(r), loseCoeff(LOSE_COEFF), margin(PHYSICS_WIDTH / 2 - r),
        active(true), g(sf::Vector2<double>(0, 9810.0)) { };

    void update(const double dt);
    void resolveCollision(const Pin& pin);

    double getRadius() const;
    sf::Vector2<double> getPosition() const;

    bool getActivity() const;
    void setActivity(const bool value);
};