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

float randomPerturbation() {
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

// Components
struct TransformComponent 
{
    Vec2 position;
    Vec2 velocity;
};

struct RenderComponent 
{
    float radius;
};

struct CollisionComponent 
{
    float radius;
};

struct PhysicsComponent 
{
    bool affectedByGravity;
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
private:
    int nextEntityID = 0;
    std::unordered_map<int, Entity> entities;

public:
    Entity& createEntity() 
    {
        int id = nextEntityID++;
        entities[id] = { id };
        return entities[id];
    }

    void removeEntity(int id) 
    {
        entities.erase(id);
    }

    std::vector<Entity*> getEntities() 
    {
        std::vector<Entity*> result;
        for (auto& pair : entities) 
        {
            result.push_back(&pair.second);
        }
        return result;
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

    void update(float dt, std::vector<Entity*>& entities) 
    {
        dt *= SPEED_MULTIPLIER;

        float xOffset = (WINDOW_WIDTH - RECT_WIDTH) / 2.0f;
        float yOffset = (WINDOW_HEIGHT - RECT_HEIGHT) / 2.0f;

        for (Entity* entity : entities) 
        {
            if (entity->physics && entity->transform) 
            {
                if (entity->physics->affectedByGravity && gravityEnabled) 
                {
                    entity->transform->velocity.y += GRAVITY * dt;
                }
                entity->transform->velocity = entity->transform->velocity * FRICTION;

                if (entity->transform->velocity.length() > MAX_SPEED) 
                {
                    entity->transform->velocity = entity->transform->velocity.normalize() * MAX_SPEED;
                }

                entity->transform->position = entity->transform->position + entity->transform->velocity * dt;

                if (entity->transform->position.x - entity->collision->radius < xOffset) 
                {
                    entity->transform->position.x = xOffset + entity->collision->radius;
                    entity->transform->velocity.x = -entity->transform->velocity.x * RESTITUTION;
                    entity->transform->velocity.x += randomPerturbation();
                    entity->transform->velocity.y += randomPerturbation();
                }
                else if (entity->transform->position.x + entity->collision->radius > xOffset + RECT_WIDTH) 
                {
                    entity->transform->position.x = xOffset + RECT_WIDTH - entity->collision->radius;
                    entity->transform->velocity.x = -entity->transform->velocity.x * RESTITUTION;
                    entity->transform->velocity.x += randomPerturbation();
                    entity->transform->velocity.y += randomPerturbation();
                }

                if (entity->transform->position.y - entity->collision->radius < yOffset) 
                {
                    entity->transform->position.y = yOffset + entity->collision->radius;
                    entity->transform->velocity.y = -entity->transform->velocity.y * RESTITUTION;
                    entity->transform->velocity.x += randomPerturbation();
                    entity->transform->velocity.y += randomPerturbation();
                }
                else if (entity->transform->position.y + entity->collision->radius > yOffset + RECT_HEIGHT) 
                {
                    entity->transform->position.y = yOffset + RECT_HEIGHT - entity->collision->radius;
                    entity->transform->velocity.y = -entity->transform->velocity.y * RESTITUTION;
                    entity->transform->velocity.x += randomPerturbation();
                    entity->transform->velocity.y += randomPerturbation();
                }
            }
        }
    }
};


class CollisionSystem 
{
public:
    void resolveCollisions(std::vector<Entity*>& entities) 
    {
        for (size_t i = 0; i < entities.size(); ++i) {
            for (size_t j = i + 1; j < entities.size(); ++j) 
            {
                Entity* a = entities[i];
                Entity* b = entities[j];

                if (a->collision && b->collision && a->transform && b->transform) 
                {
                    Vec2 delta = a->transform->position - b->transform->position;
                    float distance = delta.length();
                    float overlap = a->collision->radius + b->collision->radius - distance;

                    if (overlap > 0) 
                    {
                        Vec2 normal = delta.normalize();
                        Vec2 impulse = normal * (overlap / 2.0f);
                        a->transform->position = a->transform->position + impulse;
                        b->transform->position = b->transform->position - impulse;

                        a->transform->velocity = a->transform->velocity - normal * 2.0f * (a->transform->velocity.x * normal.x + a->transform->velocity.y * normal.y);
                        b->transform->velocity = b->transform->velocity - normal * 2.0f * (b->transform->velocity.x * normal.x + b->transform->velocity.y * normal.y);

                        a->transform->velocity.x += randomPerturbation();
                        a->transform->velocity.y += randomPerturbation();
                        b->transform->velocity.x += randomPerturbation();
                        b->transform->velocity.y += randomPerturbation();
                    }
                }
            }
        }
    }
};

class RenderSystem 
{
public:
    void render(std::vector<Entity*>& entities) 
    {
        glClear(GL_COLOR_BUFFER_BIT);

        glBegin(GL_LINE_LOOP);
        float xOffset = (WINDOW_WIDTH - RECT_WIDTH) / 2, yOffset = (WINDOW_HEIGHT - RECT_HEIGHT) / 2;
        glVertex2f(xOffset, yOffset);
        glVertex2f(xOffset + RECT_WIDTH, yOffset);
        glVertex2f(xOffset + RECT_WIDTH, yOffset + RECT_HEIGHT);
        glVertex2f(xOffset, yOffset + RECT_HEIGHT);
        glEnd();

        for (Entity* entity : entities) 
        {
            if (entity->render && entity->transform) 
            {
                glBegin(GL_TRIANGLE_FAN);
                glVertex2f(entity->transform->position.x, entity->transform->position.y);
                for (int i = 0; i <= 20; ++i) 
                {
                    float angle = i * 2.0f * M_PI / 20;
                    glVertex2f(entity->transform->position.x + entity->render->radius * std::cos(angle),
                        entity->transform->position.y + entity->render->radius * std::sin(angle));
                }
                glEnd();
            }
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
        Entity& ball = ecsManager.createEntity();
        ball.transform = new TransformComponent{ { 960, startY }, { 0, 0 } };
        ball.render = new RenderComponent{ BALL_RADIUS };
        ball.collision = new CollisionComponent{ BALL_RADIUS };
        ball.physics = new PhysicsComponent{ true };
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
            std::vector<Entity*> entities = ecsManager.getEntities();

            physicsSystem.handleInput(window);
            physicsSystem.update(deltaTime, entities);
            collisionSystem.resolveCollisions(entities);
            renderSystem.render(entities);

            glfwSwapBuffers(window);
            glfwPollEvents();

            previousTime = currentTime;
        }
    }

    glfwTerminate();
    return 0;
}
