#pragma once

#include "Module.h"
#include <SDL3/SDL.h>

class Window : public Module
{
public:

	Window();

	// Destructor
	virtual ~Window();

	// Called before render is available
	bool Awake();

	// Called before quitting
	bool CleanUp();

	// Changae title
	void SetTitle(const char* title);

	// Retrive window size
	void GetWindowSize(int& width, int& height) const;

	// Retrieve window scale
	float GetScale() const;

public:
	// The window we'll be rendering to
	SDL_Window* window;

	std::string title;
	int width = 1280;
	int height = 720;
	float scale = 1.0f;
};
