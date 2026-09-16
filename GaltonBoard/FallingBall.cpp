#include <vector>

#include "FallingBall.h"
#include "Pin.h"

void FallingBall::update(const double dt) {
    velocity += g * dt;
    position += velocity * dt;

    if (position.x < -margin) {
        position.x = -margin;
        velocity.x *= -loseCoeff;
    }
    else if (position.x > margin) {
        position.x = margin;
        velocity.x *= -loseCoeff;
    }
}

void FallingBall::resolveCollision(const Pin& pin) {
    sf::Vector2<double> dir = position - pin.getPosition();
    double distance = std::hypot(dir.x, dir.y);
    sf::Vector2<double> normal = dir / distance;

    sf::Vector2<double> normalVelocity = -(velocity.x*normal.x + velocity.y*normal.y) * normal;
    velocity += 2.0 * normalVelocity * loseCoeff;

    double overlap = (radius + pin.getRadius()) - distance;
    position += normal * overlap;
}

double FallingBall::getRadius() const { return radius; }
sf::Vector2<double> FallingBall::getPosition() const { return position; }

bool FallingBall::getActivity() const { return active; };
void FallingBall::setActivity(bool value) { active = value; };