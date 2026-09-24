// In-game world editor (F10): fly around, select, move, rotate, duplicate,
// delete and place prefabs, then save back to assets/scenes/world.scene.
#pragma once
#include "engine/common.h"

void EditorEnter();
void EditorUpdate(float dt);
void EditorDraw();
Camera3D EditorCamera();
