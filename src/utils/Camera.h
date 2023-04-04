#pragma once

#include <glm/glm.hpp>

typedef struct Camera {
	glm::vec3 pos;
	glm::vec3 lookAt;
	glm::vec3 upDir;
	glm::vec3 forwardDir;
	float cameraYaw;
	float cameraPitch;
} Camera;