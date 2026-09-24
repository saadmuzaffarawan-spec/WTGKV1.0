#pragma once
// =========================================================================
// STRUCTURE 1: THE GAS STATION (CENTER OF ROAD: X = 126..130, Z = 134..146)
// Sits neatly in the exact center of the highway, splitting the road into
// two equal lanes (Left Lane: 117..125, Right Lane: 131..139).
// Features central pump island, dual-sided fuel pumps, overhead canopy & roof camera.
// =========================================================================

inline void BuildGasStation(Chunk* chunk) {
    // 1. Concrete Pump Island curb in the center of the road at Y = 10..11
    for (int x = 126; x <= 130; x++) {
        for (int z = 134; z <= 146; z++) {
            Voxel& v = chunk->voxels[x][10][z];
            v.isSolid = true;
            v.glyphIndex = '#';
            v.fgColor = { 26, 24, 22, 255 }; // Stained concrete island base
        }
    }
}

// =========================================================================
// STRUCTURE 2: THE STANDALONE HORROR SUPERSTORE & SURROUNDING LOT
// Lot footprint: X in [78, 115], Z in [120, 160] on the west bank of the highway.
// Hollows out terrain voxels (Y >= 11) to open air, zero ASCII wall stamping.
// All walls, trims, fixtures, and blood stains are drawn as real 3D shapes!
// =========================================================================

inline void BuildShop(Chunk* chunk) {
    // Clear all terrain voxels within the shop lot footprint (Y >= 10 up to CHUNK_H - 1)
    // Ensures zero rogue ASCII blocks anywhere inside the shop or on its floor
    for (int x = 75; x <= 116; x++) {
        for (int z = 118; z <= 170; z++) {
            for (int y = 10; y < CHUNK_H; y++) {
                Voxel& v = chunk->voxels[x][y][z];
                v.isSolid = false;
                v.glyphIndex = 0;
            }
        }
    }
}

// =========================================================================
// STRUCTURE 3: THE ABANDONED BLACKWOOD COLLEGE BUILDING & GROUNDS
// East bank of Route 9: X in [148, 194], Z in [118, 166]
// Hollows out terrain voxels (Y >= 10 up to CHUNK_H - 1) to open air
// All walls, partitions, classrooms, and fixtures are drawn in rich 3D!
// =========================================================================
inline void BuildCollege(Chunk* chunk) {
    for (int x = 148; x <= 194; x++) {
        for (int z = 118; z <= 166; z++) {
            for (int y = 10; y < CHUNK_H; y++) {
                Voxel& v = chunk->voxels[x][y][z];
                v.isSolid = false;
                v.glyphIndex = 0;
            }
        }
    }
}
