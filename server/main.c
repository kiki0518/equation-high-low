#include "game.h"

int main(int argc, char *argv[]) {    
    player_count = atoi(argv[1]);
    for (int i = 0; i < player_count; i++) {
        clientFd[i] = atoi(argv[i + 2]);
    }
    start_game();
    return 0;
}