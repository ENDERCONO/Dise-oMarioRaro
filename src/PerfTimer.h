#pragma once

#include <SDL3/SDL_timer.h>

class PerfTimer
{
public:

	// Constructor
	PerfTimer();

	void Start();
	double ReadMs() const;
	Uint64 ReadTicks() const;

private:
	Uint64 startTime;

	// Frequency is the same across all timers, that's why we use a static.
	static Uint64 frequency;
};
