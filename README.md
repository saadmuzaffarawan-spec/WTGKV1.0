# What The Ground Keeps (WTGKV1.0)

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/)
[![Raylib](https://img.shields.io/badge/Raylib-5.5-red.svg)](https://www.raylib.com/)
[![CMake](https://img.shields.io/badge/CMake-3.21+-green.svg)](https://cmake.org/)
[![Platform](https://img.shields.io/badge/Platform-Windows%20x64-lightgrey.svg)]()

> **"What The Ground Keeps"** is a highly atmospheric, psychological horror and survival simulation game built entirely from scratch using a custom C++ engine powered by Raylib. 

This document serves as an **honest, highly detailed technical and feature breakdown** of everything currently implemented in V1.0 of the game.

---

## 🏗️ Core Engine & Architecture
Unlike games built in Unity or Unreal, every system in *WTGK* is coded at a low level in C++17.
*   **Custom Rendering Pipeline:** Built on top of Raylib, utilizing lgl for immediate mode and batched 3D rendering. Features complex render-to-texture pipelines (used for CCTVs and CRT screens) that seamlessly integrate into the 3D world.
*   **Procedural Audio Engine (udio_synthesis.cpp):** The game does **not load external audio files** (no .wav or .mp3). 100% of the game's sound effects—from ambient night drones and jumpscare stingers to ATM stepper motors and UI beeps—are synthesized mathematically at runtime using oscillators (sine, square, sawtooth, noise).
*   **Sharp Typography (ui_helpers.cpp):** Custom implementations for rendering high-fidelity TrueType fonts (IBMPlexMono, lagard) avoiding native scaling artifacts, resulting in crisp AAA-quality HUDs and panels.

---

## 🌍 The World & Environments
The world is an open, eerie expanse composed of distinct procedural and handcrafted setpieces:

### 1. The Summit Valley Gas Station & Shop
The focal point of the environment.
*   **Procedural Shop Assets:** Gondolas, shelving, and checkout counters are procedurally assembled and populated (procedural_shop_assets.cpp).
*   **Dynamic Lighting:** A claustrophobic mix of warm spotlights and ambient darkness (shop_lighting.cpp), reacting to the player's presence.
*   **Seamless Culling:** A custom rendering pipeline ensures that when the player enters the shop, massive outdoor geometry (like the college and ground plane) is managed cleanly to prevent Z-fighting and matrix projection bugs.

### 2. Blackwood College
A massive, foreboding architectural setpiece looming in the distance. It utilizes intense, warm lanterns that pierce through the pitch-black volumetric fog, serving as a beacon in the wasteland (lackwood_college.cpp).

### 3. The Highway & Crashed Sedan
Long stretches of procedural roads (world_structures.inl) peppered with environmental storytelling, including a wrecked sedan site (crashed_sedan_site.cpp).

### 4. The Procedural Ocean
A camera-relative dynamic body of water (ocean_system.h). It calculates a multi-wave Gerstner spectrum on the CPU/GPU, complete with underwater diving mechanics, atmospheric underwater post-processing, and Snell's window refraction.

---

## 🌦️ Weather, Skybox, & Atmosphere
The dread of the game is driven by its dynamic weather systems:
*   **Cloud Layers (cloud_system.inl):** Dynamic moving cloud coverage (ranging from clear skies to dense overcast) that masks the moonlight.
*   **Plasma Lightning (plasma_lightning.inl):** Procedural, branching lightning strikes that flash and dynamically illuminate the entire world geometry.
*   **Atmospheric Particles (tmospheric_particles.cpp):** Floating ash, dust, or snow that reacts to the environment.
*   **Volumetric Fog & Night Sky:** A heavily fog-occluded world contrasted against a starry sky and glowing moon.

---

## 👁️ Entities & NPCs
### 1. Mr. Grethnar Woule (The Shopkeeper)
An intensely creepy, interactive horror entity standing behind the checkout counter (grethnar_system.cpp).
*   **Dynamic Staring Mechanic:** He tracks the player's camera dot-product. If you stare at him, he stares back.
*   **Procedural Blood Physics:** If you hold eye contact too long, his eyes engorge and drip procedural blood particles that possess gravity, velocity, and collision—splattering directly onto the shop counter.
*   **Jumpscare State Machine:** If you prime him by staring, look away to an empty counter, and look back, he triggers a brutal jumpscare utilizing synthesized audio, violent camera shake, and FOV manipulation.

### 2. The Wasteland Fauna
*   **The Hound (hound_npc.cpp)**: A stalking entity in the darkness.
*   **The Skeleton Cow (skeleton_cow_npc.cpp)**: Macabre, surreal environmental setpieces.

---

## 🕹️ Interactive Mechanics & Systems

### 1. The 1980s ATM Terminal (tm_system.cpp)
A breathtakingly detailed, fully interactive banking terminal ("Summit Valley Trust"):
*   **State Machine:** Insert Card $\rightarrow$ Read Chip $\rightarrow$ PIN Entry (masking digits) $\rightarrow$ Central Host Authentication $\rightarrow$ Withdrawal Menu $\rightarrow$ Dispense / Error.
*   **CRT Simulation:** Renders to a 2D texture mapped onto 3D geometry. Features heavy visual post-processing: dark vignette edges, phosphor scanlines, and a scrolling translucent V-Sync glitch bar.
*   **Economy & Feedback:** Fully functional account balances. If the player attempts to withdraw more than their $840.00 balance, the ATM triggers an ATM_INSUFFICIENT_FUNDS state, buzzing an error and flashing a red "TRANSACTION DECLINED" screen.
*   **AAA UI Prompts:** Uses DrawAAAPanel for sleek, floating, screen-space contextual interaction prompts (e.g., "[E] COLLECT CASH").

### 2. CCTV Surveillance (cctv_surveillance.cpp)
Working security cameras that render alternate viewpoints of the shop and the player to off-screen framebuffers, displaying them on monitors inside the game world.

### 3. Player Mechanics
*   **Footprint Decals (ootprint_decals.cpp):** The player leaves physical footprints mapped to the ground geometry as they walk.
*   **Shovel & Hand Items (shovel_system.cpp, hand_item_ui.inl):** A first-person viewmodel system allowing the player to wield items with procedural view-bobbing and interaction logic.
*   **Phone System (phone_system.cpp):** Interactive telephone mechanics.
*   **Receipt Printer (eceipt_printer.cpp):** Dynamically prints physical receipts for transactions.

### 4. Cinematics
*   **Intro & Main Menu (intro_cinematic.cpp, main_menu_system.cpp):** Fully scripted opening sequences.
*   **Car Cockpit (car_cockpit_cinematic.inl):** A cinematic driving sequence from inside the vehicle's interior.

---

## 🛠️ Building and Running

### Prerequisites
- **Compiler**: Visual Studio 2022 / Build Tools with MSVC (C++17)
- **Build System**: CMake 3.21 or higher
- **Graphics**: OpenGL 3.3 compatible GPU

### Build Instructions

1. **Configure with CMake**:
   `powershell
   cmake -B build -S .
   `

2. **Compile the Game**:
   `powershell
   cmake --build build --config RelWithDebInfo
   `

3. **Launch**:
   `powershell
   .\build\RelWithDebInfo\WhatTheGroundKeeps.exe
   `

*(Alternatively, open the directory in **Visual Studio Code** and press Ctrl+Shift+B to build, followed by F5 to run with debugging).*

---

## 📜 License & Credits
Developed by Saad Muzaffar Awan. Powered by [Raylib](https://www.raylib.com/).
