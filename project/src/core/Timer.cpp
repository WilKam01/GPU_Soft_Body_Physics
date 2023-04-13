#include "pch.h"
#include "Timer.h"

void Timer::init()
{
	m_delta = 0.0f;
	m_total = 0.0f;
	m_lastFrame = std::chrono::system_clock::now();
}

void Timer::update()
{
	m_currentFrame = std::chrono::system_clock::now();
	m_elapsedSeconds = m_currentFrame - m_lastFrame;
	m_lastFrame = m_currentFrame;

	m_delta = m_elapsedSeconds.count();
	m_total += m_delta;
}

void Timer::reset()
{
	m_lastFrame = std::chrono::system_clock::now();
	update();
}
