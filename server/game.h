#ifndef GAME_H
#define GAME_H

#include <sys/socket.h>
#include <ctype.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include <sys/select.h>
#include <fcntl.h>

#define MAX_PLAYERS 5
#define BUFFER_SIZE 1024
#define TIMEOUT 120
#define HIGH 1
#define LOW 2
#define FOLD 0
#define SPEC -1

struct Card {
    char suit[10];
    int number;
};

struct Player {
    int id, chips, bet[2], stat, fd;  
    char name[40], op[4], exp[2][BUFFER_SIZE];
    struct Card card[4];
};

extern struct Card deck_num[44];
extern struct Player pc[MAX_PLAYERS];
extern char deck_op[5], sendline[BUFFER_SIZE], recvline[BUFFER_SIZE], buffer[BUFFER_SIZE];
extern int arr[44], clientFd[MAX_PLAYERS], main_pot[2];
extern int deal_index, clicnt, playercnt, n, end_game;

void initArr();
void initCard();
void dealCard(int player_index);
void betting();
void broadcast(const char *message);
double evaluate(int player_index, int exp_index);
void findWinner();
void initClient();
void collectAns();
void start_game();

#endif 
