#include <algorithm>
#include <cmath>
#include <chrono>
#include <ctime>
#include <fstream>
#include <limits>
#include <list>
#include <memory>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include <SFML/Graphics.hpp>
#include <omp.h>

#include "FallingBall.h"
#include "Pin.h"

// Meta settings
const std::string RESULTS_FOLDER_PATH = "SimulationResults/";
#define DO_RENDER true;

// Screen settings
const sf::Color BACKGROUND_COLOR = sf::Color(54, 79, 107);
const sf::Color PIN_COLOR = sf::Color(63, 193, 201);
const sf::Color BALL_COLOR = sf::Color(252, 81, 133);

constexpr unsigned int INITIAL_SCREEN_WIDTH = 800;
constexpr unsigned int INITIAL_SCREEN_HEIGHT = 600;

// Galton board settings
constexpr int TOTAL_ROWS = 20,
              MAX_PINS_PER_ROW = 25;

constexpr double PIN_H_SPACING = 5.0,
                 PIN_V_SPACING = 5.0,
                 BORDER_MARGIN = 5.0,
                 TOP_MARGIN = 50.0,
                 HALF_GAP = 1.0,
                 PIN_RADIUS = .25;

// Physics settings
constexpr double BALL_RADIUS = .3,
                 LOSS_COEFF = 0.9,
                 FIXED_TIMESTEP = 0.001,
                 SIMULATION_SPEED = 0.1;

// Simulation limits
constexpr int TOTAL_BALLS = 1000000,
              MAX_ACTIVE_BALLS = 10000;

// Data types
using PinGrid = std::vector<std::vector<Pin>>;
using Vector2d = sf::Vector2<double>;

struct PhysicsWorld {
    double width = 0;
    double height = 0;
    Vector2d pinsOrigin;
    std::vector<double> pinBoundaries;
};

// Functions
void adjustView(sf::View& view, unsigned int screenWidth, unsigned int screenHeight,
    double physicsWidth, double physicsHeight) {
    const double physicsAspect = physicsWidth / physicsHeight;
    const double screenAspect = static_cast<double>(screenWidth) / screenHeight;

    if (screenAspect > physicsAspect) {
        const float viewHeight = static_cast<float>(physicsHeight);
        const float viewWidth = static_cast<float>(viewHeight * screenAspect);
        view.setSize(viewWidth, viewHeight);
    }
    else {
        const float viewWidth = static_cast<float>(physicsWidth);
        const float viewHeight = static_cast<float>(viewWidth / screenAspect);
        view.setSize(viewWidth, viewHeight);
    }

    view.setCenter(sf::Vector2f(0.f, static_cast<float>(physicsHeight) / 2.f));
}

sf::Vector2f toPhysicsCoordinates(const Vector2d& position,
    const Vector2d& origin) {
    return static_cast<sf::Vector2f>(position - origin);
}

PhysicsWorld createGaltonBoard(PinGrid& pins) {
    PhysicsWorld world;

    double minLeftBound = 0;
    bool isFirstRow = true;

    // Generate each row
    for (int row = 0; row < TOTAL_ROWS; ++row) {
        const double yPos = TOP_MARGIN + row * PIN_V_SPACING;
        const int pinsInRow = MAX_PINS_PER_ROW - (row % 2);
        const double leftBound = -((pinsInRow - 1) * PIN_H_SPACING) / 2;

        // For world sizes
        minLeftBound = std::min(leftBound, minLeftBound);

        // For collion check
        if (isFirstRow) {
            world.pinsOrigin = { leftBound - PIN_H_SPACING / 2, yPos - PIN_V_SPACING / 2 };
            isFirstRow = false;
        }

        std::vector<Pin> rowPins;
        rowPins.reserve(pinsInRow);

        // Generate each pin
        for (int i = 0; i < pinsInRow; ++i) {
            const double xPos = leftBound + i * PIN_H_SPACING;
            rowPins.emplace_back(Pin(Vector2d(xPos, yPos), PIN_RADIUS));

            // For balls counting
            if (row == TOTAL_ROWS - 1) world.pinBoundaries.push_back(xPos);
        }

        pins.push_back(std::move(rowPins));
    }

    world.width = 2 * (-minLeftBound + BORDER_MARGIN);
    world.height = TOP_MARGIN + (TOTAL_ROWS - 1) * PIN_V_SPACING + PIN_RADIUS;
    world.pinBoundaries.push_back(world.width / 2);

    return world;
}



int main() {
    // Multithreading
    omp_set_num_threads(6);

    // Window init
    sf::RenderWindow window(sf::VideoMode(INITIAL_SCREEN_WIDTH, INITIAL_SCREEN_HEIGHT),
        "Galton Board", sf::Style::Default);
    window.setFramerateLimit(0);

    // Grid init
    PinGrid pins;
    const PhysicsWorld physicsWorld = createGaltonBoard(pins);

    // Collision lambda-functions
    auto collide = [&pins = std::as_const(pins)]
    (int col, int row, FallingBall& ball) {
        const int maxRow = TOTAL_ROWS;
        const int maxCol = MAX_PINS_PER_ROW - (row % 2);

        if (col > -1 && row > -1) {
            if (col < maxCol && row < maxRow) {
                auto const curPin = pins[row][col];
                if (curPin.checkCollision(ball)) ball.resolveCollision(pins[row][col]);
            }
        }
        };

    auto handleCollisions = [&pinsOrigin = std::as_const(physicsWorld.pinsOrigin),
        &pins = std::as_const(pins),
        &collide = std::as_const(collide)]
        (FallingBall& ball) {
        const Vector2d localPos = ball.getPosition() - pinsOrigin;
        const int row = static_cast<int>(localPos.y / PIN_V_SPACING);
        const bool isOddRow = row % 2;
        const double horizontalOffset = isOddRow ? PIN_H_SPACING / 2 : 0;
        const int col = static_cast<int>((localPos.x + horizontalOffset) / PIN_H_SPACING);

        const int direction = -1 + isOddRow * 2;

        // Hexagonal cell grid around chosen cell with ball
        collide(col - direction, row, ball);
        for (int dx = 0; dx != direction; dx = dx + direction) {
            for (int dy = -1; dy <= 1; ++dy) {
                const int curRow = row + dy;
                const int curCol = col + dx;

                collide(curCol, curRow, ball);
            }
        }
        };

    sf::View view;
    adjustView(view, INITIAL_SCREEN_WIDTH, INITIAL_SCREEN_HEIGHT,
        physicsWorld.width, physicsWorld.height);

    // Random coordinates generation
    std::random_device rd;
    std::mt19937 gen(rd());

    double right_inclusive = std::nextafter(HALF_GAP, std::numeric_limits<double>::max());
    std::uniform_real_distribution<double> positionDistribution(-HALF_GAP, right_inclusive);

    // Containers
    std::vector<std::unique_ptr<FallingBall>> activeBalls;
    activeBalls.reserve(MAX_ACTIVE_BALLS);

    int totalBallsCreated = 0;
    std::vector<unsigned int> ballsCounter(physicsWorld.pinBoundaries.size(), 0);

    // Clocks
    sf::Clock physicsClock, renderClock;
    double physicsAccumulator = 0.0;
    const double renderInterval = 1.0 / 60.0;
    double renderAccumulator = 0.0;

    // Drawing
    sf::CircleShape pinShape(static_cast<float>(PIN_RADIUS));
    pinShape.setFillColor(PIN_COLOR);

    sf::CircleShape ballShape(static_cast<float>(BALL_RADIUS));
    ballShape.setFillColor(BALL_COLOR);

    // Main loop
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }

            if (event.type == sf::Event::Resized) {
                adjustView(view, event.size.width, event.size.height,
                    physicsWorld.width, physicsWorld.height);
                window.setView(view);
            }
        }

        // Add new balls
        while (totalBallsCreated < TOTAL_BALLS && activeBalls.size() < MAX_ACTIVE_BALLS) {
            double x = positionDistribution(gen);
            activeBalls.emplace_back(std::make_unique<FallingBall>(Vector2d(x, 0), BALL_RADIUS, LOSS_COEFF, physicsWorld.width));
            totalBallsCreated++;
        }

        // Exit
        if (activeBalls.size() == 0) break;

        // Physics update
        const double deltaTime = physicsClock.restart().asSeconds();
        physicsAccumulator += deltaTime * SIMULATION_SPEED;

        while (physicsAccumulator >= FIXED_TIMESTEP) {
            const int numBalls = static_cast<int>(activeBalls.size());

            // Individual ball calculations
            #pragma omp parallel for schedule(dynamic, 256)
            for (int i = 0; i < numBalls; ++i) {
                auto& ball = activeBalls[i];
                if (!ball->getActivity()) continue;

                // Physics
                ball->update(FIXED_TIMESTEP);
                handleCollisions(*ball);

                // Fall out
                const Vector2d position = ball->getPosition();
                if (position.y > physicsWorld.height) {
                    const auto it = std::upper_bound(physicsWorld.pinBoundaries.begin(),
                        physicsWorld.pinBoundaries.end(),
                        position.x);
                    const size_t pinIndex = std::distance(physicsWorld.pinBoundaries.begin(), it);

                    #pragma omp atomic
                    ballsCounter[pinIndex]++;

                    ball->setActivity(false);
                }
            }

            // Remove inacative balls
            auto new_end = std::remove_if(
                activeBalls.begin(),
                activeBalls.end(),
                [](const std::unique_ptr<FallingBall>& ball) {
                    return !ball->getActivity();
                }
            );
            activeBalls.erase(new_end, activeBalls.end());

            physicsAccumulator -= FIXED_TIMESTEP;
        }

        // Rendering
        #if DO_RENDER
        renderAccumulator += renderClock.restart().asSeconds();
        if (renderAccumulator >= renderInterval) {
            window.clear(BACKGROUND_COLOR);
            window.setView(view);

            // Draw pins
            for (const auto& row : pins) {
                for (const auto& pin : row) {
                    const sf::Vector2f position = toPhysicsCoordinates(pin.getPosition(), Vector2d(PIN_RADIUS, PIN_RADIUS));
                    pinShape.setPosition(position);
                    window.draw(pinShape);
                }
            }

            // Draw balls
            for (const auto& ball : activeBalls) {
                const sf::Vector2f position = toPhysicsCoordinates(ball->getPosition(), Vector2d(BALL_RADIUS, BALL_RADIUS));
                ballShape.setPosition(position);
                window.draw(ballShape);
            }

            window.display();
            renderAccumulator = 0.0;
        }
        #endif
    }

    // Create file name
    auto currentTime = std::chrono::system_clock::now();
    auto timeSinceEpoch = currentTime.time_since_epoch();
    auto timeSinceEpochMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(timeSinceEpoch).count();
    std::string fileName = RESULTS_FOLDER_PATH + std::to_string(timeSinceEpochMilliseconds) + ".csv";

    // Create file and write data
    std::ofstream outputFile(fileName);
    if (outputFile.is_open() && !ballsCounter.empty()) {
        outputFile << std::endl;
        for (int i = 0; i < ballsCounter.size(); ++i)
            outputFile << i << ", " << ballsCounter[i] << std::endl;
        outputFile.close();
    }

    return 0;
}