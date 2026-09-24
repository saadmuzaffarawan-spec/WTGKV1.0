# WHAT THE GROUND KEEPS — Game Design (v2)

> A first-person psychological horror game about a dead older brother, a night-shift job
> at a gas station on Route 9, and the thing that lives under the ground.

This document is the single source of truth for the v2 rebuild. It records what was
brainstormed, how each idea was validated and rated, and what made the cut.

---

## 1. Audit of v1 (why the rebuild)

| Area | v1 state | Verdict |
|---|---|---|
| World | 256×32×256 voxel grid rendered as ASCII glyph quads; buildings are flat-coloured `DrawCube` slabs | Root cause of the "Roblox" look. Replaced by heightfield terrain + real meshes. |
| Code structure | ~10,000-line `main()`, hard-coded coordinates everywhere | Nothing is an object; nothing is editable. Replaced by entities + prefabs + a scene file + an in-game editor. |
| Collision | voxel lookups, missing on most "real shape" geometry | Replaced by capsule vs. oriented boxes / cylinders / terrain. |
| Sky | flat colour + sprite moon | Replaced by an atmospheric night-sky shader. |
| Lighting | flat vertex colour, no real light falloff | Replaced by per-pixel lit, textured, fogged, shadowed rendering. |
| Audio | good idea (100% synthesized), but single fixed sample per sound, no spatialization | Kept the approach, rewrote the DSP: variants, surface-aware footsteps, 3D panning/attenuation, room reverb, ambience beds. |
| Story | sister, engine failure, no connection to the shopkeeper | Rewritten to the brother story below. |
| Worth keeping (ported as ideas) | Grethnar's "stare and he stares back" mechanic, pump CRT, register/receipt, the shovel dig, skeleton cows, IBM Plex Mono typography, "forensic evidence" menu tone | Re-implemented on the new core. |

---

## 2. Story (final)

**Adam** (player, 19) drives his younger brother **Zain** (14) home at dusk on Route 9.
They laugh; Zain teases, fiddles with the radio, grabs the wheel as a joke ("relax, I'm kidding").
A tall figure stands in the headlights. The wheel jerks, the car skids, drifts and wraps
around the base of a radio tower. Adam dies.

His **ruh** (soul) rises out of the body. From above he watches **The Dragger** — a
tall, wet, emaciated man — pull Zain out, throw him on the asphalt and drag him by the
ankle into the dark. Adam blacks out.

He wakes on the road. The blood is still there. Past the wreck: **Blackwood College**
(abandoned), **Grethnar's** store and the **Route 9 gas station**. The drag marks end at
the gas station's underground-tank hatch.

**Mr. Grethnar Woule** can see him. *"You're one of the kept now."* Deal: work three
night shifts; after each one Grethnar tells Adam something about where the ground took
his brother.

### Hidden logic (all foreshadowed, revealed at the end)
* The gas station's underground tanks are not full of petrol. They feed **the ground**
  (the world below). Creatures later come up to drink from the pumps.
* Every shift Adam works **binds** his soul to the land: he gets heavier (footsteps grow
  louder), can carry heavier things, and the world at the edge of his vision turns to text
  (the ASCII "spirit sight" slowly creeps inward).
* **Zain made a deal.** Their mother died a year ago; the ground keeps her. Grethnar
  promised her back for "a life of the same blood". Zain yanked the wheel on purpose. He
  was never screaming while being dragged.
* The old man in the college's hidden room is a previous worker — a kept soul — who warns
  Adam and tells him where the key is.

### Clue trail (for the "did you notice?" rewatch moment)
1. Intro: Zain's hand is on the wheel at the moment of the skid; he looks at the figure,
   not at Adam.
2. Spirit scene: Zain doesn't scream or struggle while dragged.
3. Shift 2 reward: a Polaroid of Zain and their mother — Grethnar is in the background.
4. Old man: *"The boy wasn't screaming, was he?"*
5. The chair ropes in the underground are tied loosely, from the front.

### Ending
Adam finds Zain tied to a chair at the roots. Untied, Zain admits everything. Adam pumps
fuel, pours it across the world (college → pumps → store → the hatch), finds a fire source
(the old man's matchbook) and lights it. The fire runs along the trails. Zain panics and
runs back toward the pumps; the fire reaches him; Adam — solid now, because the job made
him "kept" — throws himself over his brother and burns. Zain survives, and for the first
time really cries. Dawn. Credits. A final shot: something under the burnt ground breathes.

---

## 3. Brainstorm → validate → rate

Scoring: **Fear** (does it produce dread?), **Clarity** (is it obvious what to do?),
**Share** (would a streamer clip it?), **Cost** (1 = cheap, 5 = expensive). Kept if
(Fear+Clarity+Share) ≥ 11 and it fits the story.

| Idea | Fear | Clarity | Share | Cost | Decision |
|---|---|---|---|---|---|
| Playable drive with brother banter, then crash | 3 | 5 | 5 | 3 | **KEEP** – emotional hook |
| Figure standing in the headlights | 5 | 5 | 5 | 1 | **KEEP** – thumbnail image |
| Slow-motion crash, glass, tinnitus | 4 | 5 | 5 | 2 | **KEEP** |
| Soul leaves body, third-person look at own corpse | 4 | 5 | 5 | 2 | **KEEP** |
| Can't move while brother is dragged (input resists) | 5 | 4 | 5 | 1 | **KEEP** |
| Spirit sight: world turns to ASCII text in the dark, creeping inward over the game | 4 | 4 | 5 | 2 | **KEEP** – signature visual |
| Grethnar stares back if you stare at him | 5 | 4 | 5 | 1 | **KEEP** (ported from v1) |
| Pump customers: stop the pump on the exact amount | 2 | 5 | 3 | 2 | **KEEP** – core job skill |
| Customer car with nobody inside | 5 | 5 | 5 | 1 | **KEEP** |
| Mop blood stains (they come back as words) | 4 | 5 | 5 | 2 | **KEEP** |
| Restock shelves from the storage room | 1 | 5 | 1 | 2 | **KEEP** (breather task, pacing) |
| Take trash out to the dumpster in the dark | 5 | 5 | 4 | 1 | **KEEP** |
| Breaker box during power cut | 4 | 5 | 3 | 2 | **KEEP** |
| Tank dipstick comes up with hair on it | 5 | 5 | 5 | 1 | **KEEP** |
| Payphone ringing; breathing on the line | 4 | 5 | 4 | 1 | **KEEP** |
| Shovel dig for the key in the cow field; cow skeleton comes alive | 5 | 4 | 5 | 3 | **KEEP** |
| Underground: animals riding humans in carts, pig-man on a bicycle | 4 | 3 | 5 | 4 | **KEEP** as staged tableaux |
| Dead bodies that reanimate when touched | 5 | 4 | 5 | 2 | **KEEP** |
| Creatures drink fuel from the pumps / eat skeleton cows | 4 | 4 | 5 | 3 | **KEEP** |
| Pour fuel trails, fire propagates along them | 3 | 5 | 5 | 3 | **KEEP** – finale |
| Combat with guns | 2 | 5 | 2 | 4 | CUT – breaks the vulnerable-soul fantasy |
| Stamina/hunger/thirst survival bars | 1 | 3 | 1 | 2 | CUT – busywork |
| Open-world ocean + diving (v1) | 1 | 2 | 1 | 5 | CUT – unrelated to story |
| ATM mini-game (v1) | 1 | 4 | 1 | 3 | CUT from main path |
| Background music | – | – | – | – | CUT by design: **no music, only real sounds** |
| Jumpscare every 30 s | 2 | – | 3 | 1 | CUT – dread over startle; max ~1 real scare per shift |

---

## 4. Core loop

**Normal → Strange → Investigation → Discovery → Consequence → Deeper mystery**

Each **shift** (night) is one turn of that loop:

1. *Normal* — Grethnar hands you a task list (procedurally picked from the pool and
   placed at randomized locations). Mundane work with satisfying micro-feedback.
2. *Strange* — one scripted anomaly per shift (driverless car, stains that write words,
   phone call, dipstick hair).
3. *Investigation* — an errand that leads off the lot (college ledger, boiler room).
4. *Discovery* — a story object or place (Polaroid, hidden room, key).
5. *Consequence* — the world changes permanently (fog thicker, creatures, spirit sight
   creeps in, Grethnar gets less human).
6. *Deeper mystery* — the shift reward: one fact about Zain that raises a new question.

Why it's addictive: short tasks (30–90 s) with a clear finish tick and cash sound, a
visible pay total at the end of the shift, and a **story reward you can't get any other
way** after every shift. The "one more shift" pull comes from the story, not from grinding.

### Task pool (procedural)
`SERVE_PUMP`, `RESTOCK`, `MOP_STAINS`, `TRASH_OUT`, `FIX_BREAKER`, `CHECK_TANK`,
`ANSWER_PHONE`, `REGISTER_CUSTOMER`. Each shift picks 3–5 with a weighted random draw
(seeded per save), with story tasks inserted at fixed positions.

### Structure
| Chapter | Content |
|---|---|
| 0 Prologue | Drive, crash, soul, the Dragger |
| 1 Awakening | Wake on road, follow drag marks, meet Grethnar, accept the deal |
| 2 Shift One | Tutorial tasks; anomaly: driverless car. Reward: Zain's sneaker |
| 3 Shift Two | Tasks + fetch the ledger from the college. Reward: the Polaroid |
| 4 Shift Three | Tasks + relight the college boiler → hidden room → old man → key hint |
| 5 The Key | Dig in the cow field, skeleton cow wakes, escape with the key |
| 6 Below | Open the door; creatures rush up; explore the underground; find Zain; the twist |
| 7 Burn | Fuel cans, pour trails, matchbook, fire, sacrifice, credits |

---

## 5. Art direction

* **Realistic scale** (1 unit = 1 m, 1.75 m eye height, 3.6 m lanes, 2.1 m doors).
* **Materials over models**: every surface is textured (triplanar, procedurally generated
  albedo/normal/roughness): asphalt with cracks and oil, stained concrete, rusted and
  painted metal, rotten wood, brick, dead grass, mud, tile.
* **Lighting tells the story**: sodium-orange canopy lights, a cold moon, one flickering
  tube in the store, the flashlight with a real shadow, the red aviation light blinking on
  the radio tower. Darkness is the default; light is a place of safety.
* **Fog** is thick height fog; distant things are silhouettes.
* **Post**: ACES tonemap, bloom only on light sources, film grain, vignette, slight
  chromatic aberration at the edges, cold colour grade.
* **Spirit sight (ASCII)**: in the darkest parts of the image and at the edge of vision,
  pixels are replaced by characters from IBM Plex Mono, sized to the pixel's brightness.
  It starts almost invisible and creeps inward as the ground claims Adam. It is never used
  on bright areas, so it never hurts the eyes.
* **Characters** are sculpted from signed-distance shapes (smooth, organic, never boxy),
  mostly seen in low light, in fog, from behind, or in silhouette.

## 6. Sound direction — 100% non-musical

No music anywhere, including menus. Every sound is a (synthesized) physical event:
wind with gusts, crickets and frogs that go silent when something is near, distant highway
hiss, sodium-lamp buzz, fridge compressors, fluorescent ballast tick, rain, thunder,
footsteps per surface (asphalt, gravel, grass, wood, tile, metal, concrete), door creaks,
switch clicks, pump motor and fuel flow, car engine, tire screech, crash impact, glass,
heartbeat, tinnitus, fire. Sounds come in randomized variants and are positioned in 3D.
Any `.wav` placed in `assets/sounds/<id>_N.wav` overrides the synthesized version.

## 7. Technical architecture

```
src/engine/   renderer, shaders, procedural textures, mesh builder, SDF sculpting,
              terrain, vegetation, sky, audio + DSP, physics, entities, scene I/O,
              prefab library, in-game editor, UI toolkit
src/game/     game states, player, characters + animation, AI, cutscenes, story director,
              job/task system, fire, HUD, menus, save/settings
assets/scenes/world.scene   every placed object, human-editable text
```

**Everything placed in the world is an entity** created from a named prefab
(`sedan`, `gas_pump`, `radio_tower`, `street_lamp`, `dead_tree`…) with a transform and
optional properties. Prefabs build their own mesh, colliders, lights and interaction
hooks. Press **F10** in game to open the editor: select, move, rotate, duplicate, delete,
place new prefabs, and save back to `world.scene`.
