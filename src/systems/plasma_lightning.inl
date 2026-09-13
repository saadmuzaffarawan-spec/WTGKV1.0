#pragma once
#include <raylib.h>
#include <raymath.h>
#include <vector>
#include <utility>

// =========================================================================
// HYPERREALISTIC PLASMA LIGHTNING SYSTEM
// Multi-layer high-current discharge: White Core + Cyan Sheath + UV Bloom
// =========================================================================

inline void GenerateLightningBolt(Vector3 start, Vector3 end, int generations, std::vector<std::pair<Vector3, Vector3>>& segments) {
    if (generations == 0) {
        segments.push_back({start, end});
        return;
    }
    
    Vector3 mid = { (start.x + end.x) / 2.0f, (start.y + end.y) / 2.0f, (start.z + end.z) / 2.0f };
    
    float displace = (generations * generations) * 0.8f; 
    mid.x += (GetRandomValue(-100, 100) / 100.0f) * displace;
    mid.y += (GetRandomValue(-100, 100) / 100.0f) * displace;
    mid.z += (GetRandomValue(-100, 100) / 100.0f) * displace;
    
    GenerateLightningBolt(start, mid, generations - 1, segments);
    GenerateLightningBolt(mid, end, generations - 1, segments);
    
    if (GetRandomValue(0, 100) < 40) { 
        Vector3 branchEnd = mid;
        branchEnd.x += (GetRandomValue(-100, 100) / 100.0f) * displace * 3.0f;
        branchEnd.y -= GetRandomValue(10, 40); 
        branchEnd.z += (GetRandomValue(-100, 100) / 100.0f) * displace * 3.0f;
        GenerateLightningBolt(mid, branchEnd, generations - 1, segments);
    }
}

inline void DrawPlasmaLightningBolt(const std::vector<std::pair<Vector3, Vector3>>& segments,
                                   float flashTimer, Vector3 impactPos, float impactTimer) {
    if (flashTimer <= 0.0f || segments.empty()) return;
    
    float a = Clamp(flashTimer, 0.0f, 1.0f);
    unsigned char alphaCore   = (unsigned char)(255.0f * a);
    unsigned char alphaCyan   = (unsigned char)(215.0f * a);
    unsigned char alphaViolet = (unsigned char)(95.0f  * a);
    
    Color colCore   = { 255, 255, 255, alphaCore };
    Color colCyan   = { 170, 235, 255, alphaCyan };
    Color colViolet = { 135,  90, 255, alphaViolet };
    
    // 1. Layer 1: Wide Ultraviolet / Violet Corona Glow
    for (const auto& seg : segments) {
        DrawLine3D((Vector3){ seg.first.x + 0.28f, seg.first.y, seg.first.z + 0.28f },
                   (Vector3){ seg.second.x + 0.28f, seg.second.y, seg.second.z + 0.28f }, colViolet);
        DrawLine3D((Vector3){ seg.first.x - 0.28f, seg.first.y, seg.first.z - 0.28f },
                   (Vector3){ seg.second.x - 0.28f, seg.second.y, seg.second.z - 0.28f }, colViolet);
        DrawLine3D((Vector3){ seg.first.x, seg.first.y + 0.28f, seg.first.z },
                   (Vector3){ seg.second.x, seg.second.y + 0.28f, seg.second.z }, colViolet);
    }
    
    // 2. Layer 2: Ionized Plasma Sheath (Electric Cyan)
    for (const auto& seg : segments) {
        DrawLine3D((Vector3){ seg.first.x + 0.09f, seg.first.y, seg.first.z - 0.09f },
                   (Vector3){ seg.second.x + 0.09f, seg.second.y, seg.second.z - 0.09f }, colCyan);
        DrawLine3D((Vector3){ seg.first.x - 0.09f, seg.first.y, seg.first.z + 0.09f },
                   (Vector3){ seg.second.x - 0.09f, seg.second.y, seg.second.z + 0.09f }, colCyan);
    }
    
    // 3. Layer 3: Blinding Stark White Core Channel
    for (const auto& seg : segments) {
        DrawLine3D(seg.first, seg.second, colCore);
    }
    
    // 4. Plasma Node Beads at branch junctions
    if (a > 0.20f) {
        for (size_t s = 0; s < segments.size(); s += 2) {
            DrawCube(segments[s].first, 0.38f, 0.38f, 0.38f, { 220, 245, 255, (unsigned char)(235.0f * a) });
        }
    }
    
    // 5. Ground Impact Plasma Burst & Shockwave Ring
    if (impactTimer > 0.0f) {
        float blastProg = Clamp(impactTimer / 0.5f, 0.0f, 1.0f);
        float ringRadius = (1.0f - blastProg) * 8.5f + 0.6f;
        unsigned char ringAlpha = (unsigned char)(blastProg * 255.0f);
        
        // Expanding ionized shockwave ring
        DrawCircle3D(impactPos, ringRadius, (Vector3){ 1.0f, 0.0f, 0.0f }, 90.0f, { 150, 220, 255, ringAlpha });
        // Secondary electric arc ring
        DrawCircle3D(impactPos, ringRadius * 0.45f, (Vector3){ 1.0f, 0.0f, 0.0f }, 90.0f, { 255, 255, 255, ringAlpha });
        // Radiant ground fireball
        DrawSphere(impactPos, blastProg * 2.4f, { 240, 250, 255, (unsigned char)(blastProg * 225.0f) });
    }
}
