#include "pch.h"
#include "Timer.h"

void Timer::init(float fixedTimestep)
{
	m_delta = 0.0f;
	m_total = 0.0f;
	m_fixedDelta = fixedTimestep;
	m_accumulator = 0.0f;
	m_lastFrame = std::chrono::system_clock::now();
}

void Timer::update()
{
	m_currentFrame = std::chrono::system_clock::now();
	m_elapsedSeconds = m_currentFrame - m_lastFrame;
	m_lastFrame = m_currentFrame;

	m_delta = m_elapsedSeconds.count();
	m_total += m_delta;
	m_accumulator += m_delta;
}

void Timer::reset()
{
	m_lastFrame = std::chrono::system_clock::now();
	update();
}

bool Timer::passedFixedDT()
{
	bool result = m_accumulator > m_fixedDelta;
	if (result)
		m_accumulator -= m_fixedDelta;
	return result;
}
