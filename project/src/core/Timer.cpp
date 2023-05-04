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

	m_averageDelta[m_averageDeltaIndex] = m_delta;
	m_averageDeltaIndex = (m_averageDeltaIndex + 1) % FRAME_COUNT_AVG;
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

void Timer::setFixedDT(float fixedDT)
{
	m_fixedDelta = fixedDT;
}

float Timer::getAverageDT()
{
	float average = 0.0f;
	for (auto& dt : m_averageDelta)
		average += dt;
	return average / FRAME_COUNT_AVG;
}
