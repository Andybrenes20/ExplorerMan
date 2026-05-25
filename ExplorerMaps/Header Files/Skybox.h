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
	Skybox(
		const std::vector<std::string>& dayFacePaths,
		const std::vector<std::string>& nightFacePaths,
		const char* vertexShaderPath,
		const char* fragmentShaderPath
	);
	~Skybox();

	void Draw(const Camera& camera, float FOVdeg, float nearPlane, float farPlane);
	void SetBlendFactor(float factor);
	static std::vector<unsigned char> BuildProceduralFace(int width, int height, int faceIndex, bool nightTheme);

private:
	GLuint VAO = 0;
	GLuint VBO = 0;
	GLuint dayCubemapTexture = 0;
	GLuint nightCubemapTexture = 0;
	float blendFactor = 0.0f;
	Shader shader;

	void setupCube();
	static glm::vec3 DirectionForFace(int faceIndex, float u, float v);
};

#endif
