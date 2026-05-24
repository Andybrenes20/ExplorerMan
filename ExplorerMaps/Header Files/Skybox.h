#ifndef SKYBOX_CLASS_H
#define SKYBOX_CLASS_H

#include <string>
#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "Camara.h"
#include "shaderClass.h"

class Skybox
{
public:
	Skybox(const std::vector<std::string>& facePaths, const char* vertexShaderPath, const char* fragmentShaderPath);
	~Skybox();

	void Draw(const Camera& camera, float FOVdeg, float nearPlane, float farPlane);

private:
	GLuint VAO = 0;
	GLuint VBO = 0;
	GLuint cubemapTexture = 0;
	Shader shader;

	void setupCube();
	static std::vector<unsigned char> BuildProceduralFace(int width, int height, int faceIndex);
	static glm::vec3 DirectionForFace(int faceIndex, float u, float v);
};

#endif
