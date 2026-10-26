#pragma once

#include "Entity.h"
#include <SDL3/SDL.h>

struct SDL_Texture;

class Item : public Entity
{
public:

	Item();
	virtual ~Item();

	bool Awake();

	bool Start();

	bool Update(float dt);

	bool Draw();

	bool CleanUp();

	bool Destroy();

private:

	SDL_Texture* texture;
	int texW, texH;

	// L08: TODO 4: Add a physics body to the item
	PhysBody* pbody = nullptr;
};
