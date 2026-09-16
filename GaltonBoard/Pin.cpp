#include "Pin.h"

bool Pin::checkCollision(const FallingBall& ball) const {
    sf::Vector2<double> ballPos = ball.getPosition();

    double dx = position.x - ballPos.x;
    double dy = position.y - ballPos.y;

    double ballRadius = ball.getRadius();
    return (dx * dx + dy * dy) <= (radius + ballRadius) * (radius + ballRadius);
}

double Pin::getRadius() const { return radius; }
sf::Vector2<double> Pin::getPosition() const { return position; }
