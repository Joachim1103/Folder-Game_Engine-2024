//sources
// Lesson Notes
// Assistance from ChatGPT

#include <iostream>
#include <vector>
#include <cmath>
#include <GLFW/glfw3.h>
#include <cstdlib>
#include <unordered_map>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Constants
const float BALL_RADIUS = 10.0f;
const int WINDOW_WIDTH = 1920;
const int WINDOW_HEIGHT = 1080;
const float DAMPING = 0.98f;
const float GRAVITY = -9.8f * 3.0f;
const float MAX_SPEED = 200.0f;
const float RECT_WIDTH = 600.0f;
const float RECT_HEIGHT = 900.0f;
const float FRICTION = 0.997f;
const float RESTITUTION = 0.75f;
const float SPEED_MULTIPLIER = 5.0f;

float randomPerturbation() 
{
    return (static_cast<float>(rand()) / static_cast<float>(RAND_MAX) - 0.5f) * 0.1f;
}

// Vec2 Struct
struct Vec2 
{
    float x, y;
    Vec2 operator+(const Vec2& v) const { return { x + v.x, y + v.y }; }
    Vec2 operator-(const Vec2& v) const { return { x - v.x, y - v.y }; }
    Vec2 operator*(float scalar) const { return { x * scalar, y * scalar }; }
    float length() const { return std::sqrt(x * x + y * y); }
    Vec2 normalize() const { float len = length(); return { x / len, y / len }; }
};

// DoD Components
struct TransformComponent 
{
    std::vector<Vec2> positions;
    std::vector<Vec2> velocities;
};

struct RenderComponent 
{
    std::vector<float> radii;
};

struct CollisionComponent 
{
    std::vector<float> radii;
};

struct PhysicsComponent 
{
    std::vector<bool> affectedByGravity;
};

// Entity
struct Entity 
{
    int id;
    TransformComponent* transform = nullptr;
    RenderComponent* render = nullptr;
    CollisionComponent* collision = nullptr;
    PhysicsComponent* physics = nullptr;
};

// ECS Manager
class ECSManager 
{
public:
    TransformComponent transforms;
    RenderComponent renders;
    CollisionComponent collisions;
    PhysicsComponent physics;

    void addEntity(float x, float y, float radius, bool affectedByGravity) 
    {
        transforms.positions.push_back({ x, y });
        transforms.velocities.push_back({ 0, 0 });
        renders.radii.push_back(radius);
        collisions.radii.push_back(radius);
        physics.affectedByGravity.push_back(affectedByGravity);
    }

    size_t getEntityCount() const 
    {
        return transforms.positions.size();
    }
};

// Systems
class PhysicsSystem 
{
private:
    bool gravityEnabled = false;

public:
    void handleInput(GLFWwindow* window) 
    {
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) 
        {
            gravityEnabled = true;
        }
    }

    void update(float dt, TransformComponent& transforms, PhysicsComponent& physics, CollisionComponent& collisions) 
    {
        dt *= SPEED_MULTIPLIER;
        float xOffset = (WINDOW_WIDTH - RECT_WIDTH) / 2.0f;
        float yOffset = (WINDOW_HEIGHT - RECT_HEIGHT) / 2.0f;

        for (size_t i = 0; i < transforms.positions.size(); ++i) 
        {
            if (physics.affectedByGravity[i] && gravityEnabled) 
            {
                transforms.velocities[i].y += GRAVITY * dt;
            }

            transforms.velocities[i] = transforms.velocities[i] * FRICTION;

            if (transforms.velocities[i].length() > MAX_SPEED) 
            {
                transforms.velocities[i] = transforms.velocities[i].normalize() * MAX_SPEED;
            }

            transforms.positions[i] = transforms.positions[i] + transforms.velocities[i] * dt;

            if (transforms.positions[i].x - collisions.radii[i] < xOffset) 
            {
                transforms.positions[i].x = xOffset + collisions.radii[i];
                transforms.velocities[i].x = -transforms.velocities[i].x * RESTITUTION;
                transforms.velocities[i].x += randomPerturbation();
                transforms.velocities[i].y += randomPerturbation();
            }
            else if (transforms.positions[i].x + collisions.radii[i] > xOffset + RECT_WIDTH) 
            {
                transforms.positions[i].x = xOffset + RECT_WIDTH - collisions.radii[i];
                transforms.velocities[i].x = -transforms.velocities[i].x * RESTITUTION;
                transforms.velocities[i].x += randomPerturbation();
                transforms.velocities[i].y += randomPerturbation();
            }

            if (transforms.positions[i].y - collisions.radii[i] < yOffset) 
            {
                transforms.positions[i].y = yOffset + collisions.radii[i];
                transforms.velocities[i].y = -transforms.velocities[i].y * RESTITUTION;
                transforms.velocities[i].x += randomPerturbation();
                transforms.velocities[i].y += randomPerturbation();
            }
            else if (transforms.positions[i].y + collisions.radii[i] > yOffset + RECT_HEIGHT) 
            {
                transforms.positions[i].y = yOffset + RECT_HEIGHT - collisions.radii[i];
                transforms.velocities[i].y = -transforms.velocities[i].y * RESTITUTION;
                transforms.velocities[i].x += randomPerturbation();
                transforms.velocities[i].y += randomPerturbation();
            }
        }
    }
};

class CollisionSystem 
{
public:
    void resolveCollisions(TransformComponent& transforms, CollisionComponent& collisions) 
    {
        for (size_t i = 0; i < transforms.positions.size(); ++i) 
        {
            for (size_t j = i + 1; j < transforms.positions.size(); ++j) 
            {
                Vec2 delta = transforms.positions[i] - transforms.positions[j];
                float distance = delta.length();
                float overlap = collisions.radii[i] + collisions.radii[j] - distance;

                if (overlap > 0) 
                {
                    Vec2 normal = delta.normalize();
                    Vec2 impulse = normal * (overlap / 2.0f);
                    transforms.positions[i] = transforms.positions[i] + impulse;
                    transforms.positions[j] = transforms.positions[j] - impulse;

                    transforms.velocities[i] = transforms.velocities[i] - normal * 2.0f * (transforms.velocities[i].x * normal.x + transforms.velocities[i].y * normal.y);
                    transforms.velocities[j] = transforms.velocities[j] - normal * 2.0f * (transforms.velocities[j].x * normal.x + transforms.velocities[j].y * normal.y);

                    transforms.velocities[i].x += randomPerturbation();
                    transforms.velocities[i].y += randomPerturbation();
                    transforms.velocities[j].x += randomPerturbation();
                    transforms.velocities[j].y += randomPerturbation();
                }
            }
        }
    }
};

class RenderSystem 
{
public:
    void render(const TransformComponent& transforms, const RenderComponent& renders) 
    {
        glClear(GL_COLOR_BUFFER_BIT);

        glBegin(GL_LINE_LOOP);
        float xOffset = (WINDOW_WIDTH - RECT_WIDTH) / 2.0f;
        float yOffset = (WINDOW_HEIGHT - RECT_HEIGHT) / 2.0f;
        glVertex2f(xOffset, yOffset);
        glVertex2f(xOffset + RECT_WIDTH, yOffset);
        glVertex2f(xOffset + RECT_WIDTH, yOffset + RECT_HEIGHT);
        glVertex2f(xOffset, yOffset + RECT_HEIGHT);
        glEnd();

        for (size_t i = 0; i < transforms.positions.size(); ++i) 
        {
            glBegin(GL_TRIANGLE_FAN);
            glVertex2f(transforms.positions[i].x, transforms.positions[i].y);
            for (int j = 0; j <= 20; ++j) 
            {
                float angle = j * 2.0f * M_PI / 20;
                glVertex2f(transforms.positions[i].x + renders.radii[i] * std::cos(angle),
                    transforms.positions[i].y + renders.radii[i] * std::sin(angle));
            }
            glEnd();
        }
    }
};

int main() 
{
    if (!glfwInit()) 
    {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "ECS Ball Collision Simulation", nullptr, nullptr);
    if (!window) 
    {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    ECSManager ecsManager;
    PhysicsSystem physicsSystem;
    CollisionSystem collisionSystem;
    RenderSystem renderSystem;

    float startY = 950;
    for (int i = 0; i < 7; ++i) 
    {
        ecsManager.addEntity(960, startY, BALL_RADIUS, true);
        startY -= 50;
    }

    double previousTime = glfwGetTime();
    const double targetFrameTime = 1.0 / 360.0;

    while (!glfwWindowShouldClose(window)) 
    {
        double currentTime = glfwGetTime();
        float deltaTime = static_cast<float>(currentTime - previousTime);

        if (deltaTime >= targetFrameTime) 
        {
            physicsSystem.handleInput(window);
            physicsSystem.update(deltaTime, ecsManager.transforms, ecsManager.physics, ecsManager.collisions);
            collisionSystem.resolveCollisions(ecsManager.transforms, ecsManager.collisions);
            renderSystem.render(ecsManager.transforms, ecsManager.renders);

            glfwSwapBuffers(window);
            glfwPollEvents();
            previousTime = currentTime;
        }
    }

    glfwTerminate();
    return 0;
}
