#pragma once

#include "Entity.h"
#include <SDL3/SDL.h>
#include <box2d/box2d.h>
#include "Animation.h"

#include <string>
#include <vector>

struct SDL_Texture;

class Player : public Entity
{
public:

	Player();

	virtual ~Player();

	bool Awake();

	bool Start();

	bool Update(float dt);

	bool Draw();

	void Die();

	void Respawn();

	bool IsDead() const { return dead; }

	bool godMode = false;
	std::vector<std::string> GetDebugLines() const;

	bool CleanUp();

	// L08: TODO 6: Define OnCollision function for the player.
	void OnCollision(PhysBody* physA, PhysBody* physB);

public:

	// Declare player parameters (fallback values)
	float speed = 5.0f;
	SDL_Texture* texture;
	int texW, texH;
	int hitW = 20, hitH = 34;
	bool spriteFacesRight = true;

	// L08: TODO 5: Add physics to the player - declare a Physics body
	PhysBody* pbody = nullptr;

	// True while the player is standing on a platform (updated every frame)
	bool isGround = false;

	// ---- Jump tuning (Box2D meters / seconds; 50 px = 1 m) ----
	// Peak height = jumpSpeed^2 / (2 * 10 * riseGravityScale)  ->  ~2.3 m (~115 px)
	float jumpSpeed = 10.0f;
	float riseGravityScale = 2.2f;   // gravity multiplier while going up with the jump key held
	float cutGravityScale = 5.0f;    // going up but key released -> short hop
	float fallGravityScale = 3.0f;   // going down: snappier landing
	float maxFallSpeed = 18.0f;
	float coyoteTime = 0.10f;        // can still jump this long after leaving a ledge
	float jumpBufferTime = 0.12f;    // a jump pressed this early before landing still counts

	float shadowMaxDistance = 320.0f;

	// ---- Dash tuning ----
	float dashSpeed = 14.0f;
	float dashDuration = 0.16f;      // ~2.2 m (~110 px)
	float dashCooldown = 0.5f;

private:
	// ---- One function per action ----
	void LoadParameters();
	void ReadInput();
	void UpdateTimers(float dt);
	void Move();
	void Jump();
	void UpdateJumpSound(float dt);
	void StopJumpSound();
	void Dash(float dt);
	void ApplyGravity();
	void UpdateAnimation();
	void ToggleGodMode();
	void GodModeFly();
	void UpdateDead(float dt);
	void SyncPositionFromBody();
	void CheckFallOutOfMap();

	bool CheckGround() const;
	void UpdateCamera();

	// Input of the current frame (written by ReadInput)
	float inputDir = 0.0f;
	bool jumpPressed = false;
	bool jumpHeld = false;
	bool dashPressed = false;

	bool dead = false;
	float deathTimer = 0.0f;
	float deathDelay = 0.5f;   // seconds frozen before respawning

	b2Vec2 velocity;

	// Movement state
	float coyoteTimer = 0.0f;
	float jumpBufferTimer = 0.0f;
	bool dashing = false;
	bool canDash = true;
	float dashTimer = 0.0f;
	float dashCooldownTimer = 0.0f;
	float facing = 1.0f;             // 1 = right, -1 = left
	// Audio fx
	int pickCoinFxId;
	int jumpFxId = 0;
	int spawnFxId = 0;
	int dashFxId = 0;

	// Jump sound tracking
	bool  jumpSoundActive = false;
	float jumpAirTime = 0.0f;      // seconds since takeoff
	float jumpSoundPos = 0.0f;     // how much of the sound has played (s)
	float jumpSoundLen = 0.0f;     // total sound length (s)
	float jumpStartY = 0.0f;       // y (px) at takeoff

	// L10: TODO 5: Player's animation clips, loaded from a TSX in Start()
	// ..
	AnimationSet anims;
};