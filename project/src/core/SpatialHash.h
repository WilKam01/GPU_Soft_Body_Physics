#pragma once



class SpatialHash
{
private:
	float m_spacing;
	uint32_t m_tableSize;
	std::vector<uint32_t> m_cellStart;
	std::vector<uint32_t> m_cellEntries;

	glm::ivec3 intPos(glm::vec3 pos);
	uint32_t hashPos(glm::vec3 pos);
public:
	void init(float spacing, std::vector<glm::vec3> positions);

	std::vector<uint32_t> query(glm::vec3 pos, float maxDist);
};

