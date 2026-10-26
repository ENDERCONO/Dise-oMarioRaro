#pragma once
#include "Module.h"
#include "Entity.h"
#include <list>
#include <cmath>           // for floor in METERS_TO_PIXELS
#include <box2d/box2d.h>   // Box2D 3.x single header

constexpr float GRAVITY_X = 0.0f;
constexpr float GRAVITY_Y = -10.0f;

constexpr float PIXELS_PER_METER = 50.0f;
constexpr float METER_PER_PIXEL = 1.0f / PIXELS_PER_METER; // derived, never hand-edit

inline int METERS_TO_PIXELS(float m) { return (int)std::floor(PIXELS_PER_METER * m); }
inline float PIXEL_TO_METERS(int p) { return METER_PER_PIXEL * (float)p; }

constexpr float DEGTORAD = 0.0174532925199432957f;
constexpr float RADTODEG = 57.295779513082320876f;

// Types of bodies
enum class bodyType {
	DYNAMIC,
	STATIC,
	KINEMATIC
};

enum class ColliderType {
	PLAYER,
	ITEM,
	PLATFORM,
	ENEMY,
	UNKNOWN
	// ..
};

// Small class to return to other modules to track position and rotation of physics bodies
class PhysBody
{
public:
	PhysBody() : listener(NULL), body(b2_nullBodyId), ctype(ColliderType::UNKNOWN) {}
	~PhysBody() {}

	void  GetPosition(int& x, int& y) const;
	void  SetPosition(int x, int y);
	float GetRotation() const;
	bool  Contains(int x, int y) const;
	int   RayCast(int x1, int y1, int x2, int y2, float& normal_x, float& normal_y) const;

public:
	b2BodyId body;              // id instead of pointer (v3.x)
	Entity* listener;
	ColliderType ctype;
};

// Module --------------------------------------
class Physics : public Module
{
public:

	// Constructors & Destructors
	Physics();
	~Physics();

	// Main module steps
	bool Awake();
	bool PreUpdate();
	bool PostUpdate();
	bool CleanUp();

	// Create basic physics objects
	PhysBody* CreateRectangle(int x, int y, int width, int height, bodyType type);
	PhysBody* CreateCircle(int x, int y, int radius, bodyType type);

	// Capsule = two semicircles (radius = width/2) joined by a segment of
	// length (height - width); degenerates to a circle when height == width.
	// Character bodies only -- fixedRotation is set unconditionally so they
	// don't tip over the way a free-rotating circle does.
	PhysBody* CreateCapsule(int x, int y, int width, int height, bodyType type);

	PhysBody* CreateRectangleSensor(int x, int y, int width, int height, bodyType type);
	PhysBody* CreateChain(int x, int y, int* points, int size, bodyType type);

	// Attaches an additional sensor shape to an existing body, at an offset
	// from its origin -- e.g. a foot sensor -- instead of creating a whole
	// separate body that would have to be kept in sync every frame.
	void AddSensorShape(PhysBody* p, int offsetX, int offsetY, int w, int h);

	// Invoked from our event processing
	void BeginContact(b2ShapeId shapeA, b2ShapeId shapeB);
	void EndContact(b2ShapeId shapeA, b2ShapeId shapeB);

	void DeletePhysBody(PhysBody* physBody);
	bool IsPendingToDelete(PhysBody* physBody);

	// --- Velocity helpers (thin wrappers over Box2D 3.x C API)
	b2Vec2 GetLinearVelocity(const PhysBody* p) const;
	float  GetXVelocity(const PhysBody* p) const;
	float  GetYVelocity(const PhysBody* p) const;

	void   SetLinearVelocity(PhysBody* p, const b2Vec2& v) const;
	void   SetLinearVelocity(PhysBody* p, float vx, float vy) const;
	void   SetXVelocity(PhysBody* p, float vx) const;
	void   SetYVelocity(PhysBody* p, float vy) const;

	// --- Impulse helper (handy for jumps/dashes)
	void   ApplyLinearImpulseToCenter(PhysBody* p, float ix, float iy, bool wake = true) const;

private:
	// helpers
	static b2BodyType ToB2Type(bodyType t);
	static void* ToUserData(PhysBody* p) { return (void*)p; }
	static PhysBody* FromUserData(void* ud) { return (PhysBody*)ud; }
	static PhysBody* BodyToPhys(b2BodyId b) { return FromUserData(b2Body_GetUserData(b)); }

	// --- Debug draw callbacks (Box2D 3.1 signatures)
	static void DrawSegmentCb(b2Vec2 p1, b2Vec2 p2, b2HexColor color, void* ctx);
	static void DrawPolygonCb(const b2Vec2* verts, int count, b2HexColor color, void* ctx);
	static void DrawSolidPolygonCb(b2Transform xf, const b2Vec2* verts, int count, float radius, b2HexColor color, void* ctx);
	static void DrawCircleCb(b2Vec2 center, float radius, b2HexColor color, void* ctx);
	static void DrawSolidCircleCb(b2Transform xf, float radius, b2HexColor color, void* ctx);
	static void DrawSolidCapsuleCb(b2Vec2 a, b2Vec2 b, float r, b2HexColor c, void* ctx);

	// --- Defensive no-op stubs (so Box2D never calls a null fn ptr)
	static void DrawPointStub(b2Vec2 p, float size, b2HexColor c, void* ctx);
	static void DrawStringStub(b2Vec2 p, const char* s, b2HexColor c, void* ctx);
	static void DrawTransformStub(b2Transform xf, void* ctx);

private:

	// Debug mode
	bool debug;

	// Box2D World (id instead of pointer)
	b2WorldId world;

	// Fixed-timestep accumulator (seconds). dt has been in seconds since L02.
	float accumulator = 0.0f;

	// List of physics bodies
	std::list<PhysBody*> bodiesToDelete;
};
