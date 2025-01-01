#include "game.h"

struct Card deck_num[44];
struct Player pc[MAX_PLAYERS];
char buffer[BUFFER_SIZE], deck_op[5] = {'+', '-', '*', '/', 'R'};
int random_array[44], clientFd[MAX_PLAYERS], main_pot[2];
int deal_index, player_count;

void Initialize_random_array() {
    for (int i = 0; i < 44; i++)    random_array[i] = i;

    srand(time(NULL));
    for (int i = 43; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = random_array[i];
        random_array[i] = random_array[j];
        random_array[j] = temp;
    }

    printf("Initialized random array.\n");
}

void Initialize_card() {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j <= 10; j++) {
            int index = i * 11 + j;
            snprintf(deck_num[index].suit, sizeof(deck_num[index].suit), "%s",
                     (i == 0) ? "Gold" : (i == 1) ? "Silver" : (i == 2) ? "Bronze" : "Iron");
            deck_num[index].number = j;
        }
    }
    printf("Initialized deck of cards.\n");
}

void Initialize_player() {
    for (int i = 0; i < player_count; i++) {
        pc[i].id = i + 1;
        pc[i].chips = 100;
        pc[i].bet[0] = pc[i].bet[1] = 0;
        pc[i].stat = 0;
        pc[i].fd = clientFd[i];
        memset(pc[i].name, 0, sizeof(pc[i].name));
        memset(pc[i].expression, 0, sizeof(pc[i].expression));
        deal_cards(i);
        printf("Player %d initialized.\n", pc[i].id);
    }
    printf("Initialized all players.\n");
}

void deal_cards(int player_index) {
    snprintf(sendline, sizeof(sendline), "You have been dealt the following cards:\n");

    // Deal 4 cards and 3 operators to each player
    for (int i = 0; i < 4; i++) {
        int n = random_array[deal_index++];
        pc[player_index].card[i] = deck_num[n];
        snprintf(sendline + strlen(sendline), sizeof(sendline) - strlen(sendline), "%s %d\n", pc[player_index].card[i].suit, pc[player_index].card[i].number);
    }

    snprintf(sendline + strlen(sendline), sizeof(sendline) - strlen(sendline), "You have been dealt the following operators:\n");
    int n = rand() % 4;
    int idx = 0;
    for(int i = 0; i < 4; i++) {
        if(n != i)  {
            pc[player_index].op[idx++] = deck_op[i];
            snprintf(sendline + strlen(sendline), sizeof(sendline) - strlen(sendline), "%c\n", pc[player_index].op[idx - 1]);
        }
    }

    if (rand() % 5 == 0) {  // 20% chance to get a root operator
        pc[player_index].op[3] = deck_op[4];
        snprintf(sendline + strlen(sendline), sizeof(sendline) - strlen(sendline), "%c\n", pc[player_index].op[3]);
    } else {
        pc[player_index].op[3] = 0;   // 0 means no operator
    }

    if(write(pc[player_index].fd, sendline, strlen(sendline)) < 0) {
        printf("Error sending message to client %d.\n", pc[player_index].id);
        return;
    }
    printf("Send: %s\n", sendline);
}