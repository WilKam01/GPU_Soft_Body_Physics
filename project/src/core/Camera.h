#pragma once



class Camera
{
private:
	glm::vec3 m_position;
	glm::vec3 m_rotation;
	glm::mat4 m_matrix;

	float m_fov;
	float m_aspectRatio;

	void update();
public:
	void init(glm::vec3 pos, glm::vec3 rot, float fov, float aspectRatio);
	void setPosition(glm::vec3 pos);
	void setRotation(glm::vec3 rot);
	void updateAspectRatio(float aspectRatio);

	inline glm::vec3 getPosition() { return m_position; }
	inline glm::vec3 getRotation() { return m_rotation; }
	inline glm::mat4& getMatrix() { return m_matrix; }
};

