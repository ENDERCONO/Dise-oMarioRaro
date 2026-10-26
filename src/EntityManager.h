#pragma once

#include "Module.h"
#include "Entity.h"
#include <list>

class EntityManager : public Module
{
public:

	EntityManager();

	// Destructor
	virtual ~EntityManager();

	// Called before render is available
	bool Awake();

	// Called after Awake. Initialises anything created during Awake().
	bool Start();

	// Called every frame -- logic only, via Entity::Update()
	bool Update(float dt);

	// Called every frame, after Update -- rendering only, via Entity::Draw()
	bool PostUpdate();

	// Called before quitting
	bool CleanUp();

	// Additional methods
	std::shared_ptr<Entity> CreateEntity(EntityType type);

	void DestroyEntity(std::shared_ptr<Entity> entity);

	void AddEntity(std::shared_ptr<Entity> entity);

private:

	// Runs Awake() on every entity in a pending batch, then Start() on every
	// entity in that same batch -- every Awake() completes before any Start(),
	// so a Start() can safely reference another entity created in the same
	// batch. Anything created during that Start() lands in the next batch.
	void InitialisePending();

public:

	std::list<std::shared_ptr<Entity>> entities;

private:

	// Created but not yet initialised -- entities created after startup (e.g.
	// by Scene::Awake()) wait here until the next InitialisePending() call.
	std::list<std::shared_ptr<Entity>> pending;
};
