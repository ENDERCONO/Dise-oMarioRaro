# [Spikewood] — v0.1-alpha

**[Parry God]** — Alejandro Pérez, Ian León  
Assignment 1 · Platformer

## The game

A short precision platformer. Guide the little blue creature through a forest full of pits and spikes, from the first clearing to the far end of the map, using jumps, short hops and a mid-air dash.

Not included yet: enemies, UI, an end-of-level trigger and more than one level.

## How to play

Download `Spikewood.zip`, unzip it and run `Game.exe`.  
Keep `config.xml` and the `Assets` folder next to the executable.

| Key | Action |
|---|---|
| `A` / `D` | Move left / right |
| `SPACE` or `W` | Jump (hold for a high jump, tap for a short hop) |
| `LEFT SHIFT` or `E` | Dash |
| `ESC` | Quit |

Cross the level from left to right. Falling into a pit or touching spikes kills you, and you respawn at the start.

### Debug keys

| Key | Action |
|---|---|
| `H` | Show / hide the help menu |
| `F9` | Draw colliders and logic |
| `F10` | God mode |
| `F11` | Enable / disable the 30 fps cap |

## Features in this release

**Player** — Inherits from `Entity` and is managed by `EntityManager`. Dynamic Box2D capsule body with fixed rotation. Movement and jump are driven by Box2D and normalised with delta time. Each action is its own function (`Move()`, `Jump()`, `Dash()`, `ApplyGravity()`, `Die()`, `Respawn()`...). Animations idle, walk, jump and die, with the sprite facing the direction of travel. Ground detection from the Box2D contact normals. Death by falling into a pit or touching spikes, and respawn at the spawn point defined in the map. The camera follows the player and stops at the map edges.

**Map** — Made in Tiled and saved as `.tmx`, loaded and drawn orthographically. Layers: Background, Trees, Decoration, Platforms, Collisions and a Spawn object layer. Colliders are created once at load time from the Collisions layer using Box2D, with three types: platform, damage and pit. Loading and unloading are organised so more levels can be added (levels are listed in `config.xml`).

**Data from files** — `config.xml` holds the window, frame rate, vsync, map, music and sound paths, and the player parameters (speed, jump force, gravity tuning, dash, hitbox, texture, frame size). Animations are defined in Tiled and loaded from `PlayerBlue.tsx`.

**Engine** — 60 fps cap with vsync off, read from `config.xml`. Box2D runs with a fixed time step, so physics do not depend on the frame rate. The window title shows FPS, average FPS, last-frame ms and vsync on/off.

## Extra features

- **Dash.** `LEFT SHIFT` / `E`, with speed, duration and cooldown in `config.xml`.
- **Drop shadow.** A shadow under the player that shrinks with height.

## Known issues

- At very low frame rates (for example 16 fps) the dash lasts a few ms longer, because its duration is counted in whole frames.

## Credits

Tilesets, sprites, music and sound effects we did not make. Licence files are included next to each asset in `Assets/`.

- Player spritesheet — [author], [licence]
- Forest tileset and trees — [author], [licence]
- Grass, dirt and rock tiles — [author], [licence]
- Music — [author], [licence]
- Sound effects — [author], [licence]

## Team

Alejandro and Ian worked mostly together on Alejandro's computer, so most commits come from his account. Ian also worked separately and sent his changes by email (player sprites and animations, camera changes), which Alejandro merged. The full breakdown is in the work division document delivered on Atenea.
