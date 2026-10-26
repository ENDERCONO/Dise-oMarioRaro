#include "Animation.h"
#include "Log.h"
#include "pugixml.hpp"

// --- Animation ---------------------------------------------------------

void Animation::AddFrame(const SDL_Rect& r, float durationSeconds)
{
	frames.push_back({ r, durationSeconds });
}

void Animation::SetLoop(bool v)
{
	loop = v;
}

void Animation::Reset()
{
	currentIndex = 0;
	timeInFrame = 0.0f;
	finishedOnce = false;
}

bool Animation::HasFinishedOnce() const
{
	return finishedOnce && !loop;
}

void Animation::Update(float dt)
{
	if (frames.empty()) return;

	timeInFrame += dt;

	while (timeInFrame >= frames[currentIndex].duration)
	{
		timeInFrame -= frames[currentIndex].duration;

		if (currentIndex + 1 < (int)frames.size())
		{
			++currentIndex;
		}
		else if (loop)
		{
			currentIndex = 0;
		}
		else
		{
			finishedOnce = true;
			currentIndex = (int)frames.size() - 1;
			break;
		}
	}
}

const SDL_Rect& Animation::GetCurrentFrame() const
{
	static const SDL_Rect empty{ 0, 0, 0, 0 };
	return frames.empty() ? empty : frames[currentIndex].rect;
}

int Animation::GetFrameCount() const
{
	return (int)frames.size();
}

// --- AnimationSet --------------------------------------------------------

SDL_Rect AnimationSet::TileIdToRect(int tileId, int columns, int tileWidth, int tileHeight)
{
	SDL_Rect r{};
	r.x = (tileId % columns) * tileWidth;
	r.y = (tileId / columns) * tileHeight;
	r.w = tileWidth;
	r.h = tileHeight;
	return r;
}

bool AnimationSet::LoadFromTSX(const char* tsxPath, const std::unordered_map<int, std::string>& aliases)
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(tsxPath);
	if (!result)
	{
		LOG("Could not load animation TSX %s. pugi error: %s", tsxPath, result.description());
		return false;
	}

	pugi::xml_node tilesetNode = doc.child("tileset");
	if (!tilesetNode)
	{
		LOG("TSX %s has no <tileset> node", tsxPath);
		return false;
	}

	tileWidth = tilesetNode.attribute("tilewidth").as_int();
	tileHeight = tilesetNode.attribute("tileheight").as_int();
	columns = tilesetNode.attribute("columns").as_int();

	for (pugi::xml_node tileNode : tilesetNode.children("tile"))
	{
		pugi::xml_node animNode = tileNode.child("animation");
		if (!animNode) continue;

		int baseId = tileNode.attribute("id").as_int(-1);
		auto alias = aliases.find(baseId);
		std::string name = (alias != aliases.end()) ? alias->second : ("tile_" + std::to_string(baseId));

		Animation clip;
		clip.SetLoop(true);

		for (pugi::xml_node frameNode : animNode.children("frame"))
		{
			int frameTileId = frameNode.attribute("tileid").as_int();
			float durationSeconds = frameNode.attribute("duration").as_int(100) / 1000.0f;
			clip.AddFrame(TileIdToRect(frameTileId, columns, tileWidth, tileHeight), durationSeconds);
		}

		clip.Reset();
		clips.emplace(name, std::move(clip));
	}

	if (!clips.empty() && currentName.empty()) currentName = clips.begin()->first;

	return !clips.empty();
}

void AnimationSet::SetCurrent(const std::string& name)
{
	if (currentName == name || !Has(name)) return;
	currentName = name;
	clips[currentName].Reset();
}

void AnimationSet::Update(float dt)
{
	if (Has(currentName)) clips[currentName].Update(dt);
}

const SDL_Rect& AnimationSet::GetCurrentFrame() const
{
	static const SDL_Rect empty{ 0, 0, 0, 0 };
	return Has(currentName) ? clips.at(currentName).GetCurrentFrame() : empty;
}

const std::string& AnimationSet::GetCurrentName() const
{
	return currentName;
}

bool AnimationSet::Has(const std::string& name) const
{
	return clips.find(name) != clips.end();
}
