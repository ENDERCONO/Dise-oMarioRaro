#include "EntityManager.h"
#include "Player.h"
#include "Item.h"
#include "Engine.h"
#include "Textures.h"
#include "Scene.h"
#include "Log.h"

EntityManager::EntityManager() : Module()
{
	name = "entitymanager";
}

// Destructor
EntityManager::~EntityManager()
{}

// Called before render is available
bool EntityManager::Awake()
{
	LOG("Loading Entity Manager");
	return true;
}

bool EntityManager::Start()
{
	// Entities created during Awake() (e.g. Scene::Awake() creating the
	// player) are still pending -- initialise them before the first Update().
	InitialisePending();
	return true;
}

// Called before quitting
bool EntityManager::CleanUp()
{
	bool ret = true;

	for (const auto& entity : entities)
	{
		ret = ret && entity->CleanUp();
	}

	entities.clear();
	pending.clear();

	return ret;
}

std::shared_ptr<Entity> EntityManager::CreateEntity(EntityType type)
{
	std::shared_ptr<Entity> entity = nullptr;

	// L04: TODO 3a: Instantiate the entity for this type, add it to the
	// pending queue, and return it -- or return nullptr for an unknown type
	switch (type)
	{
	case EntityType::PLAYER:
		entity = std::make_shared<Player>();
		break;
	case EntityType::ITEM:
		entity = std::make_shared<Item>();
		break;
	default:
		LOG("Error in EntityManager::CreateEntity: Unknown entity type");
		return nullptr;
	}

	pending.push_back(entity);

	return entity;
}

void EntityManager::DestroyEntity(std::shared_ptr<Entity> entity)
{
	entity->CleanUp();
	entities.remove(entity);
}

void EntityManager::AddEntity(std::shared_ptr<Entity> entity)
{
	if (entity != nullptr) pending.push_back(entity);
}

void EntityManager::InitialisePending()
{
	if (pending.empty()) return;

	std::list<std::shared_ptr<Entity>> batch;
	batch.swap(pending); // anything created during Start() lands in the next batch

	for (const auto& entity : batch) entity->Awake(); // all Awake first...
	for (const auto& entity : batch) entity->Start(); // ...then all Start

	entities.splice(entities.end(), batch);
}

bool EntityManager::Update(float dt)
{
	InitialisePending();

	bool ret = true;

	// List to store entities pending deletion
	std::list<std::shared_ptr<Entity>> pendingDelete;

	// Iterates over the entities and calls Update -- logic only
	for (const auto& entity : entities)
	{
		// If the entity is marked for deletion, add it to the pendingDelete list
		if (entity->pendingToDelete)
		{
			pendingDelete.push_back(entity);
		}
		// If the entity is not active, skip it
		if (entity->active == false) continue;
		ret = ret && entity->Update(dt);
	}

	// Now iterates over the pendingDelete list and destroys the entities
	for (const auto& entity : pendingDelete)
	{
		DestroyEntity(entity);
	}

	return ret;
}

bool EntityManager::PostUpdate()
{
	bool ret = true;

	// Rendering only, once every entity's Update() for this frame has run
	for (const auto& entity : entities)
	{
		if (entity->active == false) continue;
		if (entity->renderable == false) continue;
		ret = ret && entity->Draw();
	}

	return ret;
}
