#define _CRT_SECURE_NO_WARNINGS

//------- Ignore this ----------
#include<algorithm>
#include<filesystem>
#include<iomanip>
#include<sstream>
namespace fs = std::filesystem;
//------------------------------

#ifdef _WIN32
extern "C"
{
    __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 0x00000001;
}
#endif

#include "Model.h"
#include "Skybox.h"

// ─── Ventana / escena ──────────────────────────────────
const unsigned int width = 1920;
const unsigned int height = 1080;
const float targetSceneRadius = 1800.0f;
const float cameraFov = 55.0f;
const float cameraNearPlane = 0.05f;
const float cameraFarPlane = 6000.0f;
const float celestialOrbitRadius = 2800.0f;
const float maxSunHeight = 2000.0f;

// ─── Caminar ───────────────────────────────────────────
const float walkEyeHeight = 6.0f;
const float walkProbeRadius = 8.0f;
const float walkMaxStepUp = 12.0f;
const float walkMaxDropDown = 45.0f;
const float walkMaxSlopeDegrees = 68.0f;
const float walkSpeed = 85.0f;

// ─── Opciones ──────────────────────────────────────────
const bool showCoordinatesInWindowTitle = true;
const bool useFastRenderMode = false;
const glm::vec3 blockedZoneMin = glm::vec3(240.0f, -260.0f, 360.0f);
const glm::vec3 blockedZoneMax = glm::vec3(310.0f, -150.0f, 435.0f);

// CICLO MUY RAPIDO PARA VIDEO
const float dayNightSpeed = 0.20f;

const float SUN_SIZE = 90.0f;
const float MOON_SIZE = 65.0f;

void createSphere(GLuint& VAO, GLuint& VBO, GLuint& EBO, int sectors, float radius) {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    float x, y, z, xy;
    float nx, ny, nz;
    float sectorStep = 2 * 3.14159f / sectors;
    float stackStep = 3.14159f / sectors;
    float sectorAngle, stackAngle;

    for (int i = 0; i <= sectors; ++i) {
        stackAngle = 3.14159f / 2 - i * stackStep;
        xy = radius * cosf(stackAngle);
        z = radius * sinf(stackAngle);

        for (int j = 0; j <= sectors; ++j) {
            sectorAngle = j * sectorStep;
            x = xy * cosf(sectorAngle);
            y = xy * sinf(sectorAngle);

            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);

            nx = x / radius;
            ny = y / radius;
            nz = z / radius;
            vertices.push_back(nx);
            vertices.push_back(ny);
            vertices.push_back(nz);
        }
    }

    for (int i = 0; i < sectors; ++i) {
        int k1 = i * (sectors + 1);
        int k2 = k1 + sectors + 1;

        for (int j = 0; j < sectors; ++j, ++k1, ++k2) {
            if (i != 0) {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }
            if (i != (sectors - 1)) {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(width, height, "Proyecto", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);
    gladLoadGL();
    glViewport(0, 0, width, height);

    Shader shaderProgram("Shaders/default.vert", "Shaders/default.frag");
    Shader lightShader("Shaders/light.vert", "Shaders/light.frag");
    Shader sphereShader("Shaders/sphere.vert", "Shaders/sphere.frag");

    GLuint sunVAO, sunVBO, sunEBO;
    GLuint moonVAO, moonVBO, moonEBO;
    createSphere(sunVAO, sunVBO, sunEBO, 48, SUN_SIZE);
    createSphere(moonVAO, moonVBO, moonEBO, 36, MOON_SIZE);

    Model model("modelos/city.glb");
    Skybox skybox(
        {},
        "Shaders/skybox_cubemap.vert",
        "Shaders/skybox_cubemap.frag"
    );

    glm::vec3 modelCenter = model.GetCenter();
    float modelRadius = model.GetRadius();
    if (modelRadius < 1.0f) modelRadius = 1.0f;

    const float normalizationScale = targetSceneRadius / modelRadius;
    const glm::mat4 modelTransform =
        glm::scale(glm::mat4(1.0f), glm::vec3(normalizationScale)) *
        glm::translate(glm::mat4(1.0f), -modelCenter);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    Camera camera(width, height, glm::vec3(112.0f, -179.0f, 600.0f));
    camera.Orientation = glm::normalize(glm::vec3(1.0f, 0.0f, 0.0f));
    camera.speed = walkSpeed;
    camera.flyMode = false;

    float lastFrame = 0.0f;
    float lastTitleUpdate = 0.0f;
    glm::vec3 lastTitlePosition = camera.Position;

    glm::vec3 snappedStart;
    if (model.TrySnapToWalkableSurface(
        camera.Position, modelTransform,
        walkProbeRadius, walkEyeHeight,
        walkMaxStepUp, walkMaxDropDown, walkMaxSlopeDegrees,
        snappedStart))
    {
        camera.Position = snappedStart;
        lastTitlePosition = camera.Position;
    }

    float timeOfDayAngle = 0.0f;
    float sunHeight = 0.0f;
    float dayFactor = 0.0f;
    float nightFactor = 0.0f;

    glm::vec3 sunPos;
    glm::vec3 moonPos;
    bool isDay = true;

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        timeOfDayAngle = currentFrame * dayNightSpeed;
        float sunAngleRad = timeOfDayAngle;

        sunHeight = std::sin(sunAngleRad);
        isDay = sunHeight > 0.0f;

        dayFactor = glm::clamp(sunHeight + 0.2f, 0.05f, 1.0f);
        nightFactor = glm::clamp(-sunHeight + 0.1f, 0.0f, 1.0f);

        float sunHorizontal = std::cos(sunAngleRad);
        float sunVertical = sunHeight;

        if (sunVertical > 0.0f) {
            sunPos = glm::vec3(
                sunHorizontal * celestialOrbitRadius,
                sunVertical * maxSunHeight,
                std::sin(sunAngleRad) * celestialOrbitRadius * 0.5f
            );
        }
        else {
            sunPos = glm::vec3(
                sunHorizontal * celestialOrbitRadius,
                -500.0f,
                std::sin(sunAngleRad) * celestialOrbitRadius * 0.5f
            );
        }

        float moonAngleRad = sunAngleRad + 3.14159f;
        float moonHorizontal = std::cos(moonAngleRad);
        float moonVertical = std::sin(moonAngleRad);

        if (moonVertical > 0.0f) {
            moonPos = glm::vec3(
                moonHorizontal * celestialOrbitRadius * 0.8f,
                moonVertical * maxSunHeight * 0.6f,
                std::sin(moonAngleRad) * celestialOrbitRadius * 0.4f
            );
        }
        else {
            moonPos = glm::vec3(
                moonHorizontal * celestialOrbitRadius * 0.8f,
                -300.0f,
                std::sin(moonAngleRad) * celestialOrbitRadius * 0.4f
            );
        }

        glm::vec3 mainLightPos;
        glm::vec3 mainLightColor;
        float mainLightIntensity;
        glm::vec3 ambientColor;
        float diffuseIntensity;
        float specularIntensity;

        if (isDay) {
            mainLightPos = sunPos;
            float lightPower = glm::clamp(sunHeight * 1.2f, 0.1f, 1.0f);

            if (sunHeight > 0.7f) {
                mainLightColor = glm::vec3(1.0f, 0.98f, 0.92f);
                mainLightIntensity = 1.2f * lightPower;
                ambientColor = glm::vec3(0.35f, 0.38f, 0.42f);
                diffuseIntensity = 1.0f;
                specularIntensity = 0.6f;
            }
            else if (sunHeight > 0.4f) {
                float t = (sunHeight - 0.4f) / 0.3f;
                mainLightColor = glm::mix(
                    glm::vec3(1.0f, 0.75f, 0.55f),
                    glm::vec3(1.0f, 0.98f, 0.92f),
                    t
                );
                mainLightIntensity = 0.9f * lightPower;
                ambientColor = glm::vec3(0.28f, 0.30f, 0.32f);
                diffuseIntensity = 0.8f;
                specularIntensity = 0.5f;
            }
            else if (sunHeight > 0.1f) {
                float t = (sunHeight - 0.1f) / 0.3f;
                mainLightColor = glm::mix(
                    glm::vec3(1.0f, 0.45f, 0.25f),
                    glm::vec3(1.0f, 0.75f, 0.55f),
                    t
                );
                mainLightIntensity = 0.5f * lightPower;
                ambientColor = glm::vec3(0.18f, 0.16f, 0.20f);
                diffuseIntensity = 0.5f;
                specularIntensity = 0.3f;
            }
            else {
                mainLightColor = glm::vec3(0.9f, 0.45f, 0.30f);
                mainLightIntensity = 0.25f * lightPower;
                ambientColor = glm::vec3(0.08f, 0.07f, 0.10f);
                diffuseIntensity = 0.25f;
                specularIntensity = 0.15f;
            }
        }
        else {
            // NOCHE - Luna ilumina un poco la ciudad (20-35% de intensidad)
            mainLightPos = moonPos;
            float moonHeight = std::abs(moonVertical);
            // Intensidad de la luna: entre 20% y 35% para que se vea la ciudad
            float moonLightPower = glm::clamp(moonHeight * 0.4f, 0.20f, 0.35f);

            mainLightColor = glm::vec3(0.65f, 0.70f, 0.90f);
            mainLightIntensity = moonLightPower;

            // Ambiente suficiente para ver la ciudad
            ambientColor = glm::vec3(0.08f, 0.10f, 0.15f);
            diffuseIntensity = 0.35f;
            specularIntensity = 0.15f;

            // Noche mas oscura pero siempre visible
            if (sunHeight < -0.6f) {
                ambientColor = glm::vec3(0.05f, 0.06f, 0.08f);
                mainLightIntensity = 0.20f;
                diffuseIntensity = 0.25f;
            }
        }

        glm::vec4 lightColor = glm::vec4(mainLightColor * mainLightIntensity, 1.0f);

        glm::vec3 skyColor;
        if (isDay) {
            if (sunHeight > 0.6f) {
                skyColor = glm::vec3(0.45f, 0.75f, 1.0f);
            }
            else if (sunHeight > 0.2f) {
                float t = (sunHeight - 0.2f) / 0.4f;
                skyColor = glm::mix(
                    glm::vec3(0.85f, 0.55f, 0.45f),
                    glm::vec3(0.45f, 0.75f, 1.0f),
                    t
                );
            }
            else if (sunHeight > 0.0f) {
                float t = sunHeight / 0.2f;
                skyColor = glm::mix(
                    glm::vec3(0.08f, 0.06f, 0.15f),
                    glm::vec3(0.85f, 0.55f, 0.45f),
                    t
                );
            }
            else {
                skyColor = glm::vec3(0.04f, 0.03f, 0.08f);
            }
        }
        else {
            // Cielo nocturno visible (no completamente negro)
            skyColor = glm::vec3(0.03f, 0.03f, 0.06f);
            float moonHeight = std::abs(moonVertical);
            skyColor += mainLightColor * (0.03f + moonHeight * 0.03f);
        }

        glClearColor(skyColor.x, skyColor.y, skyColor.z, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const glm::vec3 prevPos = camera.Position;
        camera.Inputs(window, deltaTime);

        if (!camera.flyMode)
        {
            glm::vec3 snapped;
            if (model.TrySnapToWalkableSurface(
                camera.Position, modelTransform,
                walkProbeRadius, walkEyeHeight,
                walkMaxStepUp, walkMaxDropDown, walkMaxSlopeDegrees,
                snapped))
                camera.Position = snapped;
            else if (model.TrySnapToWalkableSurface(
                prevPos, modelTransform,
                walkProbeRadius, walkEyeHeight,
                walkMaxStepUp, walkMaxDropDown, walkMaxSlopeDegrees,
                snapped))
                camera.Position = snapped;
            else
                camera.Position = prevPos;

            const bool blocked =
                camera.Position.x >= blockedZoneMin.x && camera.Position.x <= blockedZoneMax.x &&
                camera.Position.y >= blockedZoneMin.y && camera.Position.y <= blockedZoneMax.y &&
                camera.Position.z >= blockedZoneMin.z && camera.Position.z <= blockedZoneMax.z;
            if (blocked) camera.Position = prevPos;
        }

        camera.updateMatrix(cameraFov, cameraNearPlane, cameraFarPlane);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(cameraFov), (float)width / height, cameraNearPlane, cameraFarPlane);

        shaderProgram.Activate();
        glUniform4f(glGetUniformLocation(shaderProgram.ID, "lightColor"), lightColor.x, lightColor.y, lightColor.z, lightColor.w);
        glUniform3f(glGetUniformLocation(shaderProgram.ID, "lightPos"), mainLightPos.x, mainLightPos.y, mainLightPos.z);
        glUniform3f(glGetUniformLocation(shaderProgram.ID, "moonPos"), moonPos.x, moonPos.y, moonPos.z);
        glUniform3f(glGetUniformLocation(shaderProgram.ID, "viewPos"), camera.Position.x, camera.Position.y, camera.Position.z);
        glUniform3f(glGetUniformLocation(shaderProgram.ID, "ambientColor"), ambientColor.x, ambientColor.y, ambientColor.z);
        glUniform1f(glGetUniformLocation(shaderProgram.ID, "time"), currentFrame);
        glUniform1f(glGetUniformLocation(shaderProgram.ID, "dayFactor"), dayFactor);
        glUniform1f(glGetUniformLocation(shaderProgram.ID, "nightFactor"), nightFactor);
        glUniform1f(glGetUniformLocation(shaderProgram.ID, "isDay"), isDay ? 1.0f : 0.0f);
        glUniform1f(glGetUniformLocation(shaderProgram.ID, "diffuseIntensity"), diffuseIntensity);
        glUniform1f(glGetUniformLocation(shaderProgram.ID, "specularIntensity"), specularIntensity);
        glUniform1f(glGetUniformLocation(shaderProgram.ID, "sunHeight"), sunHeight);

        model.Draw(shaderProgram, camera, modelTransform);

        if (sunPos.y > -200.0f && isDay) {
            sphereShader.Activate();
            glm::mat4 sunModel = glm::translate(glm::mat4(1.0f), sunPos);
            glUniformMatrix4fv(glGetUniformLocation(sphereShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(sunModel));
            glUniformMatrix4fv(glGetUniformLocation(sphereShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(glGetUniformLocation(sphereShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
            glUniform3f(glGetUniformLocation(sphereShader.ID, "color"), 1.0f, 0.85f, 0.45f);
            glUniform3f(glGetUniformLocation(sphereShader.ID, "lightPos"), mainLightPos.x, mainLightPos.y, mainLightPos.z);
            glUniform3f(glGetUniformLocation(sphereShader.ID, "viewPos"), camera.Position.x, camera.Position.y, camera.Position.z);
            glUniform1f(glGetUniformLocation(sphereShader.ID, "isDay"), 1.0f);
            glUniform1f(glGetUniformLocation(sphereShader.ID, "sunHeight"), sunHeight);

            glBindVertexArray(sunVAO);
            glDrawElements(GL_TRIANGLES, 48 * 48 * 6, GL_UNSIGNED_INT, 0);
        }

        if (moonPos.y > -200.0f && !isDay) {
            sphereShader.Activate();
            glm::mat4 moonModel = glm::translate(glm::mat4(1.0f), moonPos);
            glUniformMatrix4fv(glGetUniformLocation(sphereShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(moonModel));
            glUniformMatrix4fv(glGetUniformLocation(sphereShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(glGetUniformLocation(sphereShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
            glUniform3f(glGetUniformLocation(sphereShader.ID, "color"), 0.75f, 0.80f, 0.95f);
            glUniform3f(glGetUniformLocation(sphereShader.ID, "lightPos"), mainLightPos.x, mainLightPos.y, mainLightPos.z);
            glUniform3f(glGetUniformLocation(sphereShader.ID, "viewPos"), camera.Position.x, camera.Position.y, camera.Position.z);
            glUniform1f(glGetUniformLocation(sphereShader.ID, "isDay"), 0.0f);
            glUniform1f(glGetUniformLocation(sphereShader.ID, "sunHeight"), sunHeight);

            glBindVertexArray(moonVAO);
            glDrawElements(GL_TRIANGLES, 36 * 36 * 6, GL_UNSIGNED_INT, 0);
        }

        if (!useFastRenderMode)
            skybox.Draw(camera, cameraFov, cameraNearPlane, cameraFarPlane);

        if (showCoordinatesInWindowTitle && currentFrame - lastTitleUpdate >= 0.1f)
        {
            const glm::vec3 normPos = glm::clamp(camera.Position / targetSceneRadius, glm::vec3(-1.0f), glm::vec3(1.0f));

            std::ostringstream title;
            title << std::fixed << std::setprecision(3);
            title << "X:" << normPos.x << " Y:" << normPos.y << " Z:" << normPos.z;
            glfwSetWindowTitle(window, title.str().c_str());
            lastTitleUpdate = currentFrame;
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &sunVAO);
    glDeleteBuffers(1, &sunVBO);
    glDeleteBuffers(1, &sunEBO);
    glDeleteVertexArrays(1, &moonVAO);
    glDeleteBuffers(1, &moonVBO);
    glDeleteBuffers(1, &moonEBO);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}