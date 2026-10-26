#pragma once

#include "Module.h"
#include <memory>

class Player;

class Scene : public Module
{
public:

	Scene();

	// Destructor
	virtual ~Scene();

	// Called before render is available
	bool Awake();

	// Called before the first frame
	bool Start();

	// Called before all Updates
	bool PreUpdate();

	// Called each loop iteration
	bool Update(float dt);

	// Called before all Updates
	bool PostUpdate();

	// Called before quitting
	bool CleanUp();

private:
	// L04: TODO 3a: Declare a player as a member of Scene.h
	std::shared_ptr<Player> player;
};