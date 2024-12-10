//sources
// Lesson Notes
// Assistance from ChatGPT

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>

const int WINDOW_WIDTH = 1920;
const int WINDOW_HEIGHT = 1080;
const int MAX_PARTICLES = 1000;

const char* vertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec2 aPos;

void main()
{
    gl_Position = vec4(aPos, 0.0, 1.0);
    gl_PointSize = 4.0; // Adjust for snowflake size
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

void main()
{
    FragColor = vec4(1.0, 1.0, 1.0, 1.0); // Snowflakes are white
}
)";

struct Particle
{
    glm::vec2 position, velocity;
    bool landed = false;

    void update(float dt, float surfaceY, float surfaceWidth, float surfaceX, std::vector<float>& snowHeights)
    {
        if (landed) return;

        position += velocity * dt;

        if (position.y <= surfaceY && position.x >= surfaceX && position.x <= surfaceX + surfaceWidth)
        {
            int column = static_cast<int>((position.x - surfaceX) / (surfaceWidth / snowHeights.size()));
            float snowTop = surfaceY + snowHeights[column];

            if (position.y <= snowTop)
            {
                position.y = snowTop;
                snowHeights[column] += 0.01f;
                landed = true;
            }
        }
    }
};

GLuint compileShader(GLenum type, const char* source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader Compilation Error: " << infoLog << std::endl;
    }
    return shader;
}

class RenderLoop
{
private:
    GLFWwindow* window;
    GLuint shaderProgram;
    GLuint surfaceVAO, surfaceVBO;
    GLuint particleVBO;
    std::vector<Particle> particles;
    std::vector<float> snowHeights;

    float surfaceX, surfaceY, surfaceWidth, surfaceHeight;

    void initShaders()
    {
        GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
        GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);

        int success;
        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
        if (!success)
        {
            char infoLog[512];
            glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
            std::cerr << "Shader Linking Error: " << infoLog << std::endl;
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }

    void initSurface()
    {
        surfaceX = -1.0f;
        surfaceY = -1.0f;
        surfaceWidth = 2.5f;
        surfaceHeight = 0.1f;
        snowHeights = std::vector<float>(100, 0.0f);

        float surfaceVertices[] = {
            surfaceX, surfaceY,
            surfaceX + surfaceWidth, surfaceY,
            surfaceX + surfaceWidth, surfaceY + surfaceHeight,
            surfaceX, surfaceY + surfaceHeight
        };

        glGenVertexArrays(1, &surfaceVAO);
        glGenBuffers(1, &surfaceVBO);
        glBindVertexArray(surfaceVAO);
        glBindBuffer(GL_ARRAY_BUFFER, surfaceVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(surfaceVertices), surfaceVertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
    }

    void initParticles()
    {
        glGenBuffers(1, &particleVBO);
    }

    void renderSurface()
    {
        glUseProgram(shaderProgram);
        glBindVertexArray(surfaceVAO);
        glDrawArrays(GL_LINE_LOOP, 0, 4);
        glBindVertexArray(0);
    }

    void renderParticles()
    {
        std::vector<float> positions;
        for (const auto& particle : particles)
        {
            positions.push_back(particle.position.x / (WINDOW_WIDTH / 2) - 1.0f);
            positions.push_back(particle.position.y / (WINDOW_HEIGHT / 2) - 1.0f);
        }

        glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
        glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(float), positions.data(), GL_DYNAMIC_DRAW);

        glUseProgram(shaderProgram);
        glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glDrawArrays(GL_POINTS, 0, particles.size());
        glDisableVertexAttribArray(0);
    }

    void updateParticles(float deltaTime)
    {
        for (auto& particle : particles)
        {
            particle.update(deltaTime, surfaceY * (WINDOW_HEIGHT / 2), surfaceWidth * WINDOW_WIDTH, surfaceX * WINDOW_WIDTH, snowHeights);
        }
    }

public:
    RenderLoop(GLFWwindow* win) : window(win), shaderProgram(0), surfaceVAO(0), surfaceVBO(0), particleVBO(0) {}

    void initialize()
    {
        initShaders();
        initSurface();
        initParticles();
    }

    void run()
    {
        double previousTime = glfwGetTime();

        while (!glfwWindowShouldClose(window))
        {
            double currentTime = glfwGetTime();
            float deltaTime = static_cast<float>(currentTime - previousTime);
            previousTime = currentTime;

            for (int i = 0; i < 5; ++i)
            {
                float x = static_cast<float>(rand() % WINDOW_WIDTH);
                particles.push_back({ { x, WINDOW_HEIGHT }, { 0, -50.0f } });
            }

            updateParticles(deltaTime);

            glClear(GL_COLOR_BUFFER_BIT);
            renderSurface();
            renderParticles();
            glfwSwapBuffers(window);
            glfwPollEvents();
        }

        cleanup();
    }

    void cleanup()
    {
        glDeleteBuffers(1, &particleVBO);
        glDeleteBuffers(1, &surfaceVBO);
        glDeleteVertexArrays(1, &surfaceVAO);
        glDeleteProgram(shaderProgram);
    }
};

int main()
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Snow Simulation", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    RenderLoop renderLoop(window);
    renderLoop.initialize();
    renderLoop.run();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
