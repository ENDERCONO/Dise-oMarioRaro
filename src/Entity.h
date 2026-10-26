#pragma once

#include <memory>
#include <string>
#include "Vector2D.h"
#include "pugixml.hpp"

// L08: TODO 2: Add Physics Module
// Add forward declaration to avoid circular dependency
class PhysBody;

enum class EntityType
{
	PLAYER,
	ITEM,
	UNKNOWN
};

class Entity : public std::enable_shared_from_this<Entity>
{
public:

	Entity() {}
	Entity(EntityType type) : type(type), active(true) {}

	virtual bool Awake()
	{
		return true;
	}

	virtual bool Start()
	{
		return true;
	}

	// Logic only. EntityManager calls this from Update(), before any Draw().
	virtual bool Update(float dt)
	{
		return true;
	}

	// Rendering only. EntityManager calls this from PostUpdate(), once every
	// entity's Update() for the frame has already run.
	virtual bool Draw()
	{
		return true;
	}

	virtual bool CleanUp()
	{
		return true;
	}

	// L10: TODO 4: Store this entity's slice of config.xml, mirroring
	// Module::LoadParameters
	virtual void SetParameters(pugi::xml_node p) {
		parameters = p; 
	}

	virtual bool Destroy()
	{
		pendingToDelete = true;
		return true;
	}

	// L08: TODO 2: Add Physics Module
	// Add OnCollision()/OnCollisionEnd() when a shape on this
	// entity's body starts/stops touching another shape. 

	virtual void OnCollision(PhysBody* physA, PhysBody* physB) {}
	virtual void OnCollisionEnd(PhysBody* physA, PhysBody* physB) {}

	void Enable()
	{
		if (!active)
		{
			active = true;
			Start();
		}
	}

	void Disable()
	{
		if (active)
		{
			active = false;
			CleanUp();
		}
	}

public:

	std::string name;
	EntityType type = EntityType::UNKNOWN;
	bool active = true;
	bool pendingToDelete = false;

	// Possible properties, it depends on how generic we
	// want our Entity class, maybe it's not renderable...
	Vector2D position;
	bool renderable = true;

	// L10: TODO 4: This entity's slice of config.xml, set via SetParameters()
	pugi::xml_node parameters;
};
