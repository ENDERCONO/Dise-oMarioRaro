#include "Window.h"
#include "Log.h"
#include "Engine.h"

Window::Window() : Module()
{
	window = NULL;
	name = "window";
}

// Destructor
Window::~Window()
{
}

// Called before render is available
bool Window::Awake()
{
	LOG("Init SDL window & surface");
	bool ret = true;

	if (SDL_Init(SDL_INIT_VIDEO) != true)
	{
		LOG("SDL_VIDEO could not initialize! SDL_Error: %s\n", SDL_GetError());
		ret = false;
	}
	else
	{
		// Create window
		// L05: TODO 6: Load fullscreen/borderless/resizable/fullscreen_window
		// and the window resolution from config.xml

		Uint32 flags = 0;
		bool fullscreen = false;
		bool borderless = false;
		bool resizable = false;
		bool fullscreen_window = false;

		// TODO Get the values from the config file
		width = configParameters.child("resolution").attribute("width").as_int();
		height = configParameters.child("resolution").attribute("height").as_int();
		scale = configParameters.child("resolution").attribute("scale").as_float();

		fullscreen = configParameters.child("fullscreen").attribute("value").as_bool();
		if (fullscreen == true)        flags |= SDL_WINDOW_FULLSCREEN;

		borderless = configParameters.child("borderless").attribute("value").as_bool();
		if (borderless == true)        flags |= SDL_WINDOW_BORDERLESS;

		resizable = configParameters.child("resizable").attribute("value").as_bool();
		if (resizable == true)         flags |= SDL_WINDOW_RESIZABLE;

		fullscreen_window = configParameters.child("fullscreen_window").attribute("value").as_bool();

		// SDL3: SDL_CreateWindow(title, w, h, flags). Set position separately.
		window = SDL_CreateWindow("Platform Game", width, height, flags);

		if (window == NULL)
		{
			LOG("Window could not be created! SDL_Error: %s\n", SDL_GetError());
			ret = false;
		}
		else
		{
			if (fullscreen_window == true)
			{
				SDL_SetWindowFullscreenMode(window, nullptr); // use desktop resolution
				SDL_SetWindowFullscreen(window, true);
			}
			SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
			SDL_ShowWindow(window);
		}
	}

	return ret;
}

// Called before quitting
bool Window::CleanUp()
{
	LOG("Destroying SDL window and quitting all SDL systems");

	// Destroy window
	if (window != NULL)
	{
		SDL_DestroyWindow(window);
	}

	// Quit SDL subsystems
	SDL_Quit();
	return true;
}

// Set new window title
void Window::SetTitle(const char* new_title)
{
	//title.create(new_title);
	SDL_SetWindowTitle(window, new_title);
}

void Window::GetWindowSize(int& width, int& height) const
{
	width = this->width;
	height = this->height;
}

float Window::GetScale() const
{
	return scale;
}
