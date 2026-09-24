#pragma once
#include <raylib.h>

const char* instancedVS = R"(

#version 330

in vec3 vertexPosition;

in vec2 vertexTexCoord;

layout(location = 4) in mat4 instanceTransform;



out vec2 fragTexCoord;

out vec4 fragColor;

out float fragIsDirt;



uniform mat4 matView;

uniform mat4 matProjection;

uniform vec2 uvOffset;

uniform vec2 uvScale;

uniform float time;

uniform vec3 playerPos;

uniform vec3 trailPos[16];

uniform float trailLife[16];

uniform float lightningFlash;

uniform vec3 sunDir;

uniform float dayFactor;

uniform vec3 sunColor;

uniform vec3 moonDir;

uniform float nightFactor;



void main()

{

    vec3 position = vec3(instanceTransform[3][0], instanceTransform[3][1], instanceTransform[3][2]);

    fragColor = vec4(instanceTransform[0][0], instanceTransform[0][1], instanceTransform[0][2], instanceTransform[0][3]);

    

    // Apply lightning flash additive blending

    fragColor.rgb += vec3(lightningFlash);

    

    vec2 size = vec2(instanceTransform[1][0], instanceTransform[1][1]);

    

    float isFoliage = instanceTransform[2][0];

    float disableFog = instanceTransform[2][1];

    float faceId = instanceTransform[2][2]; 

    float sunlight = instanceTransform[2][3];



    // --- DYNAMIC DIRECTIONAL SUNLIGHT & MOONLIGHT ILLUMINATION ---

    // No self-illumination: directly illuminated by Sun in day and Moon at night!

    if (disableFog < 0.5) {

        vec3 norm = vec3(0.0, 1.0, 0.0);

        if (faceId > 0.5 && faceId < 1.5) norm = vec3(0.0, 1.0, 0.0);

        else if (faceId < 2.5) norm = vec3(0.0, -1.0, 0.0);

        else if (faceId < 3.5) norm = vec3(-1.0, 0.0, 0.0);

        else if (faceId < 4.5) norm = vec3(1.0, 0.0, 0.0);

        else if (faceId < 5.5) norm = vec3(0.0, 0.0, -1.0);

        else if (faceId >= 5.5) norm = vec3(0.0, 0.0, 1.0);



        float nDotL_Sun = max(0.0, dot(norm, sunDir));

        float nDotL_Moon = max(0.0, dot(norm, moonDir));

        float sunDiff = (isFoliage > 0.5) ? (0.35 + 0.65 * max(0.0, sunDir.y)) : (0.25 + 0.75 * nDotL_Sun);

        float moonDiff = (isFoliage > 0.5) ? (0.35 + 0.65 * max(0.0, moonDir.y)) : (0.25 + 0.75 * nDotL_Moon);



        vec3 directSun = sunColor * sunDiff * 1.35 * dayFactor;

        // Directional cool silver moonlight illuminating surfaces facing the moon

        vec3 directMoon = vec3(0.45, 0.58, 0.85) * moonDiff * 0.80 * nightFactor;

        // Natural pitch-dark starlight ambient at night (0.04) vs daylight ambient (0.40)

        vec3 ambient = mix(vec3(0.035, 0.045, 0.070), vec3(0.40, 0.42, 0.46), dayFactor);

        if (isFoliage > 2.5) {
            // Player character visibility floor: preserve crisp, readable silhouette at night
            ambient = max(ambient, vec3(0.45, 0.48, 0.55));
        }
        fragColor.rgb *= (ambient + directSun + directMoon);

    }



    vec3 tRight, tUp;

    if (faceId < 0.5) { 

        tRight = vec3(matView[0][0], matView[1][0], matView[2][0]);

        tUp = vec3(matView[0][1], matView[1][1], matView[2][1]);

    } else if (faceId < 1.5) { // Top

        tRight = vec3(1.0, 0.0, 0.0);

        tUp    = vec3(0.0, 0.0, -1.0);

    } else if (faceId < 2.5) { // Bottom

        tRight = vec3(1.0, 0.0, 0.0);

        tUp    = vec3(0.0, 0.0, 1.0);

    } else if (faceId < 3.5) { // Left

        tRight = vec3(0.0, 0.0, 1.0);

        tUp    = vec3(0.0, 1.0, 0.0);

    } else if (faceId < 4.5) { // Right

        tRight = vec3(0.0, 0.0, -1.0);

        tUp    = vec3(0.0, 1.0, 0.0);

    } else if (faceId < 5.5) { // Front

        tRight = vec3(-1.0, 0.0, 0.0);

        tUp    = vec3(0.0, 1.0, 0.0);

    } else { // Back

        tRight = vec3(1.0, 0.0, 0.0);

        tUp    = vec3(0.0, 1.0, 0.0);

    }



    if (isFoliage > 0.5 && isFoliage < 2.5) {

        float swayOffset = position.x * 0.5 + position.z * 0.5;

        float intensity = (isFoliage > 1.5) ? 0.35 : 0.20;

        float swayX = sin(time * 1.5 + swayOffset) * intensity;

        float swayZ = cos(time * 1.2 + swayOffset) * intensity;

        

        float bend = (vertexPosition.y + 0.5);

        if (isFoliage > 1.5) bend = 1.0;

        

        position.x += swayX * bend;

        position.z += swayZ * bend;

        

        // --- MEADOW WAKE TRAIL EFFECT ---

        float maxTrample = 0.0;

        for (int i = 0; i < 16; i++) {

            if (trailLife[i] > 0.0) {

                float d = distance(position, trailPos[i]);

                if (d < 1.8) {

                    float trample = (1.0 - (d / 1.8)) * trailLife[i];

                    if (trample > maxTrample) maxTrample = trample;

                }

            }

        }

        

        if (maxTrample > 0.0) {

            size.y *= (1.0 - maxTrample * 0.8);

            position.y -= maxTrample * 0.4 * bend;

        }



        // --- PLAYER PARTING EFFECT ---

        float dist = distance(position, playerPos);

        if (dist < 2.5) {

            float pushStrength = (2.5 - dist) * 0.5; // Closer = stronger push

            vec3 pushDir = normalize(vec3(position.x - playerPos.x, 0.0, position.z - playerPos.z));

            position.x += pushDir.x * pushStrength * bend;

            position.z += pushDir.z * pushStrength * bend;

            position.y -= pushStrength * 0.4 * bend;

        }

    }



    vec3 vertexPos = position 

                     + tRight * vertexPosition.x * size.x 

                     + tUp * vertexPosition.y * size.y;



    fragTexCoord = vertexTexCoord * uvScale + uvOffset;

    

    vec4 viewPos = matView * vec4(vertexPos, 1.0);

    gl_Position = matProjection * viewPos;

}

)";



const char* instancedFS = R"(

#version 330

in vec2 fragTexCoord;

in vec4 fragColor;

in float fragIsDirt;



out vec4 finalColor;

uniform sampler2D texture0;



void main()

{

    vec4 texColor = texture(texture0, fragTexCoord);

    

    if (texColor.a < 0.1) {
        discard;
    } else {
        finalColor = texColor * fragColor;
    }
}

)";



Mesh GenQuadMesh() {

    Mesh mesh = {0};

    mesh.vertexCount = 4;

    mesh.triangleCount = 2;

    mesh.vertices = (float*)MemAlloc(mesh.vertexCount * 3 * sizeof(float));

    mesh.texcoords = (float*)MemAlloc(mesh.vertexCount * 2 * sizeof(float));

    mesh.indices = (unsigned short*)MemAlloc(mesh.triangleCount * 3 * sizeof(unsigned short));

    

    mesh.vertices[0] = -0.5f; mesh.vertices[1] = -0.5f; mesh.vertices[2] = 0.0f;

    mesh.texcoords[0] = 0.0f; mesh.texcoords[1] = 1.0f;

    mesh.vertices[3] = -0.5f; mesh.vertices[4] = 0.5f; mesh.vertices[5] = 0.0f;

    mesh.texcoords[2] = 0.0f; mesh.texcoords[3] = 0.0f;

    mesh.vertices[6] = 0.5f; mesh.vertices[7] = 0.5f; mesh.vertices[8] = 0.0f;

    mesh.texcoords[4] = 1.0f; mesh.texcoords[5] = 0.0f;

    mesh.vertices[9] = 0.5f; mesh.vertices[10] = -0.5f; mesh.vertices[11] = 0.0f;

    mesh.texcoords[6] = 1.0f; mesh.texcoords[7] = 1.0f;

    

    mesh.indices[0] = 0; mesh.indices[1] = 2; mesh.indices[2] = 1;

    mesh.indices[3] = 0; mesh.indices[4] = 3; mesh.indices[5] = 2;

    

    UploadMesh(&mesh, false);

    return mesh;

}



Texture2D CreateGlyphAtlas() {

    const int cols = 16;

    const int rows = 16;

    const int glyphSize = 32;

    

    RenderTexture2D target = LoadRenderTexture(cols * glyphSize, rows * glyphSize);

    

    BeginTextureMode(target);

    ClearBackground(BLANK);

    

    for (int i = 0; i < 256; i++) {

        int x = (i % cols) * glyphSize;

        int y = (i / cols) * glyphSize;

        char text[2] = {(char)i, '\0'};

        int w = MeasureText(text, 20);

        DrawText(text, x + (glyphSize - w)/2, y + 6, 20, WHITE);

    }

    

    EndTextureMode();

    

    Image img = LoadImageFromTexture(target.texture);

    ImageFlipVertical(&img); 

    Texture2D atlas = LoadTextureFromImage(img);

    UnloadImage(img);

    UnloadRenderTexture(target);

    

    return atlas;

}
