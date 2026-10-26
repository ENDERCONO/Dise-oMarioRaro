#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <SDL3/SDL_rect.h>

struct AnimFrame
{
	SDL_Rect rect{};
	float duration = 0.1f; // seconds
};

// A single named clip: an ordered list of source rects and how long each
// one is shown for.
class Animation
{
public:
	void AddFrame(const SDL_Rect& r, float durationSeconds);
	void SetLoop(bool loop);
	void Reset();
	bool HasFinishedOnce() const;

	// dt in seconds, matching every other Update() in the engine.
	void Update(float dt);

	const SDL_Rect& GetCurrentFrame() const;
	int GetFrameCount() const;

private:
	std::vector<AnimFrame> frames;
	int currentIndex = 0;
	float timeInFrame = 0.0f;
	bool loop = true;
	bool finishedOnce = false;
};

// All of one entity's clips, loaded in one call from a Tiled TSX. Tiled
// marks an animated clip by giving its first frame's tile an <animation>
// block; `aliases` names those clips (e.g. {0, "idle"}, {11, "move"}) so
// gameplay code can ask for them by name instead of by tile id.
class AnimationSet
{
public:
	bool LoadFromTSX(const char* tsxPath, const std::unordered_map<int, std::string>& aliases);

	void SetCurrent(const std::string& name);
	void Update(float dt);
	const SDL_Rect& GetCurrentFrame() const;
	const std::string& GetCurrentName() const;
	bool Has(const std::string& name) const;

private:
	int tileWidth = 0;
	int tileHeight = 0;
	int columns = 0;
	std::unordered_map<std::string, Animation> clips;
	std::string currentName;

	static SDL_Rect TileIdToRect(int tileId, int columns, int tileWidth, int tileHeight);
};
