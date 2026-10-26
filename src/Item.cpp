#include "Item.h"
#include "Engine.h"
#include "Textures.h"
#include "Render.h"
#include "Log.h"
#include "Physics.h"

Item::Item() : Entity(EntityType::ITEM)
{
	name = "item";
}

Item::~Item() {}

bool Item::Awake() {
	return true;
}

bool Item::Start() {
	texture = Engine::GetInstance().textures->Load("Assets/Textures/goldCoin.png");
	Engine::GetInstance().textures->GetSize(texture, texW, texH);

	// L08: TODO 4: Add a physics to an item - initialize the physics body
	pbody = Engine::GetInstance().physics->CreateCircle((int)position.getX() + texW / 2, (int)position.getY() + texH / 2, texW / 2, bodyType::DYNAMIC);
	pbody->listener = this; // Set the listener to this item instance, it will useful for accesing the class when a collision happens

	// L08: TODO 7: Assign collider type
	pbody->ctype = ColliderType::ITEM;

	return true;
}

bool Item::Update(float dt)
{
	// L08: TODO 4: Add a physics to an item - update the position of the object from the physics.
	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	return true;
}

bool Item::Draw()
{
	// L08: TODO 4: Add a physics to an item - update the position of the object from the physics.
	Engine::GetInstance().render->DrawTexture(texture, (int)position.getX() - texW / 2, (int)position.getY() - texH / 2);
	return true;
}

bool Item::CleanUp()
{
	// Release the physics body first, then the texture
	if (pbody != nullptr)
	{
		pbody->listener = nullptr;
		Engine::GetInstance().physics->DeletePhysBody(pbody);
		pbody = nullptr;
	}

	if (texture != nullptr)
	{
		Engine::GetInstance().textures->UnLoad(texture);
		texture = nullptr;
	}
	return true;
}

bool Item::Destroy()
{
	LOG("Destroying item");
	pendingToDelete = true;
	return true;
}
