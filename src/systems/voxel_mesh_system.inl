#pragma once
#include <raylib.h>
#include <vector>
#include <functional>
#include <stdint.h>

#define BUCKET_SIZE 32

#define BUCKETS_X (CHUNK_W / BUCKET_SIZE)

#define BUCKETS_Z (CHUNK_D / BUCKET_SIZE)



struct RenderBucket {

    std::vector<Matrix> instances[256];

    std::vector<uint8_t> activeGlyphs;

};



struct Chunk {

    Voxel voxels[CHUNK_W][CHUNK_H][CHUNK_D];

    RenderBucket buckets[BUCKETS_X][BUCKETS_Z];



    // High-performance local spatial bucket mesh builder (runs in <0.5ms!)

    void BuildBucket(int bx, int bz) {

        if (bx < 0 || bx >= BUCKETS_X || bz < 0 || bz >= BUCKETS_Z) return;

        RenderBucket& bucket = buckets[bx][bz];

        bucket.activeGlyphs.clear();

        for (int i = 0; i < 256; i++) bucket.instances[i].clear();



        auto isOpaque = [&](int vx, int vy, int vz) {

            if (vx < 0 || vx >= CHUNK_W || vy < 0 || vy >= CHUNK_H || vz < 0 || vz >= CHUNK_D) return true;

            Voxel& vv = voxels[vx][vy][vz];

            if (!vv.isSolid) return false;

            char g = vv.glyphIndex;

            if (g == '|' || g == '/' || g == '&' || g == '\'' || g == ',' || g == '.' || g == '"' || g == ';' || g == '`') return false;

            return true;

        };



        struct Face { int dx, dy, dz; };

        Face faces[6] = {

            { 0,  1,  0 }, // 1: Top

            { 0, -1,  0 }, // 2: Bottom

            {-1,  0,  0 }, // 3: Left

            { 1,  0,  0 }, // 4: Right

            { 0,  0, -1 }, // 5: Front

            { 0,  0,  1 }  // 6: Back

        };



        int startX = bx * BUCKET_SIZE;

        int endX   = startX + BUCKET_SIZE;

        int startZ = bz * BUCKET_SIZE;

        int endZ   = startZ + BUCKET_SIZE;



        for (int x = startX; x < endX; x++) {

            for (int y = 0; y < CHUNK_H; y++) {

                for (int z = startZ; z < endZ; z++) {

                    Voxel& v = voxels[x][y][z];

                    if (v.glyphIndex != 0 && v.glyphIndex != ' ') {

                        if (!v.isSolid || v.glyphIndex == '&' || v.glyphIndex == '|' || v.glyphIndex == '/') {

                            Matrix m = MatrixIdentity();

                            bool isGlow = (v.fgColor.a == 254);

                            float glowMultiplier = isGlow ? 1.5f : 1.0f;

                            

                            m.m0 = (v.fgColor.r / 255.0f) * glowMultiplier;

                            m.m1 = (v.fgColor.g / 255.0f) * glowMultiplier;

                            m.m2 = (v.fgColor.b / 255.0f) * glowMultiplier;

                            m.m3 = 1.0f;

                            m.m4 = 1.0f; m.m5 = 1.0f;

                            

                            m.m8 = 0.0f; // isFoliage flag

                            if (v.glyphIndex == '&') m.m8 = 2.0f;

                            else if (!v.isSolid || v.glyphIndex == '|' || v.glyphIndex == '/') m.m8 = 1.0f; // Swaying grass

                            

                            m.m9 = 0.0f;

                            m.m10 = 0.0f; // 0 = Billboard

                            m.m11 = 1.0f; // Full sunlight

                            m.m12 = (float)x; m.m13 = (float)y; m.m14 = (float)z;

                            bucket.instances[(uint8_t)v.glyphIndex].push_back(m);

                            

                            if (isGlow) {

                                Matrix glowM = m;

                                glowM.m0 *= 0.6f; glowM.m1 *= 0.6f; glowM.m2 *= 0.6f;

                                float offsets[4][2] = { {-0.08f, 0.0f}, {0.08f, 0.0f}, {0.0f, -0.08f}, {0.0f, 0.08f} };

                                for(int g = 0; g < 4; g++) {

                                    glowM.m12 = (float)x + offsets[g][0];

                                    glowM.m13 = (float)y + offsets[g][1];

                                    bucket.instances[(uint8_t)v.glyphIndex].push_back(glowM);

                                }

                            }

                        } else {

                            bool isExposed = false;

                            for (int f = 0; f < 6; f++) {

                                if (!isOpaque(x + faces[f].dx, y + faces[f].dy, z + faces[f].dz) ||

                                    !isOpaque(x + faces[f].dx * 2, y + faces[f].dy * 2, z + faces[f].dz * 2) ||

                                    !isOpaque(x + faces[f].dx * 3, y + faces[f].dy * 3, z + faces[f].dz * 3)) {

                                    isExposed = true;

                                    break;

                                }

                            }

                            

                            if (isExposed) {

                                if (fabs((float)x - (CHUNK_W / 2.0f)) < 7.5f) continue;

                                if (x >= 75 && x <= 116 && z >= 118 && z <= 162) continue; // Zero ASCII floating on shop floor



                                int surfaceY = 0;

                                for(int dy = CHUNK_H - 1; dy >= 0; dy--) {

                                    if (voxels[x][dy][z].isSolid && voxels[x][dy][z].glyphIndex == '#') {

                                        surfaceY = dy + 1;

                                        break;

                                    }

                                }

                                

                                int depthFromSurface = surfaceY - y;

                                

                                if (depthFromSurface >= 1 && depthFromSurface <= 5) {

                                    int numInstances = (depthFromSurface == 1) ? GetRandomValue(8, 12) : GetRandomValue(2, 4);

                                    static const char denseGlyphs[] = {'#', '.', '*', '+', '/', '\\', '%', '&', ':', ';'};

                                    

                                    float t = (depthFromSurface - 1) / 4.0f;

                                    float baseR, baseG, baseB;

                                    if (t < 0.5f) {

                                         float localT = t * 2.0f;

                                         baseR = 85.0f + localT * (55.0f - 85.0f);

                                         baseG = 55.0f + localT * (45.0f - 55.0f);

                                         baseB = 35.0f + localT * (40.0f - 35.0f);

                                    } else {

                                         float localT = (t - 0.5f) * 2.0f;

                                         baseR = 55.0f + localT * (25.0f - 55.0f);

                                         baseG = 45.0f + localT * (25.0f - 45.0f);

                                         baseB = 40.0f + localT * (28.0f - 40.0f);

                                    }

                                    

                                    Matrix baseM = MatrixIdentity();

                                    baseM.m3 = 1.0f;

                                    baseM.m8 = 0.0f; baseM.m9 = 0.0f; baseM.m10 = 0.0f; baseM.m11 = 1.0f; 

                                    

                                    for (int i = 0; i < numInstances; i++) {

                                        Matrix m = baseM;

                                        float r = Clamp(baseR + GetRandomValue(-8, 8), 0.0f, 255.0f);

                                        float g = Clamp(baseG + GetRandomValue(-8, 8), 0.0f, 255.0f);

                                        float b = Clamp(baseB + GetRandomValue(-8, 8), 0.0f, 255.0f);

                                        

                                        m.m0 = r / 255.0f; 

                                        m.m1 = g / 255.0f;  

                                        m.m2 = b / 255.0f;                           

                                        

                                        m.m4 = 0.50f + (GetRandomValue(-4, 4) / 100.0f);

                                        m.m5 = 0.50f + (GetRandomValue(-4, 4) / 100.0f);

                                        

                                        float ox = (GetRandomValue(-50, 50) / 100.0f);

                                        float oy = (GetRandomValue(-50, 50) / 100.0f);

                                        float oz = (GetRandomValue(-50, 50) / 100.0f);

                                        

                                        m.m12 = (float)x + ox; 

                                        m.m13 = (float)y + oy; 

                                        m.m14 = (float)z + oz;

                                        

                                        char glyph = denseGlyphs[GetRandomValue(0, 9)];

                                        bucket.instances[(uint8_t)glyph].push_back(m);

                                    }

                                } else {

                                    int numInstances = GetRandomValue(2, 3);

                                    static const char heavyGlyphs[] = {'#', '@', '%', '&', '$', 'W', 'M', '8'};

                                    

                                    Matrix baseM = MatrixIdentity();

                                    baseM.m3 = 1.0f;

                                    baseM.m4 = 1.0f; baseM.m5 = 1.0f;

                                    baseM.m8 = 0.0f; baseM.m9 = 0.0f; baseM.m10 = 0.0f; baseM.m11 = 1.0f; 

                                    

                                    for (int i = 0; i < numInstances; i++) {

                                        Matrix m = baseM;

                                        int baseVal = GetRandomValue(15, 30);

                                        m.m0 = (baseVal + GetRandomValue(5, 15)) / 255.0f; 

                                        m.m1 = (baseVal + GetRandomValue(2, 8)) / 255.0f;  

                                        m.m2 = baseVal / 255.0f;                           

                                        

                                        float ox = (GetRandomValue(-30, 30) / 100.0f);

                                        float oy = (GetRandomValue(-30, 30) / 100.0f);

                                        float oz = (GetRandomValue(-30, 30) / 100.0f);

                                        

                                        m.m12 = (float)x + ox; 

                                        m.m13 = (float)y + oy; 

                                        m.m14 = (float)z + oz;

                                        

                                        char glyph = heavyGlyphs[GetRandomValue(0, 7)];

                                        bucket.instances[(uint8_t)glyph].push_back(m);

                                    }

                                }

                            }

                        }

                    }

                }

            }

        }

        for (int i = 0; i < 256; i++) {

            if (!bucket.instances[i].empty()) {

                bucket.activeGlyphs.push_back((uint8_t)i);

            }

        }

    }



    void BuildMesh(std::function<void(float)> onProgress = nullptr) {

        for (int bx = 0; bx < BUCKETS_X; bx++) {

            if (onProgress) onProgress((float)bx / (float)BUCKETS_X);

            for (int bz = 0; bz < BUCKETS_Z; bz++) {

                BuildBucket(bx, bz);

            }

        }

    }

};
