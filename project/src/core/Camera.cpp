#include "pch.h"
#include "Camera.h"

void Camera::update()
{
	glm::mat4 rotMatrix =
		glm::rotate(glm::mat4(1.0f), glm::radians(m_rotation.z), glm::vec3(0.0f, 0.0f, 1.0f)) *
		glm::rotate(glm::mat4(1.0f), glm::radians(m_rotation.y), glm::vec3(0.0f, 1.0f, 0.0f)) *
		glm::rotate(glm::mat4(1.0f), glm::radians(m_rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	glm::vec3 lookDir = rotMatrix * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);

	m_matrix = glm::perspective(glm::radians(m_fov), m_aspectRatio, 0.1f, 100.0f);
	m_matrix *= glm::lookAt(m_position, m_position + lookDir, glm::vec3(0.0f, 1.0f, 0.0f));
}

void Camera::init(glm::vec3 pos, glm::vec3 rot, float fov, float aspectRatio)
{
	m_position = pos;
	m_rotation = rot;
	m_fov = fov;
	m_aspectRatio = aspectRatio;
	update();
}

void Camera::setPosition(glm::vec3 pos)
{
	bool shouldUpdate = pos != m_position;
	if (shouldUpdate)
	{
		m_position = pos;
		update();
	}
}

void Camera::setRotation(glm::vec3 rot)
{
	bool shouldUpdate = rot != m_rotation;
	if (shouldUpdate)
	{
		m_rotation = rot;
		update();
	}
}

void Camera::updateAspectRatio(float aspectRatio)
{
	m_aspectRatio = aspectRatio;
	update();
}
