#include"Camara.h"

Camera::Camera(int width, int height, glm::vec3 position)
{
	Camera::width = width;
	Camera::height = height;
	Position = position;
}

void Camera::updateMatrix(float FOVdeg, float nearPlane, float farPlane)
{
	// Initializes matrices since otherwise they will be the null matrix
	glm::mat4 view = glm::mat4(1.0f);
	glm::mat4 projection = glm::mat4(1.0f);

	// Makes camera look in the right direction from the right position
	view = glm::lookAt(Position, Position + Orientation, Up);
	// Adds perspective to the scene
	projection = glm::perspective(glm::radians(FOVdeg), (float)width / height, nearPlane, farPlane);

	// Sets new camera matrix
	cameraMatrix = projection * view;
}

void Camera::Matrix(Shader& shader, const char* uniform)
{
	// Exports camera matrix
	glUniformMatrix4fv(shader.GetUniformLocation(uniform), 1, GL_FALSE, glm::value_ptr(cameraMatrix));
}

glm::mat4 Camera::GetViewMatrix() const
{
	return glm::lookAt(Position, Position + Orientation, Up);
}

glm::mat4 Camera::GetProjectionMatrix(float FOVdeg, float nearPlane, float farPlane) const
{
	return glm::perspective(glm::radians(FOVdeg), static_cast<float>(width) / static_cast<float>(height), nearPlane, farPlane);
}

void Camera::Inputs(GLFWwindow* window, float deltaTime)
{
	if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
	{
		if (!toggleLatch)
		{
			flyMode = !flyMode;
			toggleLatch = true;
		}
	}
	else
	{
		toggleLatch = false;
	}

	float currentSpeed = speed * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
	{
		currentSpeed *= 2.2f;
	}

	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
	{
		flyMode = true;
	}

	glm::vec3 forward = Orientation;
	if (glm::length(forward) > 0.0001f)
	{
		forward = glm::normalize(forward);
	}
	else
	{
		forward = glm::vec3(0.0f, 0.0f, -1.0f);
	}

	glm::vec3 moveForward = forward;
	if (!flyMode)
	{
		moveForward = glm::vec3(forward.x, 0.0f, forward.z);
		if (glm::length(moveForward) > 0.0001f)
		{
			moveForward = glm::normalize(moveForward);
		}
		else
		{
			moveForward = glm::vec3(0.0f, 0.0f, -1.0f);
		}
	}

	glm::vec3 right = glm::cross(moveForward, Up);
	if (glm::length(right) > 0.0001f)
	{
		right = glm::normalize(right);
	}
	else
	{
		right = glm::vec3(1.0f, 0.0f, 0.0f);
	}

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
	{
		Position += currentSpeed * moveForward;
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
	{
		Position += currentSpeed * -right;
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
	{
		Position += currentSpeed * -moveForward;
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
	{
		Position += currentSpeed * right;
	}
	if (flyMode && glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
	{
		Position += currentSpeed * Up;
	}
	if (flyMode && glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
	{
		Position += currentSpeed * -Up;
	}

	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
	{
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

		if (firstClick)
		{
			glfwSetCursorPos(window, (width / 2), (height / 2));
			firstClick = false;
		}

		double mouseX;
		double mouseY;
		glfwGetCursorPos(window, &mouseX, &mouseY);

		float rotX = sensitivity * (float)(mouseY - (height / 2)) / height;
		float rotY = sensitivity * (float)(mouseX - (width / 2)) / width;

		glm::vec3 pitchAxis = glm::cross(forward, Up);
		if (glm::length(pitchAxis) > 0.0001f)
		{
			pitchAxis = glm::normalize(pitchAxis);
		}
		else
		{
			pitchAxis = glm::vec3(1.0f, 0.0f, 0.0f);
		}

		glm::mat4 rotationMatrixX = glm::rotate(glm::mat4(1.0f), glm::radians(-rotX), pitchAxis);
		glm::vec3 newOrientation = glm::vec3(rotationMatrixX * glm::vec4(forward, 0.0f));

		float orientationDot = glm::clamp(glm::dot(glm::normalize(newOrientation), glm::normalize(Up)), -1.0f, 1.0f);
		float orientationAngle = std::acos(orientationDot);
		if (abs(orientationAngle - glm::radians(90.0f)) <= glm::radians(85.0f))
		{
			forward = glm::normalize(newOrientation);
		}

		glm::mat4 rotationMatrixY = glm::rotate(glm::mat4(1.0f), glm::radians(-rotY), Up);
		Orientation = glm::normalize(glm::vec3(rotationMatrixY * glm::vec4(forward, 0.0f)));

		glfwSetCursorPos(window, (width / 2), (height / 2));
	}
	else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE)
	{
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		firstClick = true;
	}
}
