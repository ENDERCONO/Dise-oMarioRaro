#include "Engine.h"
#include "Render.h"
#include "Textures.h"
#include "Map.h"
#include "Log.h"
#include "Physics.h"

#include <iterator>

Map::Map() : Module(), mapLoaded(false)
{
	name = "map";
}

// Destructor
Map::~Map()
{}

// Called before render is available
bool Map::Awake()
{
	LOG("Loading Map Parser");

	return true;
}

bool Map::Start()
{
	return true;
}

bool Map::PostUpdate()
{
	bool ret = true;

	if (mapLoaded) {

		// L07: TODO 5: Draw every tile in every layer
		for (const auto& layer : mapData.layers) {

			// L09: TODO 7: Implement a method to get the value of a custom property and check if the layer is visible before drawing
			const Properties::Property* property = layer.properties.GetProperty("Draw");
			if (property != nullptr && property->value == true) { //We only enter if the Draw property is set to true

				for (int i = 0; i < mapData.width; i++) {
					for (int j = 0; j < mapData.height; j++) {

						// L07: TODO 9: Get the gid, look up its rect in the tileset, convert
						// tile coordinates to world coordinates, then draw
						int gid = layer.Get(i, j);

						// L09: TODO 3: Obtain the tile set using GetTilesetFromTileId
						if (gid != 0) {
							const TileSet* tileSet = GetTilesetFromTileId(gid);

							SDL_Rect tileRect = tileSet->GetRect(gid);
							Vector2D mapCoord = MapToWorld(i, j);
							Engine::GetInstance().render->DrawTexture(tileSet->texture, (int)mapCoord.getX(), (int)mapCoord.getY(), &tileRect);
						}

					}
				}
			}
		}
	}

	return ret;
}

// Called before quitting
bool Map::CleanUp()
{
	LOG("Unloading map");

	// L06: TODO 2: Unload every tileset's texture
	for (const auto& tileset : mapData.tilesets) {
		Engine::GetInstance().textures->UnLoad(tileset.texture);
	}
	mapData.tilesets.clear();

	// L07: TODO 2: Clear the layers
	mapData.layers.clear();

	return true;
}

// Load new map
bool Map::Load(std::string path, std::string fileName)
{
	bool ret = false;

	// Assigns the name of the map file and the path
	mapFileName = fileName;
	mapPath = path;
	std::string mapPathName = mapPath + mapFileName;

	pugi::xml_document mapFileXML;
	pugi::xml_parse_result result = mapFileXML.load_file(mapPathName.c_str());

	if (!result)
	{
		LOG("Could not load map xml file %s. pugi error: %s", mapPathName.c_str(), result.description());
		ret = false;
	}
	else {

		// L06: TODO 3: Read the <map> node's attributes into mapData
		pugi::xml_node mapNode = mapFileXML.child("map");
		mapData.width = mapNode.attribute("width").as_int();
		mapData.height = mapNode.attribute("height").as_int();
		mapData.tileWidth = mapNode.attribute("tilewidth").as_int();
		mapData.tileHeight = mapNode.attribute("tileheight").as_int();

		// L06: TODO 4: Load every <tileset>, including its image; a missing
		// image fails the whole load instead of silently drawing nothing later
		ret = true;

		for (pugi::xml_node tilesetNode = mapNode.child("tileset"); tilesetNode; tilesetNode = tilesetNode.next_sibling("tileset"))
		{
			TileSet tileSet;
			tileSet.firstGid = tilesetNode.attribute("firstgid").as_int();
			tileSet.name = tilesetNode.attribute("name").as_string();
			tileSet.tileWidth = tilesetNode.attribute("tilewidth").as_int();
			tileSet.tileHeight = tilesetNode.attribute("tileheight").as_int();
			tileSet.spacing = tilesetNode.attribute("spacing").as_int();
			tileSet.margin = tilesetNode.attribute("margin").as_int();
			tileSet.tileCount = tilesetNode.attribute("tilecount").as_int();
			tileSet.columns = tilesetNode.attribute("columns").as_int();

			std::string imgName = tilesetNode.child("image").attribute("source").as_string();
			tileSet.texture = Engine::GetInstance().textures->Load((mapPath + imgName).c_str());

			if (tileSet.texture == NULL) {
				LOG("Could not load tileset image %s", (mapPath + imgName).c_str());
				ret = false;
			}

			mapData.tilesets.push_back(tileSet);
		}

		// L07: TODO 3: Load every <layer>
		for (pugi::xml_node layerNode = mapNode.child("layer"); layerNode; layerNode = layerNode.next_sibling("layer"))
		{
			// L07: TODO 4: Read one layer's attributes and its tile data
			MapLayer layer;
			layer.id = layerNode.attribute("id").as_int();
			layer.name = layerNode.attribute("name").as_string();
			layer.width = layerNode.attribute("width").as_int();
			layer.height = layerNode.attribute("height").as_int();

			// L09: TODO 6: Call Load Layer Properties
			LoadLayerProperties(layerNode, layer.properties);

			pugi::xml_node dataNode = layerNode.child("data");
			for (pugi::xml_node tileNode = dataNode.child("tile"); tileNode; tileNode = tileNode.next_sibling("tile"))
			{
				int gid = tileNode.attribute("gid").as_int();
				layer.tiles.push_back(gid);
			}
			mapData.layers.push_back(layer);
		}

		// L09: TODO 8: Create colliders from map metadata
		// Replace L08 TODO 3 with a loop that builds one
		// rectangle collider per collider tile, for any layer whose "Collision"
		// property is true.
		for (const auto& layer : mapData.layers)
		{
			const Properties::Property* collisionLayer = layer.properties.GetProperty("Collision");
			if (collisionLayer != nullptr && collisionLayer->value == true) {
				for (int i = 0; i < mapData.width; i++) {
					for (int j = 0; j < mapData.height; j++) {

						int gid = layer.Get(i, j);
						if (gid != 0 && gid == colliderGid) {
							Vector2D mapCoord = MapToWorld(i, j);
							PhysBody* body = Engine::GetInstance().physics->CreateRectangle((int)mapCoord.getX() + mapData.tileWidth / 2,
																							(int)mapCoord.getY() + mapData.tileHeight / 2,
																							mapData.tileWidth, 
																							mapData.tileHeight, 
																							bodyType::STATIC);
							body->ctype = ColliderType::PLATFORM;
						}
					}
				}
			}
		}
		


		// L06: TODO 5: LOG all the data loaded, iterating all tilesets
		if (ret == true)
		{
			LOG("Successfully parsed map XML file: %s", fileName.c_str());
			LOG("width : %d height : %d", mapData.width, mapData.height);
			LOG("tile_width : %d tile_height : %d", mapData.tileWidth, mapData.tileHeight);
			LOG("Tilesets----");

			for (const auto& tileset : mapData.tilesets) {
				LOG("name : %s firstgid : %d", tileset.name.c_str(), tileset.firstGid);
				LOG("tile width : %d tile height : %d", tileset.tileWidth, tileset.tileHeight);
				LOG("spacing : %d margin : %d", tileset.spacing, tileset.margin);
			}

			// L07: TODO 3: LOG every layer too
			LOG("Layers----");
			for (const auto& layer : mapData.layers) {
				LOG("id : %d name : %s", layer.id, layer.name.c_str());
				LOG("Layer width : %d Layer height : %d", layer.width, layer.height);
			}
		}
		else {
			LOG("Error while parsing map file: %s", mapPathName.c_str());
		}

		if (mapFileXML) mapFileXML.reset();
	}

	mapLoaded = ret;
	return ret;
}

// L07: TODO 8: Translate tile coordinates (i, j) into world (pixel) coordinates
Vector2D Map::MapToWorld(int x, int y) const
{
	Vector2D ret;

	ret.setX((float)(x * mapData.tileWidth));
	ret.setY((float)(y * mapData.tileHeight));

	return ret;
}

// L09: TODO 2: Implement function to the Tileset based on a tile id
const TileSet* Map::GetTilesetFromTileId(int gid) const
{
	for (const auto& tileset : mapData.tilesets)
	{
		if (gid >= tileset.firstGid && gid < tileset.firstGid + tileset.tileCount)
			return &tileset;  
	}

	LOG("Map: gid %d does not belong to any tileset", gid);
	return nullptr;
}

// L09: TODO 6: Load a group of properties from a node and fill a list with it
bool Map::LoadLayerProperties(pugi::xml_node& layerNode, Properties& properties)
{
	bool ret = true;
	for (pugi::xml_node propertyNode = layerNode.child("properties").child("property"); propertyNode; propertyNode = propertyNode.next_sibling("property"))
	{
		Properties::Property property;
		property.name = propertyNode.attribute("name").as_string();
		property.value = propertyNode.attribute("value").as_bool(); // we assume the property is a boolean for simplicity; adjust as needed
		properties.propertyList.push_back(property);
	}
	return ret;
}

// L10: TODO 8: Return the map's total size in pixels (width/height in
// tiles times tile size) -- used to clamp the camera to the map's edges
Vector2D Map::GetMapSizeInPixels() {
	Vector2D mapSize;
	mapSize.setX((float)(mapData.width * mapData.tileWidth));
	mapSize.setY((float)(mapData.height * mapData.tileHeight));
	return mapSize;
}

