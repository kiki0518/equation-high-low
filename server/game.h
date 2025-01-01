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
    char name[40], op[4], expression[2][BUFFER_SIZE];
    struct Card card[4];
};

extern struct Card deck_num[44];
extern struct Player pc[MAX_PLAYERS];
extern char deck_op[5], sendline[BUFFER_SIZE], recvline[BUFFER_SIZE], buffer[BUFFER_SIZE];
extern int random_array[44], clientFd[MAX_PLAYERS], main_pot[2];
extern int deal_index, player_count;

void Initialize_random_array();
void Initialize_card();
void deal_cards(int player_index);
void handle_betting_phase();
void broadcast_message(const char *message);
double evaluate_expression(const char *expression);
void determine_winners();
void Initialize_player();
void input_player_combination();
void start_game();

#endif 
