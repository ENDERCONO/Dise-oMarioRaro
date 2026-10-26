#pragma once

#include "Module.h"
#include "Vector2D.h"
#include <SDL3/SDL_rect.h>
#include <string>
#include <vector>

struct SDL_Texture;

// L09: TODO 5: Define properties struct
struct Properties
{
	struct Property
	{
		std::string name;
		bool value;
	};

	std::vector<Property> propertyList;

	// L09: TODO 7: Implement a method to get the value of a custom property
	const Property* GetProperty(const std::string& name) const
	{
		for (const auto& property : propertyList) {
			if (property.name == name) return &property;
		}
		return nullptr;
	}
};

// L07: TODO 1: Create a struct to hold information for a single map layer
struct MapLayer
{
	int id;
	std::string name;
	int width;
	int height;
	std::vector<int> tiles;

	// L09: TODO 5: Define properties 
	Properties properties;

	// L07: TODO 6: Return the gid at tile (i, j)
	unsigned int Get(int i, int j) const
	{
		return tiles[(j * width) + i];
	}
};

// L06: TODO 2: Create a struct to hold information for a TileSet
// Ignore Terrain Types and Tile Types for now, but we want the image!
struct TileSet
{
	int firstGid;
	std::string name;
	int tileWidth;
	int tileHeight;
	int spacing;
	int margin;
	int tileCount;
	int columns;
	SDL_Texture* texture;

	// L07: TODO 7: Return the source rect for this gid within the tileset image
	SDL_Rect GetRect(unsigned int gid) const
	{
		SDL_Rect rect = { 0 };

		int relativeIndex = gid - firstGid;
		rect.w = tileWidth;
		rect.h = tileHeight;
		rect.x = margin + (tileWidth + spacing) * (relativeIndex % columns);
		rect.y = margin + (tileHeight + spacing) * (relativeIndex / columns);

		return rect;
	}
};

// L06: TODO 1: Create a struct needed to hold the information of the <map> node
struct MapData
{
	int width;
	int height;
	int tileWidth;
	int tileHeight;
	std::vector<TileSet> tilesets;

	// L07: TODO 2: Add a vector of MapLayer to MapData
	std::vector<MapLayer> layers;
};

class Map : public Module
{
public:

	Map();

	// Destructor
	virtual ~Map();

	// Called before render is available
	bool Awake();

	// Called before the first frame
	bool Start();

	// Called before rendering, after all Updates
	bool PostUpdate();

	// Called before quitting
	bool CleanUp();

	// Load new map
	bool Load(std::string path, std::string mapFileName);

	// L07: TODO 8: Translate tile coordinates (i, j) into world (pixel) coordinates
	Vector2D MapToWorld(int i, int j) const;

	// L09: TODO 2: Implement function to the Tileset based on a tile id
	const TileSet* GetTilesetFromTileId(int gid) const;

	// L09: TODO 6: Load a group of properties 
	bool LoadLayerProperties(pugi::xml_node& layerNode, Properties& properties);

	// L10: TODO 8: Return the map's total size in pixels (width/height in
	// tiles times tile size) -- used to clamp the camera to the map's edges
	Vector2D GetMapSizeInPixels();

public:
	std::string mapFileName;
	std::string mapPath;

private:
	bool mapLoaded;

	// L06: TODO 1: Declare a MapData member to hold the parsed map
	MapData mapData;

	// L09: TODO 8: Create colliders from map metadata
	int colliderGid = 49; // The gid of the tile that represents a collider in the tileset
};
