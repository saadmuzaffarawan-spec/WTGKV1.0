#pragma once
#include <raylib.h>
#include <math.h>

void DrawShovelUI(int offsetX, int offsetY) {

    int baseX = 900 + offsetX;

    int baseY = 250 + offsetY;

    

    // Right Hand and Shovel ASCII Art

    const char* shovelArt[] = {

        "       _       ",

        "      / \\      ",

        "     /   \\     ",

        "    |  .  |    ",

        "     \\   /     ",

        "      \\ /      ",

        "       |       ",

        "       |       ",

        "     _[ ]_     ",

        "    /     \\    ",

        "   | /\\ /\\ |   ",

        "   \\__ _ __/   ",

        "     | |       ",

        "     | |       "

    };



    // Draw Shovel (Cyan Glow)

    for (int row = 0; row < 14; row++) {

        for (int col = 0; col < 15; col++) {

            char c = shovelArt[row][col];

            if (c == ' ') continue;

            

            int px = baseX + col * 18;

            int py = baseY + row * 26;

            

            // Glow effect

            DrawText(TextFormat("%c", c), px - 1, py, 26, (Color){0, 200, 255, 100});

            DrawText(TextFormat("%c", c), px + 1, py, 26, (Color){0, 200, 255, 100});

            DrawText(TextFormat("%c", c), px, py - 1, 26, (Color){0, 200, 255, 100});

            DrawText(TextFormat("%c", c), px, py + 1, 26, (Color){0, 200, 255, 100});

            // Core

            DrawText(TextFormat("%c", c), px, py, 26, (Color){150, 255, 255, 255});

        }

    }

}



void DrawLanternUI(int offsetX, int offsetY, float timeVal) {

    int baseX = 150 + offsetX;

    int baseY = 320 + offsetY;

    

    // Left Hand holding Lantern

    const char* lanternArt[] = {

        "      ===      ",

        "     /   \\     ",

        "    |  *  |    ",

        "     \\___/     ",

        "       |       ",

        "     _[ ]_     ",

        "    /     \\    ",

        "   | /\\ /\\ |   ",

        "   \\__ _ __/   ",

        "     | |       ",

        "     | |       "

    };



    float fireBob = sinf(timeVal * 4.0f) * 3.0f;



    for (int row = 0; row < 11; row++) {

        for (int col = 0; col < 15; col++) {

            char c = lanternArt[row][col];

            if (c == ' ') continue;



            int px = baseX + col * 18;

            int py = baseY + row * 26;

            

            if (c == '*') {

                py += (int)fireBob; // Bob the light

                // Bright Green Light Glow

                DrawText(TextFormat("%c", c), px - 2, py, 32, (Color){50, 255, 50, 80});

                DrawText(TextFormat("%c", c), px + 2, py, 32, (Color){50, 255, 50, 80});

                DrawText(TextFormat("%c", c), px, py - 2, 32, (Color){50, 255, 50, 80});

                DrawText(TextFormat("%c", c), px, py + 2, 32, (Color){50, 255, 50, 80});

                DrawText(TextFormat("%c", c), px, py, 32, (Color){200, 255, 200, 255});

            } else {

                // Cyan Hand Glow

                DrawText(TextFormat("%c", c), px - 1, py, 26, (Color){0, 200, 255, 100});

                DrawText(TextFormat("%c", c), px + 1, py, 26, (Color){0, 200, 255, 100});

                DrawText(TextFormat("%c", c), px, py, 26, (Color){150, 255, 255, 255});

            }

        }

    }

}



const char* moonArt[] = {

    "   _..._   ",

    " .       . ",

    " |       | ",

    " .       . ",

    "   `...`   "

};
