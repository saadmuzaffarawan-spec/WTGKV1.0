// What The Ground Keeps - entry point.
#include "game/game.h"

int main(int argc, char** argv) {
    Game& g = G();
    if (!g.Init(argc, argv)) return 1;
    g.Run();
    g.Shutdown();
    return 0;
}
