#pragma once

class Timer
{
private:
	static const int FRAME_COUNT_AVG = 250; // Frame count to count average dt

	float m_delta;
	float m_total;

	std::array<float, FRAME_COUNT_AVG> m_averageDelta;
	uint32_t m_averageDeltaIndex = 0;

	float m_fixedDelta;
	float m_accumulator;

	std::chrono::system_clock::time_point m_lastFrame;
	std::chrono::system_clock::time_point m_currentFrame;
	std::chrono::duration<float> m_elapsedSeconds;
public:
	void init(float fixedTimestep);
	void update();
	void reset();
	
	bool passedFixedDT();
	void setFixedDT(float fixedDT);
	float getAverageDT();

	inline float getDT() { return m_delta; }
	inline float getTotal() { return m_total; }
};