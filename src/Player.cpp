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
#include <cmath>
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

	
	//Physics hitbox and sprite direction from config.xml
	hitW = parameters.child("hitbox").attribute("width").as_int(texW / 2);
	hitH = parameters.child("hitbox").attribute("height").as_int(texH);
	spriteFacesRight = std::string(parameters.child("sprite").attribute("facing").as_string("right")) != "left";

	//Tile ids of the first frame of each clip in PlayerBlue.tsx (8 columns: one clip per row)
	anims.LoadFromTSX(parameters.child("animations").attribute("tsx").as_string(),
		{ {0, "idle"}, {8, "move"}, {16, "jump"}, {24, "die"} });
	anims.SetCurrent("idle");

	//Movement / jump / dash / death tuning from config.xml
	LoadParameters();

	// Audio effect path from config.xml
	pickCoinFxId = Engine::GetInstance().audio->LoadFx(parameters.child("audiofx").attribute("path").as_string());

	// Jump sound
	jumpFxId = Engine::GetInstance().audio->LoadFx(parameters.child("jumpfx").attribute("path").as_string());
	jumpSoundLen = Engine::GetInstance().audio->GetFxDuration(jumpFxId);

	// Respawn sound
	spawnFxId = Engine::GetInstance().audio->LoadFx(parameters.child("spawnfx").attribute("path").as_string());

	// L08: TODO 5: Add physics to the player - create the physics body: a CAPSULE (CreateCapsule) with fixed rotation, so the player does not roll
	// Capsule + fixed rotation: no rolling, and no bumping over the seams between tile rectangles
	pbody = Engine::GetInstance().physics->CreateCapsule((int)position.getX() + hitW / 2, (int)position.getY() + hitH / 2, hitW, hitH, bodyType::DYNAMIC);

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

// Update() only decides WHAT happens this frame; every action lives in its
// own function below (Move, Jump, Dash, ApplyGravity, ...).
bool Player::Update(float dt)
{
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_F10) == KEY_DOWN)
		ToggleGodMode();

	if (godMode) {
		GodModeFly();
	}
	else if (dead) {
		UpdateDead(dt);
	}
	else {
		Physics* physics = Engine::GetInstance().physics.get();

		isGround = CheckGround();
		UpdateTimers(dt);
		ReadInput();
		UpdateJumpSound(dt);

		velocity = physics->GetLinearVelocity(pbody);
		Dash(dt);
		if (!dashing) {
			Move();
			Jump();
		}
		ApplyGravity();
		physics->SetLinearVelocity(pbody, velocity);

		UpdateAnimation();
	}

	SyncPositionFromBody();
	CheckFallOutOfMap();
	UpdateCamera();

	// L10: TODO 6: Advance the animation frame (logic, so it lives in Update)
	anims.Update(dt);

	return true;
}

void Player::LoadParameters()
{
	speed = parameters.child("movement").attribute("speed").as_float(speed);

	pugi::xml_node jump = parameters.child("jump");
	jumpSpeed = jump.attribute("force").as_float(jumpSpeed);
	riseGravityScale = jump.attribute("riseGravity").as_float(riseGravityScale);
	cutGravityScale = jump.attribute("cutGravity").as_float(cutGravityScale);
	fallGravityScale = jump.attribute("fallGravity").as_float(fallGravityScale);
	maxFallSpeed = jump.attribute("maxFallSpeed").as_float(maxFallSpeed);
	coyoteTime = jump.attribute("coyoteTime").as_float(coyoteTime);
	jumpBufferTime = jump.attribute("bufferTime").as_float(jumpBufferTime);

	pugi::xml_node dash = parameters.child("dash");
	dashSpeed = dash.attribute("speed").as_float(dashSpeed);
	dashDuration = dash.attribute("duration").as_float(dashDuration);
	dashCooldown = dash.attribute("cooldown").as_float(dashCooldown);

	deathDelay = parameters.child("death").attribute("delay").as_float(deathDelay);
}

void Player::ReadInput()
{
	Input* input = Engine::GetInstance().input.get();
	auto held = [&](int sc) { KeyState k = input->GetKey(sc); return k == KEY_DOWN || k == KEY_REPEAT; };

	inputDir = (held(SDL_SCANCODE_D) ? 1.0f : 0.0f) - (held(SDL_SCANCODE_A) ? 1.0f : 0.0f);
	if (inputDir != 0.0f) facing = inputDir;

	jumpPressed = input->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN || input->GetKey(SDL_SCANCODE_W) == KEY_DOWN;
	jumpHeld = held(SDL_SCANCODE_SPACE) || held(SDL_SCANCODE_W);
	dashPressed = input->GetKey(SDL_SCANCODE_LSHIFT) == KEY_DOWN || input->GetKey(SDL_SCANCODE_E) == KEY_DOWN;

	if (jumpPressed) jumpBufferTimer = jumpBufferTime;
}

void Player::UpdateTimers(float dt)
{
	if (isGround) {
		coyoteTimer = coyoteTime;
		if (!dashing) canDash = true;
	}
	else if (coyoteTimer > 0.0f) coyoteTimer -= dt;

	if (jumpBufferTimer > 0.0f) jumpBufferTimer -= dt;
	if (dashCooldownTimer > 0.0f) dashCooldownTimer -= dt;
}

void Player::Move()
{
	// m/s: Box2D integrates it with a fixed time step, so it is framerate independent
	velocity.x = inputDir * speed;
}

void Player::Jump()
{
	// Buffered (pressed slightly before landing) + coyote time (slightly after leaving a ledge)
	if (jumpBufferTimer > 0.0f && coyoteTimer > 0.0f) {
		velocity.y = -jumpSpeed;
		jumpBufferTimer = 0.0f;
		coyoteTimer = 0.0f;
		isGround = false;

		// Jump sound starts on takeoff, UpdateJumpSound() keeps it in sync
		if (Engine::GetInstance().audio->PlayFxTracked(jumpFxId)) {
			jumpSoundActive = true;
			jumpAirTime = 0.0f;
			jumpSoundPos = 0.0f;
			jumpStartY = position.getY();
		}
	}
}

void Player::StopJumpSound()
{
	if (!jumpSoundActive) return;
	Engine::GetInstance().audio->StopFxTracked(0.05f);   // 50 ms fade-out, no click
	jumpSoundActive = false;
}

void Player::UpdateJumpSound(float dt)
{
	if (!jumpSoundActive) return;
	Audio* audio = Engine::GetInstance().audio.get();

	jumpAirTime += dt;

	// 1) Landing -> stop the sound. The 0.08 s guard is because the ground
	//    contact can still be alive for a frame or two right after takeoff.
	if (isGround && jumpAirTime > 0.08f) {
		StopJumpSound();
		return;
	}

	// 2) Estimate how long the jump still has to last (same formulas as ApplyGravity)
	float g = -GRAVITY_Y;                                                   // 8 m/s^2
	float vy = Engine::GetInstance().physics->GetLinearVelocity(pbody).y;    // < 0 = going up
	float h = (jumpStartY - position.getY()) / PIXELS_PER_METER;            // m above takeoff

	float tUp = 0.0f, apex = h;
	if (vy < 0.0f) {
		float rise = jumpHeld ? riseGravityScale : cutGravityScale;   // key held -> long, released -> short
		tUp = -vy / (g * rise);
		apex = h + (vy * vy) / (2.0f * g * rise);
	}
	if (apex < 0.1f) apex = 0.1f;                                    // avoid dividing by ~0
	float tDown = std::sqrt(2.0f * apex / (g * fallGravityScale));
	float airLeft = tUp + tDown;

	// 3) Speed so the remaining sound ends right when we land
	float soundLeft = jumpSoundLen - jumpSoundPos;
	float ratio = 1.0f;
	if (soundLeft > 0.0f && airLeft > 0.0f)
		ratio = std::clamp(soundLeft / airLeft, 0.8f, 1.25f);         // clamp: avoid weird pitch

	audio->SetFxTrackedSpeed(ratio);
	jumpSoundPos += dt * ratio;                                      // sound time advances at 'ratio'
}

void Player::Dash(float dt)
{
	if (dashPressed && !dashing && canDash && dashCooldownTimer <= 0.0f) {
		dashing = true;
		canDash = false;
		dashTimer = dashDuration;
	}
	if (!dashing) return;

	dashTimer -= dt;
	velocity.x = facing * dashSpeed;
	velocity.y = 0.0f; // straight line, ignore gravity while dashing
	if (dashTimer <= 0.0f) {
		dashing = false;
		dashCooldownTimer = dashCooldown;
	}
}

void Player::ApplyGravity()
{
	// Floaty-free jump, variable height (release early = short hop), capped fall
	float gravityScale;
	if (dashing) gravityScale = 0.0f;
	else if (velocity.y < 0.0f) gravityScale = jumpHeld ? riseGravityScale : cutGravityScale;
	else gravityScale = fallGravityScale;

	b2Body_SetGravityScale(pbody->body, gravityScale);
	if (velocity.y > maxFallSpeed) velocity.y = maxFallSpeed;
}

void Player::UpdateAnimation()
{
	if (dashing) anims.SetCurrent("move");
	else if (!isGround && coyoteTimer <= 0.0f) anims.SetCurrent("jump"); // ignore 1-frame ground losses on tile seams
	else if (inputDir != 0.0f) anims.SetCurrent("move");
	else anims.SetCurrent("idle");
}

void Player::ToggleGodMode()
{
	godMode = !godMode;
	StopJumpSound();
	dashing = false;
	Engine::GetInstance().physics->SetLinearVelocity(pbody, 0.0f, 0.0f);
	if (godMode && dead) Respawn();
	LOG("God mode %s", godMode ? "ON" : "OFF");
}

void Player::GodModeFly()
{
	Input* input = Engine::GetInstance().input.get();
	auto held = [&](int sc) { KeyState k = input->GetKey(sc); return k == KEY_DOWN || k == KEY_REPEAT; };

	float flySpeed = speed * 2.0f;
	float vx = ((held(SDL_SCANCODE_D) ? 1.0f : 0.0f) - (held(SDL_SCANCODE_A) ? 1.0f : 0.0f)) * flySpeed;
	float vy = ((held(SDL_SCANCODE_S) ? 1.0f : 0.0f) - (held(SDL_SCANCODE_W) ? 1.0f : 0.0f)) * flySpeed;
	if (vx != 0.0f) facing = (vx > 0.0f) ? 1.0f : -1.0f;

	b2Body_SetGravityScale(pbody->body, 0.0f);
	Engine::GetInstance().physics->SetLinearVelocity(pbody, vx, vy);
	anims.SetCurrent((vx != 0.0f || vy != 0.0f) ? "move" : "idle");
}

void Player::UpdateDead(float dt)
{
	// Frozen for deathDelay seconds (the die animation plays), then respawn
	Engine::GetInstance().physics->SetLinearVelocity(pbody, 0.0f, 0.0f);
	b2Body_SetGravityScale(pbody->body, 0.0f);

	deathTimer -= dt;
	if (deathTimer <= 0.0f) {
		Respawn();
		// Only if the respawn really happened (it can fail if the map has no Spawn layer;
		// without this check the sound would repeat every frame)
		if (!dead) Engine::GetInstance().audio->PlayFx(spawnFxId);
	}
}

void Player::SyncPositionFromBody()
{
	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);
}

void Player::CheckFallOutOfMap()
{
	// Backup only: pits are real PIT colliders built from the map
	if (position.getY() > Engine::GetInstance().map->GetMapSizeInPixels().getY() + 100.0f) Die();
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
	StopJumpSound();
	deathTimer = deathDelay;
	anims.SetCurrent("die");
	dashing = false;
	LOG("Player died");
}

void Player::Respawn()
{
	Map* map = Engine::GetInstance().map.get();
	if (!map->hasSpawn) {
		LOG("ERROR: the map has no 'Spawn' object layer, cannot respawn");
		return;
	}
	Vector2D p = map->spawnPoint;

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
	anims.SetCurrent("idle");
}

void Player::UpdateCamera()
{
	Vector2D mapSize = Engine::GetInstance().map->GetMapSizeInPixels();
	float camW = Engine::GetInstance().render->camera.w;
	
	float camX = camW / 4.0f - position.getX();
	if (camX > 0.0f) camX = 0.0f;
	if (camX < camW - mapSize.getX()) camX = camW - mapSize.getX();
	Engine::GetInstance().render->camera.x = camX;

	// Vertical follow, clamped so the camera never leaves the map
	float camH = Engine::GetInstance().render->camera.h;
	float camY = camH / 2.0f - position.getY();
	if (camY > 0.0f) camY = 0.0f;
	if (camY < camH - mapSize.getY()) camY = camH - mapSize.getY();
	Engine::GetInstance().render->camera.y = camY;
}

bool Player::Draw()
{
	
	float feetY = position.getY() + hitH / 2.0f;   // bottom of the capsule
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
	

	bool lookingLeft = (facing < 0.0f);
	SDL_FlipMode flip = (lookingLeft == spriteFacesRight) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
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
	case ColliderType::PIT:
		Die();
		break;
	case ColliderType::UNKNOWN:
		LOG("Collision UNKNOWN");
		break;
	default:
		break;
	}
}