# What The Ground Keeps

A first-person horror game in C++17 and raylib 5.5. Everything you see and hear is made by
code: the textures, the meshes, the characters and every sound. The game has no music.

> Two brothers are driving home on Route 9. The car wraps around a radio tower and Adam
> dies. His soul watches a tall, wet man pull his little brother out of the wreck and drag
> him into the dark. Adam wakes up on the road. The blood is still there, and the drag
> marks end at a gas station hatch. The man behind the counter can see him.

The story, the clue trail and the rated feature list are in [`docs/GAME_DESIGN.md`](docs/GAME_DESIGN.md).

---

## Build and run

```bash
cmake --preset default          # or: cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
cd build && ./WhatTheGroundKeeps
```

CMake downloads raylib 5.5 on the first configure. Each build copies `assets/` next to the
executable, so changes to the scene file show up without a rebuild. It is developed on Linux
and Windows (macOS should work too), and needs a GPU with OpenGL 3.3.

## Playing

See **[GUIDE.md](GUIDE.md)** (also in the game under *How to play*) for the walkthrough of every chapter and all the settings.

## Controls

| Action | Keyboard / mouse | Gamepad |
|---|---|---|
| Move / look | WASD / mouse | left stick / right stick |
| Sprint, crouch, jump | Shift, Ctrl or C, Space | L3, R3, — |
| Interact (tap or hold) | E | A |
| Flashlight | F | Y |
| Use held item (pour fuel) | left mouse button | RT |
| Tonight's task list | hold Tab | — |
| Pause | Esc | Start |
| World editor | F10 | — |

## Chapters

| # | Chapter | What happens |
|---|---|---|
| 0 | Prologue | The drive, the figure in the road, the crash. You rise out of your body and watch the Dragger take Zain. |
| 1 | Awakening | You wake on the road and follow the blood trail to the hatch. Grethnar offers a deal. |
| 2 | Night One | Pumps, restocking, mopping, the trash. A car arrives with nobody inside. |
| 3 | Night Two | Rain. You fetch the ledger from Blackwood College. The payphone rings. You get the Polaroid. |
| 4 | Night Three | Fog, a power cut, and the college boiler. Something is scratching behind the bookshelf. |
| 5 | The Key | The key is buried beside the third cow from the fence. She gets up when you take it. |
| 6 | Below | The iron door opens and they come up past you. Underneath is the procession, the dead along the road, and Zain. |
| 7 | Burn | You fill a can, soak the hatch, the pumps and the store, and strike the old man's match. |
| 8 | Dawn | The ending, then the credits. |

The game saves at the start of every chapter (`wtgk.sav`). *Continue* on the main menu
resumes from the last save.

---

## The engine

```
src/engine/   renderer, lit/sky/post shaders, procedural textures and materials, mesh builder,
              collision, terrain, SDF sculpting, audio synthesis, UI, entity/prefab/scene system
src/game/     prefabs (world objects), characters, player, HUD, menus, editor, story chapters
assets/       scenes/world.scene, fonts, images, plus optional texture and sound overrides
docs/         design document
```

### Rendering

- Forward HDR rendering with triplanar, GGX-lit materials and up to 16 dynamic lights.
- Shadows from the moon (orthographic) and from one spotlight, usually your flashlight.
- Height fog and volumetric scattering on lights.
- Post-processing: bloom, ACES tonemapping, grain, vignette, chromatic aberration, FXAA and
  eyelid blinks.
- An **ASCII "spirit sight"** pass that creeps in from the edges of the screen as the ground
  claims more of you.
- Procedural texture sets, generated on worker threads at startup: asphalt, concrete, brick,
  wood, rust, rock, skin, bone and more.

### Objects are objects: the scene and prefab system

Every object in the world is an **entity** made from a named **prefab**. A car is a `sedan`
and a door is a `door_wood`. The whole map is one plain-text file, `assets/scenes/world.scene`,
with one line per object:

```
# prefab      x      y     z      yaw  [pitch roll]  key=value ...
gas_pump      2.4    0.21  1.6    0    parent=canopy name=pump_1
sedan         19.2   0     166.2  52   0 -2 state=wrecked color=blue name=wreck tag=after_crash
door_iron     0.0   -3.0  -7.5    90   parent=college name=iron_door w=1.3 h=2.3 locked=1
cavern       -120   -60   -40     0    abs=1 name=cavern
```

- `y` is measured from the terrain surface unless you set `abs=1`.
- `parent=<name>` places the object relative to another entity. For example, the store's
  shelves move with the store.
- `name=` lets story code find an object (`Scn().Find("pump_1")`). `tag=` groups objects so a
  chapter can show or hide them together.
- Any other `key=value` pair is a prefab property, such as `len`, `variant`, `color`, `locked`
  or `text`.

A prefab is a C++ function that builds geometry, colliders, lights and an interaction point,
and can attach a **behaviour**: doors swing, the bookshelf slides and the windmill turns.
Prefabs are registered by family in `src/game/prefabs_*.cpp`. To add one, write a builder,
register it, and place it in the scene file.

### In-game editor (F10)

Press F10 to open the editor. From there you can:

- Fly with WASD and Q/E, and click an object to select it.
- Move the selection with the arrow keys and PgUp/PgDn, rotate it with R, and cycle its
  variant with V.
- Grab it with G, duplicate it with Ctrl+D, and delete it with Del.
- Resize it with = and -.
- Choose a prefab from the palette with Tab, then place it with P.
- Save back to `world.scene` with Ctrl+S.

### Sound: 100% synthesized, no music

Every sound is synthesized when the game starts. Footsteps on each surface, doors, pumps,
the register, rain and wind, fire, breathing and the heartbeat are all built from modal
resonators, noise grains and stick-slip friction, then passed through a small room reverb.
A live mixer blends the ambience beds (crickets, wind, the hum of the store, drips below
ground) from the game state, and the crickets fall silent when something is near. Sounds are
positioned in 3D and muffled by walls.

### Overriding generated content

- **Sounds:** drop `assets/sounds/<id>_<n>.wav` next to the build, for example
  `door_wood_open_0.wav` or `step_gravel_2.wav`. The game uses your files for that id instead
  of synthesizing it.
- **Textures:** `assets/textures/<set>_albedo.png` and an optional `<set>_normal.png` replace
  a generated texture set, such as `asphalt`, `brick` or `rust`.

### Headless testing

The game can render a single frame to an image, which is how it was developed and checked:

```bash
xvfb-run -a ./WhatTheGroundKeeps --mode chapter --chapter 6 --frames 300 --shot out.png
```

`--frames` runs the simulation at a fixed 30 fps and renders only the last frames, so it is
fast. These environment variables help:

| Variable | Effect |
|---|---|
| `WTGK_CAM=x,y,z,yaw,pitch` | fixed camera |
| `WTGK_WARP=x,z,yaw[,pitch[,y]]` | teleport the player once the chapter starts |
| `WTGK_FLASH=1` | flashlight on |
| `WTGK_BRIGHT=4` | inspection exposure (see detail in the dark) |
| `WTGK_TEST=cow\|fire\|polaroid` | trigger a set-piece for a screenshot |
