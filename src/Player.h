#pragma once

#include "Entity.h"
#include <SDL3/SDL.h>
#include <box2d/box2d.h>
#include "Animation.h"
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

	bool CleanUp();

	// L08: TODO 6: Define OnCollision function for the player.
	void OnCollision(PhysBody* physA, PhysBody* physB);

public:

	// Declare player parameters
	float speed = 5.0f;
	SDL_Texture* texture;
	int texW, texH;

	// L08: TODO 5: Add physics to the player - declare a Physics body
	PhysBody* pbody = nullptr;

	// True while the player is standing on a platform (updated every frame)
	bool isGround = false;
	float jumpSpeed = 8.0f;

private:
	bool CheckGround() const;
	b2Vec2 velocity;
	// Audio fx
	int pickCoinFxId;

	// L10: TODO 5: Player's animation clips, loaded from a TSX in Start()
	// ..
	AnimationSet anims;
};