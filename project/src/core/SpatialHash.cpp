#include "pch.h"
#include "SpatialHash.h"

glm::ivec3 SpatialHash::intPos(glm::vec3 pos)
{
	return glm::floor(pos / m_spacing);
}

uint32_t SpatialHash::hashPos(glm::vec3 pos)
{
	glm::ivec3 p = intPos(pos);
	return std::abs((p.x * 92837111) ^ (p.y * 689287499) ^ (p.z * 283923481)) % m_tableSize; // Fantasy function (from https://github.com/matthias-research/pages/blob/master/tenMinutePhysics/11-hashing.html) 
}

void SpatialHash::init(float spacing, std::vector<glm::vec3> positions)
{
	uint32_t count = (uint32_t)positions.size();

	m_spacing = spacing;
	m_tableSize = 2 * count;
	m_cellStart.resize(m_tableSize + 1);
	m_cellEntries.resize(count);

	for (uint32_t i = 0; i < count; i++)
		m_cellStart[hashPos(positions[i])]++;

	uint32_t start = 0;
	for (uint32_t i = 0; i < m_tableSize; i++)
	{
		start += m_cellStart[i];
		m_cellStart[i] = start;
	}
	m_cellStart.back() = start;

	for (uint32_t i = 0; i < count; i++)
		m_cellEntries[--m_cellStart[hashPos(positions[i])]] = i;
}

std::vector<uint32_t> SpatialHash::query(glm::vec3 pos, float maxDist)
{
	glm::ivec3 p0 = intPos(pos - maxDist);
	glm::ivec3 p1 = intPos(pos + maxDist);

	std::vector<uint32_t> queries;
	std::set<uint32_t> usedIds;

	for (int x = p0.x; x <= p1.x; x++)
	{
		for (int y = p0.y; y <= p1.y; y++)
		{
			for (int z = p0.z; z <= p1.z; z++)
			{
				uint32_t id = hashPos(glm::vec3(x, y, z) * m_spacing);
				if (!usedIds.count(id))
				{
					uint32_t start = m_cellStart[id];
					uint32_t end = m_cellStart[id + 1];
					for (uint32_t i = start; i < end; i++)
						queries.push_back(m_cellEntries[i]);

					usedIds.insert(id);
				}

			}
		}
	}
	return queries;
}

