# What The Ground Keeps (WTGKV1.0)

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/)
[![Raylib](https://img.shields.io/badge/Raylib-5.5-red.svg)](https://www.raylib.com/)
[![CMake](https://img.shields.io/badge/CMake-3.21+-green.svg)](https://cmake.org/)
[![Platform](https://img.shields.io/badge/Platform-Windows%20x64-lightgrey.svg)]()

> **"What The Ground Keeps"** is an atmospheric first-person 3D ASCII psychological horror and survival experience built with a custom C++ engine powered by Raylib.

---

## 🎮 Features

- **Custom 3D ASCII Engine**: Real-time instanced glyph shading and ASCII-rendered 3D world.
- **Camera-Relative Procedural Ocean**: Multi-wave Gerstner wave spectrum, underwater diving, Snell's window, and atmospheric post-processing.
- **Procedural Audio Synthesis**: 100% synthetically generated spatial sound effects, ambient drones, and dynamic horror audio layers.
- **Dynamic Shop Atmosphere**: Procedural textures, grime, posters, and interactive shop interior with atmospheric lighting.
- **Cinematic Vehicle Sequence**: Dynamic car cockpit interior simulation and highway driving.
- **Embedded Application Icon**: Native Windows PE executable icon integration.

---

## 📁 Project Architecture

```
WTGKV1.0/
├── CMakeLists.txt              # Primary CMake configuration (C++17, Raylib 5.5, RC)
├── CMakePresets.json           # Presets for configuring and building
├── README.md                   # Game overview and build guide
├── .gitignore                  # Git ignore rules for build and compiler artifacts
├── .vscode/                    # Editor tasks and debugger settings
│   ├── launch.json
│   ├── settings.json
│   └── tasks.json
├── assets/                     # Runtime game assets
│   ├── fonts/                  # TrueType horror and monospace typography
│   └── images/                 # Game banners, logos, and promotional art
└── src/                        # Modular C++ source code
    ├── main.cpp                # Core engine orchestrator and main loop
    ├── audio/                  # Procedural sound synthesis engine
    │   ├── audio_synthesis.h
    │   └── audio_synthesis.cpp
    ├── systems/                # Subsystems, shaders, and simulation components
    │   ├── car_cockpit_cinematic.inl
    │   ├── cloud_system.inl
    │   ├── hand_item_ui.inl
    │   ├── instanced_shader.inl
    │   ├── ocean_system.h
    │   ├── plasma_lightning.inl
    │   ├── procedural_math.inl
    │   ├── shop_atmosphere.h
    │   ├── shop_atmosphere.cpp
    │   ├── shop_item_types.inl
    │   ├── voxel_mesh_system.inl
    │   └── world_structures.inl
    └── resources/              # Windows PE resources & icons
        ├── icon.ico            # High-resolution multi-size application icon
        └── WhatTheGroundKeeps.rc # Windows resource definition
```

---

## 🛠️ Building and Running

### Prerequisites
- **Compiler**: Visual Studio 2022 / Build Tools with MSVC (C++17)
- **Build System**: CMake 3.21 or higher
- **Graphics**: OpenGL 3.3 compatible GPU

### Build Instructions

1. **Configure with CMake**:
   ```powershell
   cmake -B build -S .
   ```

2. **Compile the Game**:
   ```powershell
   cmake --build build --config RelWithDebInfo
   ```

3. **Launch**:
   ```powershell
   .\build\RelWithDebInfo\WhatTheGroundKeeps.exe
   ```

Or open the directory in **Visual Studio Code** and press `Ctrl+Shift+B` to build, followed by `F5` to run with debugging.

---

## 📜 License & Credits

Developed by Saad Muzaffar Awan. Powered by [Raylib](https://www.raylib.com/).

