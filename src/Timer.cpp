// ----------------------------------------------------
// Timer with millisecond precision
// ----------------------------------------------------

#include "Timer.h"

// L02: TODO 1: Fill Start(), ReadSec() and ReadMSec() methods
// they are simple, one line each!

Timer::Timer()
{
	Start();
}

void Timer::Start()
{
	startTime = SDL_GetTicks();
}

int Timer::ReadSec() const
{
	return (int)((SDL_GetTicks() - startTime) / 1000);
}

float Timer::ReadMSec() const
{
	return (float)(SDL_GetTicks() - startTime);
}
