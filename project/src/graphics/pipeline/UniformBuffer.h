#pragma once

#include "../Buffer.h"

template <typename T>
class UniformBuffer : public Buffer
{
private:
	T m_data;
public:
	void init(Device& device, T data);
	void update();

	inline T& get() { return m_data; }
	inline VkDeviceSize size() { return sizeof(T); }
};

template<typename T>
inline void UniformBuffer<T>::init(Device& device, T data)
{
	m_data = data;
	Buffer::init(device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sizeof(T), &m_data);
}

template<typename T>
inline void UniformBuffer<T>::update()
{
	map();
	writeTo(&m_data, sizeof(T));
	unmap();
}
