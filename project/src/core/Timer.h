#pragma once



class Timer
{
private:
	float m_delta;
	float m_total;

	std::chrono::system_clock::time_point m_lastFrame;
	std::chrono::system_clock::time_point m_currentFrame;
	std::chrono::duration<float> m_elapsedSeconds;
public:
	void init();
	void update();
	void reset();

	inline float getDT() { return m_delta; }
	inline float getTotal() { return m_total; }
};