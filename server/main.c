#include "game.h"

int main(int argc, char *argv[]) {    
    clicnt = argc - 2;
    playercnt = atoi(argv[1]);
    for (int i = 0; i < clicnt; i++) {
        clientFd[i] = atoi(argv[i + 2]);
        // snprintf(sendline, sizeof(sendline), "You are Player %d.\n", i + 1);
        // write(clientFd[i], sendline, strlen(sendline));
    }
    start_game();
    return 0;
}