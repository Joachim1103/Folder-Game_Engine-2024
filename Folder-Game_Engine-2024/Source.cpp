//sources
// Lesson Notes
// Assistance from ChatGPT

#include <iostream>
#include <vector>
#include <cmath>
#include <GLFW/glfw3.h>
#include <cstdlib>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Constants
const float BALL_RADIUS = 10.0f;
const int WINDOW_WIDTH = 1920;
const int WINDOW_HEIGHT = 1080;
const float DAMPING = 0.98f;
const int MAX_OBJECTS = 10;
const int MAX_LEVELS = 5;

struct Vec2 
{
    float x, y;
    Vec2 operator+(const Vec2& v) const { return { x + v.x, y + v.y }; }
    Vec2 operator-(const Vec2& v) const { return { x - v.x, y - v.y }; }
    Vec2 operator*(float scalar) const { return { x * scalar, y * scalar }; }
    float length() const { return std::sqrt(x * x + y * y); }
    Vec2 normalize() const { float len = length(); return { x / len, y / len }; }
};

class Ball 
{
public:
    Vec2 position, velocity;
    float radius;
    Ball(Vec2 pos, float r) : position(pos), velocity({ 0, 0 }), radius(r) {}

    void update(float dt) 
    {
        position = position + velocity * dt;
    }

    void render() 
    {
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(position.x, position.y);
        for (int i = 0; i <= 20; i++) 
        {
            float angle = i * 2.0f * M_PI / 20;
            glVertex2f(position.x + radius * cosf(angle), position.y + radius * sinf(angle));
        }
        glEnd();
    }
};

struct AABB 
{
    float x, y, width, height;

    bool contains(const Ball& ball) const 
    {
        return (ball.position.x - ball.radius >= x && ball.position.x + ball.radius <= x + width &&
            ball.position.y - ball.radius >= y && ball.position.y + ball.radius <= y + height);
    }

    bool intersects(const AABB& range) const 
    {
        return !(range.x > x + width || range.x + range.width < x || range.y > y + height || range.y + range.height < y);
    }
};

class Quadtree 
{
private:
    int level;
    std::vector<Ball*> objects;
    AABB bounds;
    Quadtree* nodes[4] = { nullptr, nullptr, nullptr, nullptr };

    void split() 
    {
        float subWidth = bounds.width / 2.0f;
        float subHeight = bounds.height / 2.0f;
        float x = bounds.x;
        float y = bounds.y;

        nodes[0] = new Quadtree(level + 1, { x + subWidth, y, subWidth, subHeight });
        nodes[1] = new Quadtree(level + 1, { x, y, subWidth, subHeight });
        nodes[2] = new Quadtree(level + 1, { x, y + subHeight, subWidth, subHeight });
        nodes[3] = new Quadtree(level + 1, { x + subWidth, y + subHeight, subWidth, subHeight });
    }

    int getIndex(const Ball& ball) const 
    {
        float verticalMidpoint = bounds.x + bounds.width / 2.0f;
        float horizontalMidpoint = bounds.y + bounds.height / 2.0f;

        bool topQuadrant = (ball.position.y - ball.radius < horizontalMidpoint);
        bool bottomQuadrant = (ball.position.y - ball.radius > horizontalMidpoint);

        if (ball.position.x - ball.radius < verticalMidpoint) 
        {
            if (topQuadrant) return 1;
            if (bottomQuadrant) return 2;
        }
        else if (ball.position.x - ball.radius > verticalMidpoint) 
        {
            if (topQuadrant) return 0;
            if (bottomQuadrant) return 3;
        }
        return -1;
    }

public:
    Quadtree(int level, const AABB& bounds) : level(level), bounds(bounds) {}

    void clear() 
    {
        objects.clear();
        for (int i = 0; i < 4; i++) 
        {
            if (nodes[i]) 
            {
                nodes[i]->clear();
                delete nodes[i];
                nodes[i] = nullptr;
            }
        }
    }

    void insert(Ball& ball) 
    {
        if (nodes[0]) 
        {
            int index = getIndex(ball);
            if (index != -1) 
            {
                nodes[index]->insert(ball);
                return;
            }
        }
        objects.push_back(&ball);
        if (objects.size() > MAX_OBJECTS && level < MAX_LEVELS) 
        {
            if (!nodes[0]) split();
            auto it = objects.begin();
            while (it != objects.end()) 
            {
                int index = getIndex(**it);
                if (index != -1) 
                {
                    nodes[index]->insert(**it);
                    it = objects.erase(it);
                }
                else ++it;
            }
        }
    }

    void retrieve(std::vector<Ball*>& returnObjects, const Ball& ball) 
    {
        int index = getIndex(ball);
        if (index != -1 && nodes[0]) nodes[index]->retrieve(returnObjects, ball);
        returnObjects.insert(returnObjects.end(), objects.begin(), objects.end());
    }
};

class Collision 
{
public:
    static bool detectBallBall(const Ball& a, const Ball& b) 
    {
        Vec2 delta = a.position - b.position;
        return delta.length() <= (a.radius + b.radius);
    }

    static void resolveBallBall(Ball& a, Ball& b) 
    {
        Vec2 delta = a.position - b.position;
        if (delta.length() == 0) return;
        Vec2 normal = delta.normalize();

        Vec2 relativeVelocity = a.velocity - b.velocity;
        float velocityAlongNormal = relativeVelocity.x * normal.x + relativeVelocity.y * normal.y;
        if (velocityAlongNormal > 0) return;

        float restitution = 0.75f;
        float impulseScalar = -(1 + restitution) * velocityAlongNormal / (1.0f / a.radius + 1.0f / b.radius);

        Vec2 impulse = normal * impulseScalar;
        a.velocity = a.velocity - impulse * (1.0f / a.radius);
        b.velocity = b.velocity + impulse * (1.0f / b.radius);

        float maxSpeed = 200.0f;
        if (a.velocity.length() > maxSpeed) a.velocity = a.velocity.normalize() * maxSpeed;
        if (b.velocity.length() > maxSpeed) b.velocity = b.velocity.normalize() * maxSpeed;

        a.velocity.x += randomPerturbation();
        b.velocity.x += randomPerturbation();
    }

    static void resolveBallWall(Ball& ball, const Vec2& wallNormal) 
    {
        float velocityAlongNormal = ball.velocity.x * wallNormal.x + ball.velocity.y * wallNormal.y;
        if (velocityAlongNormal < 0) ball.velocity = ball.velocity - wallNormal * 2.0f * velocityAlongNormal;
    }

    static float randomPerturbation() 
    {
        return (static_cast<float>(rand()) / static_cast<float>(RAND_MAX) - 0.5f) * 0.05f;
    }
};

class PhysicsEngine 
{
    std::vector<Ball> balls;
    Quadtree quadtree;
    bool gravityEnabled = false;

public:
    PhysicsEngine() : quadtree(0, { 0, 0, 1080, 1920 }) {}

    void addBall(const Ball& ball) 
    {
        balls.push_back(ball);
    }

    void update(float dt) 
    {
        if (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_SPACE) == GLFW_PRESS) 
        {
            gravityEnabled = true;
        }

        if (gravityEnabled) 
        {
            for (Ball& ball : balls) 
            {
                ball.velocity.y -= 9.8f * dt * 3.0f;
            }
        }

        quadtree.clear();
        float rectWidth = 600, rectHeight = 900;
        float xOffset = (WINDOW_WIDTH - rectWidth) / 2, yOffset = (WINDOW_HEIGHT - rectHeight) / 2;
        float friction = 0.997f, restitution = 0.75f;

        for (Ball& ball : balls) {
            ball.update(dt * 5.0f);
            quadtree.insert(ball);
            ball.velocity = ball.velocity * friction;

            if (ball.position.x - ball.radius < xOffset) 
            {
                ball.position.x = xOffset + ball.radius;
                ball.velocity.x = -ball.velocity.x * restitution;
                ball.velocity.x += Collision::randomPerturbation();
            }
            else if (ball.position.x + ball.radius > xOffset + rectWidth) 
            {
                ball.position.x = xOffset + rectWidth - ball.radius;
                ball.velocity.x = -ball.velocity.x * restitution;
                ball.velocity.x += Collision::randomPerturbation();
            }

            if (ball.position.y - ball.radius < yOffset) 
            {
                ball.position.y = yOffset + ball.radius;
                ball.velocity.y = -ball.velocity.y * restitution;
                ball.velocity.x += Collision::randomPerturbation();
            }
            else if (ball.position.y + ball.radius > yOffset + rectHeight) 
            {
                ball.position.y = yOffset + rectHeight - ball.radius;
                ball.velocity.y = -ball.velocity.y * restitution;
                ball.velocity.x += Collision::randomPerturbation();
            }

            float maxSpeed = 200.0f;
            if (ball.velocity.length() > maxSpeed) ball.velocity = ball.velocity.normalize() * maxSpeed;
        }

        for (Ball& ball : balls) 
        {
            std::vector<Ball*> possibleCollisions;
            quadtree.retrieve(possibleCollisions, ball);
            for (Ball* other : possibleCollisions) 
            {
                if (&ball != other && Collision::detectBallBall(ball, *other)) 
                {
                    Collision::resolveBallBall(ball, *other);
                }
            }
        }
    }

    void render() 
    {
        float rectWidth = 600, rectHeight = 900;
        float xOffset = (WINDOW_WIDTH - rectWidth) / 2, yOffset = (WINDOW_HEIGHT - rectHeight) / 2;

        glBegin(GL_LINE_LOOP);
        glVertex2f(xOffset, yOffset);
        glVertex2f(xOffset + rectWidth, yOffset);
        glVertex2f(xOffset + rectWidth, yOffset + rectHeight);
        glVertex2f(xOffset, yOffset + rectHeight);
        glEnd();

        for (Ball& ball : balls) ball.render();
    }
};

int main() 
{
    if (!glfwInit()) 
    {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "2D Ball Collision Simulation", nullptr, nullptr);
    if (!window) 
    {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 1920, 0, 1080, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    PhysicsEngine engine;
    engine.addBall(Ball({ 960, 950 }, BALL_RADIUS));
    engine.addBall(Ball({ 960, 900 }, BALL_RADIUS));
    engine.addBall(Ball({ 960, 850 }, BALL_RADIUS));
    engine.addBall(Ball({ 960, 800 }, BALL_RADIUS));
    engine.addBall(Ball({ 960, 750 }, BALL_RADIUS));
    engine.addBall(Ball({ 960, 700 }, BALL_RADIUS));
    engine.addBall(Ball({ 960, 650 }, BALL_RADIUS));

    double previousTime = glfwGetTime();
    const double targetFrameTime = 1.0 / 360.0;

    while (!glfwWindowShouldClose(window)) 
    {
        double currentTime = glfwGetTime();
        double deltaTime = currentTime - previousTime;
        if (deltaTime >= targetFrameTime) 
        {
            float dt = static_cast<float>(std::min(deltaTime, 0.01));
            glClear(GL_COLOR_BUFFER_BIT);
            engine.update(dt * 5.0f);
            engine.render();
            glfwSwapBuffers(window);
            glfwPollEvents();
            previousTime = currentTime;
        }
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}