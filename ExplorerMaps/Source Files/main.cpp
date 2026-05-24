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

const unsigned int width = 1920;
const unsigned int height = 1080;
const float targetSceneRadius = 1800.0f;
const float cameraFov = 55.0f;
const float cameraNearPlane = 0.05f;
const float cameraFarPlane = 6000.0f;
const bool showCoordinatesInWindowTitle = true;  // Activado para mostrar coordenadas normalizadas
const bool useFastRenderMode = false;

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

	glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	glm::vec3 lightPos = glm::vec3(0.5f, 0.5f, 0.5f);
	glm::mat4 lightModel = glm::mat4(1.0f);
	lightModel = glm::translate(lightModel, lightPos);

	shaderProgram.Activate();
	glUniform4f(glGetUniformLocation(shaderProgram.ID, "lightColor"), lightColor.x, lightColor.y, lightColor.z, lightColor.w);
	glUniform3f(glGetUniformLocation(shaderProgram.ID, "lightPos"), lightPos.x, lightPos.y, lightPos.z);

	Model model("modelos/city.glb");
	Skybox skybox(
		{},
		"Shaders/skybox_cubemap.vert",
		"Shaders/skybox_cubemap.frag"
	);

	glm::vec3 modelCenter = model.GetCenter();
	float modelRadius = model.GetRadius();
	if (modelRadius < 1.0f)
	{
		modelRadius = 1.0f;
	}

	const float normalizationScale = targetSceneRadius / modelRadius;
	const glm::mat4 modelTransform =
		glm::scale(glm::mat4(1.0f), glm::vec3(normalizationScale)) *
		glm::translate(glm::mat4(1.0f), -modelCenter);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);

	// Cámara posicionada en (112, -179, 600) mirando hacia la derecha (+X)
	Camera camera(width, height, glm::vec3(112.0f, -179.0f, 600.0f));
	camera.Orientation = glm::normalize(glm::vec3(1.0f, 0.0f, 0.0f));  // Mirando hacia la derecha
	camera.speed = 320.0f;
	float lastFrame = 0.0f;
	float lastTitleUpdate = 0.0f;
	glm::vec3 lastTitlePosition = camera.Position;

	while (!glfwWindowShouldClose(window))
	{
		float currentFrame = static_cast<float>(glfwGetTime());
		float deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// El mouse SOLO controla la dirección de la vista
		camera.Inputs(window, deltaTime);
		camera.updateMatrix(cameraFov, cameraNearPlane, cameraFarPlane);

		// Mostrar coordenadas NORMALIZADAS de la posición de la cámara (como en Minecraft)
		if (showCoordinatesInWindowTitle &&
			currentFrame - lastTitleUpdate >= 0.1f &&  // Actualizar 10 veces por segundo
			glm::distance(camera.Position, lastTitlePosition) >= 1.0f)
		{
			// Normalizar la posición de la cámara al rango [-1, 1] basado en el tamaño de la escena
			const glm::vec3 normalizedCameraPosition = glm::clamp(
				camera.Position / targetSceneRadius,
				glm::vec3(-1.0f),
				glm::vec3(1.0f)
			);

			std::ostringstream windowTitle;
			windowTitle << std::fixed << std::setprecision(3);
			windowTitle << "Coordenadas[X:" << normalizedCameraPosition.x
				<< " Y:" << normalizedCameraPosition.y
				<< " Z:" << normalizedCameraPosition.z << "]";

			windowTitle << " | Mundo [X:" << std::setprecision(0)
				<< camera.Position.x << " Y:" << camera.Position.y
				<< " Z:" << camera.Position.z << "]";

			glfwSetWindowTitle(window, windowTitle.str().c_str());
			lastTitleUpdate = currentFrame;
			lastTitlePosition = camera.Position;
		}

		model.Draw(shaderProgram, camera, modelTransform);
		if (!useFastRenderMode)
		{
			skybox.Draw(camera, cameraFov, cameraNearPlane, cameraFarPlane);
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	shaderProgram.Delete();
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}


//XD