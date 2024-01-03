#pragma once
#include <vector>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

const GLfloat YAW = -90.0f;//转到Z轴负方向
const GLfloat PITCH = 0.0f;
const GLfloat ZOOM = 45.0f;
const GLfloat SPEED = 6.0f;
enum Camera_Movement {
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT
};

class Camera {
public :
	Camera (glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), 
		glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), GLfloat yaw = YAW, 
		GLfloat pitch = PITCH):front(glm::vec3(0.0f, 0.0f, -1.0f)), zoom(ZOOM), speed(SPEED)
	{
		this->position = position;
		this->worldup = up;
		this->yaw = yaw;
		this->pitch = pitch;
		UpdateCameraVectors();
	}
	glm::mat4 GetViewMartix() {
		return glm::lookAt(position, position + front, up);
	}
	GLfloat GetZoom() {
		return zoom;
	}
	void ProcessKeyboard(Camera_Movement direction, float deltaTime) {
		float velocity = speed * deltaTime;
		if (direction == FORWARD) {
			position += front * velocity;
		}
		if (direction == BACKWARD) {
			position -= front * velocity;
		}
		if (direction == RIGHT) {
			position += glm::normalize(glm::cross(front, up)) * velocity;
		}
		if (direction == LEFT) {
			position -= glm::normalize(glm::cross(front, up)) * velocity;
		}

	}
	void ProcessMouseMove(GLfloat xOffset, GLfloat yOffset) {
		GLfloat mouseSensitity = 0.1f;
		xOffset *= mouseSensitity;
		yOffset *= mouseSensitity;
		yaw += xOffset;
		pitch += yOffset;
		UpdateCameraVectors();
	}
	bool isZoomed = false; //表示摄像机是否处于放大状态
	glm::vec3 getPosition() {
		return position;
	}
	glm::vec3 getFront() {
		return front;
	}
	void setPosition(const glm::vec3& newPosition) {
		position = newPosition;
	}

	void setFront(const glm::vec3& newFront) {
		front = newFront;
	}

private:
	GLfloat yaw, pitch;
	glm::vec3 position;
	glm::vec3 front, up, right, worldup;
	GLfloat zoom;
	GLfloat speed;
	

	void UpdateCameraVectors() {
		glm::vec3 front;
		front.x = cos(glm::radians(pitch)) * cos(glm::radians(yaw));
		front.y = sin(glm::radians(pitch));
		front.z = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
		this->front = glm::normalize(front);
		this->up = glm::normalize(glm::cross(this -> right, this -> front));
		this->right = glm::normalize(glm::cross(this->front, this->worldup));

	}
	


};

