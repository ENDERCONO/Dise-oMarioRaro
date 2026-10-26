# L10 — Animations and Parameters

**Video Game Development (804237 DESVJ) · CITM UPC**

Builds on `L09_Map_Collision_Solution`. The player gets an animated
spritesheet, the camera follows the player, and the player's tuning comes from
`config.xml` instead of being hardcoded. Check
`L10_Animations_Parameters_Solution` once you're done, or if you get stuck.

## The idea

Three things land in this lecture: the player has an animated spritesheet
instead of a static frame, the camera follows the player instead of sitting
still, and the player's tuning (texture, frame size, speed, sound) comes from
`config.xml` — the same pattern `Module::LoadParameters` already taught for
engine modules, now extended to entities.

`Player`'s shape does not change: `Update()` does the logic and `Draw()` does
the rendering. The animation clock and the camera follow go in `Update()`; the
frame lookup goes in `Draw()`. Add the new work into the existing methods.

`Animation.h`/`.cpp` (clips, and loading them from a Tiled TSX) are given, not
a TODO — you call them, like `Physics` at L08.

## What each TODO does

The numbering starts at 4 on purpose: it matches the slides.

| Marker | File | What you write |
|---|---|---|
| `L10: TODO 4` | `src/Entity.h` | `Entity::SetParameters(pugi::xml_node)` and a `parameters` member — the `Module::LoadParameters` pattern for entities |
| `L10: TODO 4` | `src/Scene.cpp` | In `Scene::Awake()`, call `player->SetParameters(...)` right after `CreateEntity()`, with the `<player>` node from `entityManager->configParameters` |
| `L10: TODO 4` | `config.xml` | Fill in the `<player>` section: texture path, animations TSX path, frame width/height (speed and sound are already there) |
| `L10: TODO 5` | `src/Player.cpp` `Start()` | Read the texture path, frame size and sound from `parameters` instead of hardcoding them; load the animations from the TSX and set `"idle"` as current |
| `L10: TODO 5` | `src/Player.h` | Declare an `AnimationSet anims` |
| `L10: TODO 6` | `src/Player.cpp` `Update()` | `anims.Update(dt)` — logic, so it lives in `Update()` |
| `L10: TODO 6` | `src/Player.cpp` `Draw()` | `anims.GetCurrentFrame()` and pass it to `DrawTexture(...)` — rendering, so it lives in `Draw()` |
| `L10: TODO 7` | `src/Player.cpp` `Update()` | `anims.SetCurrent(...)`: default to `"idle"`, switch to `"move"` while A/D is held |
| `L10: TODO 8` | `src/Player.cpp` `Update()` | Horizontal-only camera follow, clamped to the map's edges |
| `L10: TODO 8` | `src/Map.h`/`.cpp` | `Map::GetMapSizeInPixels()` — the map's size in pixels, used by the clamp |

`Entity::SetParameters()` plus the `parameters` member (TODO 4, `src/Entity.h`)
and `Map::GetMapSizeInPixels()` (TODO 8, `src/Map.h`/`.cpp`) are already
written and only marked: read them, they are what the TODOs above rely on.

The animation TSX (`player2_spritesheet.tsx`) is not in this branch: in class
you create it in Tiled with the tile animation editor (idle = tiles 0–10,
move = tiles 11–21, jump = tiles 22–24, 50 ms per frame). The finished file is
in the solution branch.

Until the TODOs are done the game runs exactly as it did at the end of L09,
except for the changes listed below.

## What changed besides the TODOs

- **Module order.** `entityManager` is now registered before `scene`. Module
  parameters are loaded just before each module's `Awake()`, so this is what
  makes `entityManager->configParameters` available when `Scene::Awake()` runs.
  Registration order is also `Update()`/draw order: entities now update before
  the scene does.
- **Spawn.** The player starts at `(300, 300)`, clear of the left wall.
- **Map.** `MapTemplate.tmx` has colliders on all the platforms, and the
  `Collisions` layer is no longer drawn.
- **Physics debug** starts off. Press **F1** to show the Box2D wireframes.

## Things to know

- **Why `SetParameters()` works.** `CreateEntity()` returns a constructed but
  not-yet-`Awake`'d entity that waits in `EntityManager`'s pending queue (L04).
  That is the window in which you can hand it its data. Call it any later and
  `Start()` would already have read an empty node.
- **The spawn position stays in code.** It is map data, not player config; L15
  loads it from a Tiled object layer. Animation clip names (`idle`, `move`,
  `jump`) stay in code too: they describe the layout of the TSX, not a tunable.
- **The camera is an `SDL_FRect`** since L01, so write a float straight into
  `camera.x`. The follow is horizontal only, and the player sits at the quarter
  mark of the screen so you see more of what is ahead.
- **The arrow-key camera from L03 still exists** in `Scene::Update()`. It runs
  after the follow, so holding Left/Right shifts the view by a few pixels
  instead of scrolling it. Up/Down still move the camera vertically.
- **`GetSize()` is not the answer for the frame size.** The texture is a
  spritesheet, so it returns the whole sheet; the frame size comes from the
  config.
- **`dt` is in seconds**, and so are the animation frame durations.

## Build

Open `PlatformGame.sln`, select **x64**, build and run. **A/D** move and
**F1** toggles the Box2D debug wireframes. After the TODOs the player walks
instead of sliding as a static frame, and the camera scrolls once you walk far
enough from the quarter-mark position.

## Read the code

Start at `Player::Start()`, then `Player::Update()` and `Player::Draw()`, where
the TODO markers sit. `Animation::Update()` and `AnimationSet::LoadFromTSX()`
in `src/Animation.cpp` hold the clip machinery.

## Homework

- Compare this branch with `L10_Animations_Parameters_Solution` once you're
  done; the diff should be only the TODO bodies and the TSX.
- The `"jump"` clip is registered but the player cannot jump yet. Add a jump,
  and switch to the clip when the jump starts and back on landing.
- If the player TSX is missing, `LoadFromTSX()` logs and returns `false`,
  leaving the player idle-only. Given L05's "hard-fail on bad config"
  precedent, should that fail loudly?

## Reference

- Tiled: tile animations — <https://doc.mapeditor.org/en/stable/manual/editing-tilesets/#tile-animation-editor>
- A1 spec: "Parameters loading — read configurations and entity parameters
  from external files: Player parameters (initial position, speed, tiles)"
