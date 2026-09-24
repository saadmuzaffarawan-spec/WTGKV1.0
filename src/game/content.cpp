// Registers every prefab family.
#include "prefab_util.h"

void RegisterAllPrefabs() {
    static bool done = false;
    if (done) return;
    done = true;
    RegisterEnvPrefabs();
    RegisterStationPrefabs();
    RegisterDoorPrefabs();
    RegisterStorePrefabs();
    RegisterCrashPrefabs();
    RegisterCollegePrefabs();
    RegisterFieldPrefabs();
    RegisterUnderPrefabs();
}
