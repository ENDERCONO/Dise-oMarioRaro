#include <vector>
#include "Player.h"
#include "Engine.h"
#include "Textures.h"
#include "Input.h"
#include "Render.h"
#include "Log.h"
#include "Physics.h"
#include "Audio.h"
#include "Map.h"

Player::Player() : Entity(EntityType::PLAYER)
{
	name = "Player";
}

Player::~Player()
{
}

bool Player::Awake()
{
	// L04: TODO 2: Initialize the player's position
	position = Vector2D(300.0f, 300.0f);
	return true;
}

bool Player::Start()
{
	// L10: TODO 5: Read texture, frame size and tuning from config.xml
	// instead of hardcoding them. GetSize() is not the fix for texW/texH --
	// the real texture (a spritesheet) would return the whole sheet's size,
	// not one frame's.
	// L04: TODO 2: Load the player texture
	texture = Engine::GetInstance().textures->Load(parameters.child("texture").attribute("path").as_string());

	texW = parameters.child("frame").attribute("width").as_int();
	texH = parameters.child("frame").attribute("height").as_int();


	// L10: TODO 5: Load this entity's animations from a TSX and set the current animation to "idle".
	// ...

	anims.LoadFromTSX(parameters.child("animations").attribute("tsx").as_string(), { {0, "idle"}, {11, "move"}, {22, "jump"} });
	anims.SetCurrent("idle");
	speed = parameters.child("movement").attribute("speed").as_float(speed);
	// Initialize audio effect
	pickCoinFxId = Engine::GetInstance().audio->LoadFx("Assets/Audio/Fx/coin-collision-sound-342335.wav");

	// L08: TODO 5: Add physics to the player - create the physics body: a CAPSULE (CreateCapsule) with fixed rotation, so the player does not roll
	// Capsule + fixed rotation: no rolling, and no bumping over the seams between tile rectangles
	pbody = Engine::GetInstance().physics->CreateCapsule((int)position.getX() + texW / 2, (int)position.getY() + texH / 2, texW / 2, texH, bodyType::DYNAMIC);

	// L08: TODO 6: Assign player class (using "this") to the listener of the pbody. This makes the Physics module to call the OnCollision method
	pbody->listener = this;

	// L08: TODO 7: Assign collider type
	pbody->ctype = ColliderType::PLAYER;

	return true;
}

bool Player::Update(float dt)
{
	// L08: TODO 5: Add physics to the player - updated player position using physics

	isGround = CheckGround();

	velocity = Engine::GetInstance().physics->GetLinearVelocity(pbody);
	velocity = { 0, velocity.y }; // Reset horizontal velocity by default

	Input* input = Engine::GetInstance().input.get();
	bool moving = false;

	if (input->GetKey(SDL_SCANCODE_A) == KEY_REPEAT) {
		velocity.x = -speed;
		moving = true;
	}
	if (input->GetKey(SDL_SCANCODE_D) == KEY_REPEAT) {
		velocity.x = speed;
		moving = true;
	}

	// Jump only when standing on the ground (W or Space). Y axis points down.
	if (isGround && (input->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN || input->GetKey(SDL_SCANCODE_W) == KEY_DOWN)) {
		velocity.y = -jumpSpeed;
		isGround = false;
	}

	// Animation state: air -> jump, ground + moving -> move, otherwise idle
	if (!isGround) anims.SetCurrent("jump");
	else if (moving) anims.SetCurrent("move");
	else anims.SetCurrent("idle");

	// Apply velocity via helper
	Engine::GetInstance().physics->SetLinearVelocity(pbody, velocity);

	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	// L10: TODO 8: Camera follow
	// ..

	Vector2D mapSize = Engine::GetInstance().map->GetMapSizeInPixels();
	float camW = Engine::GetInstance().render->camera.w;
	float limitLeft = camW / 4;
	float limitRight = mapSize.getX() - camW * 3.0f / 4.0f;

	if (position.getX() > limitLeft || position.getX() < limitRight)
	{
		Engine::GetInstance().render->camera.x = camW / 4.0f - position.getX();
	}



	// L10: TODO 6: Advance the animation frame
	// ..
	anims.Update(dt);


	return true;
}

bool Player::Draw()
{
	// L08: TODO 5: Add physics to the player - updated player position using physics
	// L04: TODO 4: Render the player texture at its current position

	// L10: TODO 6: Draw the current animation frame instead of the whole spritesheet
	// ..
	SDL_Rect currentFrame = anims.GetCurrentFrame();
	Engine::GetInstance().render->DrawTexture(texture, (int)position.getX() - texW / 2, (int)position.getY() - texH / 2, &currentFrame);

	return true;
}

bool Player::CleanUp()
{
	LOG("Cleanup player");
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

// Standing on something? Look at the live contacts and accept a platform
// whose contact normal points mostly downward (screen Y axis points down).
bool Player::CheckGround() const
{
	b2BodyId me = pbody->body;
	int cap = b2Body_GetContactCapacity(me);
	if (cap <= 0) return false;
	std::vector<b2ContactData> contacts(cap);
	int n = b2Body_GetContactData(me, contacts.data(), cap);
	for (int i = 0; i < n; ++i)
	{
		const b2ContactData& cd = contacts[i];
		if (cd.manifold.pointCount == 0) continue;
		bool iAmA = B2_ID_EQUALS(b2Shape_GetBody(cd.shapeIdA), me);
		b2ShapeId other = iAmA ? cd.shapeIdB : cd.shapeIdA;
		if (b2Shape_IsSensor(other)) continue;
		PhysBody* pb = (PhysBody*)b2Body_GetUserData(b2Shape_GetBody(other));
		if (!pb || pb->ctype != ColliderType::PLATFORM) continue;
		// normal points A -> B; ground is below me, i.e. +Y from my side
		float ny = iAmA ? cd.manifold.normal.y : -cd.manifold.normal.y;
		if (ny > 0.7f) return true;
	}
	return false;
}

void Player::OnCollision(PhysBody* physA, PhysBody* physB)
{
	// Handle collision with other physics bodies
	// For example, if the player collides with an item, you can implement logic here
	switch (physB->ctype)
	{
	case ColliderType::PLATFORM:
		LOG("Collision PLATFORM");
		break;
	case ColliderType::ITEM:

		//Play the coin pickup sound effect 
		Engine::GetInstance().audio->PlayFx(pickCoinFxId);

		// Destroy the item. 
		physB->listener->Destroy();

		LOG("Collision ITEM");

		break;
	case ColliderType::UNKNOWN:
		LOG("Collision UNKNOWN");
		break;
	default:
		break;
	}
}