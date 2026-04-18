#include "game.h"

int main() {
    Game game;
    if (!game.init(1280, 720, "City Drift — Racing Game"))
        return -1;
    game.run();
    game.cleanup();
    return 0;
}
