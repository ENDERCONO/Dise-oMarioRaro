#include "Engine.h"
#include "Input.h"
#include "Audio.h"
#include "Render.h"
#include "Scene.h"
#include "Log.h"
#include "EntityManager.h"
#include "Player.h"
#include "Entity.h"
#include "Item.h"
#include "Map.h"

#include "Physics.h"
#include <algorithm>
#include <string>
#include <vector>

namespace {
	// A dark translucent box with lines of text on it (screen space)
	void DrawTextBlock(int x, int y, const std::vector<std::string>& lines, Uint8 r, Uint8 g, Uint8 b)
	{
		Render* render = Engine::GetInstance().render.get();
		const int lineH = 18, pad = 6;
		int maxChars = 0;
		for (const auto& l : lines) maxChars = std::max(maxChars, (int)l.size());

		SDL_Rect box = { x - pad, y - pad, maxChars * 16 + pad * 2, (int)lines.size() * lineH + pad * 2 };
		render->DrawRectangle(box, 0, 0, 0, 170, true, false);

		for (size_t i = 0; i < lines.size(); ++i)
			render->DrawDebugText(lines[i].c_str(), x, y + (int)i * lineH, r, g, b);
	}
}

Scene::Scene() : Module()
{
	name = "scene";
}

// Destructor
Scene::~Scene()
{
}

// Called before render is available
bool Scene::Awake()
{
	LOG("Loading Scene");
	bool ret = true;

	// L04: TODO 3a: Instantiate the player through the entity manager
	player = std::dynamic_pointer_cast<Player>(Engine::GetInstance().entityManager->CreateEntity(EntityType::PLAYER));

	// L10: TODO 4: Hand the player its <player> config.xml section
	// ..
	player->SetParameters(Engine::GetInstance().entityManager->configParameters.child("player"));

	// L08: TODO 4: Create an item through the entity manager
	std::shared_ptr<Item> item = std::dynamic_pointer_cast<Item>(Engine::GetInstance().entityManager->CreateEntity(EntityType::ITEM));
	item->position = Vector2D(150, 200);
	return ret;
}

// Called before the first frame
bool Scene::Start()
{
	//Engine::GetInstance().audio->PlayMusic("Assets/Audio/Music/level-iv-339695.wav");

	// L06: TODO 7: Load the map through the map module
	Engine::GetInstance().map->Load("Assets/Maps/", "Mapa.tmx");

	if (player) player->Respawn();

	return true;
}

// Called each loop iteration
bool Scene::PreUpdate()
{
	return true;
}

// Called each loop iteration
bool Scene::Update(float dt)
{
	// L03: TODO 3: Make the camera movement independent of framerate.
	//   - Declare a camSpeed of 200.0f (pixels per second).
	//   - Multiply camSpeed by dt before adding/subtracting it below.
	//   - camera.x / camera.y are float (SDL_FRect), so just assign the result.
	float camSpeed = 200.0f;





	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_UP) == KEY_REPEAT)
		Engine::GetInstance().render->camera.y -= camSpeed * dt;

	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_DOWN) == KEY_REPEAT)
		Engine::GetInstance().render->camera.y += camSpeed * dt;

	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_LEFT) == KEY_REPEAT)
		Engine::GetInstance().render->camera.x -= camSpeed * dt;

	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_RIGHT) == KEY_REPEAT)
		Engine::GetInstance().render->camera.x += camSpeed * dt;

	return true;
}

void Scene::DrawDebugOverlay()
{
	Engine& engine = Engine::GetInstance();

	if (engine.input->GetKey(SDL_SCANCODE_H) == KEY_DOWN) showHelp = !showHelp;

	std::vector<std::string> status;
	if (player && player->godMode) status.push_back("GOD MODE (F10)");
	if (engine.IsCapped30()) status.push_back("30 FPS CAP (F11)");

	int y = 12;
	if (!status.empty()) {
		DrawTextBlock(12, y, status, 255, 220, 0);
		y += (int)status.size() * 18 + 20;
	}

	if (engine.physics->IsDebug() && player) {
		std::vector<std::string> info = player->GetDebugLines();
		info.insert(info.begin(), "DEBUG VIEW (F9)");
		DrawTextBlock(12, y, info, 120, 255, 160);
	}

	if (showHelp) {
		DrawTextBlock(800, 12, {
			"H        This help",
			"F9       Colliders + info",
			"F10      God mode (fly)",
			"F11      30 FPS cap",
			"A / D    Move",
			"W / Space Jump",
			"S        Fly down (god)",
			"Shift/J  Dash",
			"Esc      Quit" }, 255, 255, 255);
	}
	else {
		DrawTextBlock(1060, 12, { "H: help" }, 200, 200, 200);
	}
}

// Called each loop iteration
bool Scene::PostUpdate()
{
	bool ret = true;

	DrawDebugOverlay();

	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_ESCAPE) == KEY_DOWN)
		ret = false;

	return ret;
}

// Called before quitting
bool Scene::CleanUp()
{
	LOG("Freeing scene");

	return true;
}