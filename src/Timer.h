#pragma once

#include <SDL3/SDL_timer.h>

class Timer
{
public:

	// Constructor
	Timer();

	void Start();
	int ReadSec() const;
	float ReadMSec() const;

private:
	Uint64 startTime;
};
