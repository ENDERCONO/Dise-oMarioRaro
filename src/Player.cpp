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

#include <algorithm>
#include <cstdio>

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

	// No friction on the player: velocity.x is set directly every frame, so friction
	// is never needed on the ground, and it is what made the player stick to walls.
	// Sleeping is disabled too, a resting sleeping body gave flaky ground contacts.
	b2ShapeId shapes[4];
	int shapeCount = b2Body_GetShapes(pbody->body, shapes, 4);
	for (int i = 0; i < shapeCount; ++i) b2Shape_SetFriction(shapes[i], 0.0f);
	b2Body_EnableSleep(pbody->body, false);
	Engine::GetInstance().physics->SetDebugColor(pbody, 0x33CCFF); // cyan in F9 view

	// L08: TODO 6: Assign player class (using "this") to the listener of the pbody. This makes the Physics module to call the OnCollision method
	pbody->listener = this;

	// L08: TODO 7: Assign collider type
	pbody->ctype = ColliderType::PLAYER;

	return true;
}

bool Player::Update(float dt)
{
	// L08: TODO 5: Add physics to the player - updated player position using physics

	Input* input = Engine::GetInstance().input.get();
	Physics* physics = Engine::GetInstance().physics.get();

	// F10: god mode (invulnerable + free flight)
	if (input->GetKey(SDL_SCANCODE_F10) == KEY_DOWN) {
		godMode = !godMode;
		dashing = false;
		physics->SetLinearVelocity(pbody, 0.0f, 0.0f);
		if (godMode && dead) Respawn();
		LOG("God mode %s", godMode ? "ON" : "OFF");
	}

	if (godMode) {
		auto held = [&](int sc) { KeyState k = input->GetKey(sc); return k == KEY_DOWN || k == KEY_REPEAT; };
		float flySpeed = speed * 2.0f;
		float vx = ((held(SDL_SCANCODE_D) ? 1.0f : 0.0f) - (held(SDL_SCANCODE_A) ? 1.0f : 0.0f)) * flySpeed;
		float vy = ((held(SDL_SCANCODE_S) ? 1.0f : 0.0f) - (held(SDL_SCANCODE_W) ? 1.0f : 0.0f)) * flySpeed;
		if (vx != 0.0f) facing = (vx > 0.0f) ? 1.0f : -1.0f;

		b2Body_SetGravityScale(pbody->body, 0.0f);
		physics->SetLinearVelocity(pbody, vx, vy);

		int gx, gy;
		pbody->GetPosition(gx, gy);
		position.setX((float)gx);
		position.setY((float)gy);
		UpdateCamera();
		anims.SetCurrent((vx != 0.0f || vy != 0.0f) ? "move" : "idle");
		anims.Update(dt);
		return true;
	}

	// Dead: stay frozen for a moment, then respawn at the map's spawn point
	if (dead) {
		deathTimer -= dt;
		physics->SetLinearVelocity(pbody, 0.0f, 0.0f);
		b2Body_SetGravityScale(pbody->body, 0.0f);
		if (deathTimer <= 0.0f) Respawn();

		int dx, dy;
		pbody->GetPosition(dx, dy);
		position.setX((float)dx);
		position.setY((float)dy);
		UpdateCamera();
		anims.Update(dt);
		return true;
	}

	isGround = CheckGround();

	// ---- Timers ----
	if (isGround) {
		coyoteTimer = coyoteTime;
		if (!dashing) canDash = true;
	}
	else if (coyoteTimer > 0.0f) coyoteTimer -= dt;

	if (jumpBufferTimer > 0.0f) jumpBufferTimer -= dt;
	if (dashCooldownTimer > 0.0f) dashCooldownTimer -= dt;

	// ---- Input ----
	bool left = input->GetKey(SDL_SCANCODE_A) == KEY_REPEAT || input->GetKey(SDL_SCANCODE_A) == KEY_DOWN;
	bool right = input->GetKey(SDL_SCANCODE_D) == KEY_REPEAT || input->GetKey(SDL_SCANCODE_D) == KEY_DOWN;
	float dir = (right ? 1.0f : 0.0f) - (left ? 1.0f : 0.0f);
	if (dir != 0.0f) facing = dir;

	bool jumpPressed = input->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN || input->GetKey(SDL_SCANCODE_W) == KEY_DOWN;
	bool jumpHeld = jumpPressed || input->GetKey(SDL_SCANCODE_SPACE) == KEY_REPEAT || input->GetKey(SDL_SCANCODE_W) == KEY_REPEAT;
	bool dashPressed = input->GetKey(SDL_SCANCODE_LSHIFT) == KEY_DOWN || input->GetKey(SDL_SCANCODE_E) == KEY_DOWN;

	if (jumpPressed) jumpBufferTimer = jumpBufferTime;

	velocity = physics->GetLinearVelocity(pbody);

	// ---- Dash ----
	if (dashPressed && !dashing && canDash && dashCooldownTimer <= 0.0f) {
		dashing = true;
		canDash = false;
		dashTimer = dashDuration;
	}

	if (dashing) {
		dashTimer -= dt;
		velocity.x = facing * dashSpeed;
		velocity.y = 0.0f; // straight line, ignore gravity while dashing
		if (dashTimer <= 0.0f) {
			dashing = false;
			dashCooldownTimer = dashCooldown;
		}
	}
	else {
		velocity.x = dir * speed;

		// ---- Jump (buffered + coyote time) ----
		if (jumpBufferTimer > 0.0f && coyoteTimer > 0.0f) {
			velocity.y = -jumpSpeed;
			jumpBufferTimer = 0.0f;
			coyoteTimer = 0.0f;
			isGround = false;
		}
	}

	// ---- Gravity feel: floaty-free jump, variable height, capped fall ----
	float gravityScale;
	if (dashing) gravityScale = 0.0f;
	else if (velocity.y < 0.0f) gravityScale = jumpHeld ? riseGravityScale : cutGravityScale;
	else gravityScale = fallGravityScale;
	b2Body_SetGravityScale(pbody->body, gravityScale);
	if (velocity.y > maxFallSpeed) velocity.y = maxFallSpeed;

	// Animation state: air -> jump, ground/dash + moving -> move, otherwise idle
	if (dashing) anims.SetCurrent("move");
	else if (!isGround) anims.SetCurrent("jump");
	else if (dir != 0.0f) anims.SetCurrent("move");
	else anims.SetCurrent("idle");

	// Apply velocity via helper
	physics->SetLinearVelocity(pbody, velocity);

	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	// L10: TODO 8: Camera follow
	// ..

	// Fell below the map (pit): die
	if (position.getY() > Engine::GetInstance().map->GetMapSizeInPixels().getY() + 100.0f) Die();

	UpdateCamera();



	// L10: TODO 6: Advance the animation frame
	// ..
	anims.Update(dt);


	return true;
}

std::vector<std::string> Player::GetDebugLines() const
{
	char buf[96];
	std::vector<std::string> lines;

	std::snprintf(buf, sizeof(buf), "Pos   %d, %d px", (int)position.getX(), (int)position.getY());
	lines.push_back(buf);

	b2Vec2 v = Engine::GetInstance().physics->GetLinearVelocity(pbody);
	std::snprintf(buf, sizeof(buf), "Vel   %.1f, %.1f m/s", v.x, v.y);
	lines.push_back(buf);

	std::snprintf(buf, sizeof(buf), "Ground %s   Dash %s", isGround ? "yes" : "no",
		dashing ? "active" : (canDash && dashCooldownTimer <= 0.0f ? "ready" : "wait"));
	lines.push_back(buf);

	std::snprintf(buf, sizeof(buf), "Dead   %s   Facing %s", dead ? "yes" : "no", facing < 0.0f ? "left" : "right");
	lines.push_back(buf);

	return lines;
}

void Player::Die()
{
	if (dead || godMode) return;   // god mode: nothing can kill the player
	dead = true;
	deathTimer = deathDelay;
	dashing = false;
	LOG("Player died");
}

void Player::Respawn()
{
	Map* map = Engine::GetInstance().map.get();
	Vector2D p = map->hasSpawn ? map->spawnPoint : Vector2D(300.0f, 1400.0f);

	pbody->SetPosition((int)p.getX(), (int)p.getY());
	Engine::GetInstance().physics->SetLinearVelocity(pbody, 0.0f, 0.0f);
	position = p;

	dead = false;
	deathTimer = 0.0f;
	dashing = false;
	canDash = true;
	coyoteTimer = 0.0f;
	jumpBufferTimer = 0.0f;
	dashCooldownTimer = 0.0f;
}

void Player::UpdateCamera()
{
	Vector2D mapSize = Engine::GetInstance().map->GetMapSizeInPixels();
	float camW = Engine::GetInstance().render->camera.w;
	float limitLeft = camW / 4;
	float limitRight = mapSize.getX() - camW * 3.0f / 4.0f;

	if (position.getX() > limitLeft && position.getX() < limitRight)
	{
		Engine::GetInstance().render->camera.x = camW / 4.0f - position.getX();
	}

	// Vertical follow, clamped so the camera never leaves the map
	float camH = Engine::GetInstance().render->camera.h;
	float camY = camH / 2.0f - position.getY();
	if (camY > 0.0f) camY = 0.0f;
	if (camY < camH - mapSize.getY()) camY = camH - mapSize.getY();
	Engine::GetInstance().render->camera.y = camY;
}

bool Player::Draw()
{
	
	float feetY = position.getY() + texH / 2.0f;
	float groundY = 0.0f;
	if (Engine::GetInstance().map->FindGroundBelow(position.getX(), feetY, shadowMaxDistance, groundY))
	{
		float dist = std::max(0.0f, groundY - feetY);
		float t = std::min(1.0f, dist / shadowMaxDistance);   // 0 = standing on it, 1 = far away

		int rx = (int)(14.0f - 8.0f * t);      // 14 px -> 6 px
		int ry = (int)(4.0f - 2.0f * t);       // 4 px  -> 2 px
		Uint8 alpha = (Uint8)(150.0f - 110.0f * t);
		Engine::GetInstance().render->DrawFilledEllipse((int)position.getX(), (int)groundY - 1, rx, ry, 0, 0, 0, alpha);
	}
	
	// L08: TODO 5: Add physics to the player - updated player position using physics
	// L04: TODO 4: Render the player texture at its current position

	// L10: TODO 6: Draw the current animation frame instead of the whole spritesheet
	// ..
	SDL_Rect currentFrame = anims.GetCurrentFrame();
	

	SDL_FlipMode flip = (facing < 0.0f) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
	Engine::GetInstance().render->DrawTexture(texture, (int)position.getX() - texW / 2, (int)position.getY() - texH / 2, &currentFrame, 1.0f, 0.0, INT_MAX, INT_MAX, flip);

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
	case ColliderType::DAMAGE:
		Die();
		break;
	case ColliderType::UNKNOWN:
		LOG("Collision UNKNOWN");
		break;
	default:
		break;
	}
}