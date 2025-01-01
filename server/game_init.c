#include "game.h"

void Initialize_random_array() {
    for (int i = 0; i < 44; i++)    random_array[i] = i;

    srand(time(NULL));
    for (int i = 43; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = random_array[i];
        random_array[i] = random_array[j];
        random_array[j] = temp;
    }
}

void Initialize_card() {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j <= 10; j++) {
            int index = i * 11 + j;
            snprintf(deck_num[index].name, sizeof(deck_num[index].name), "%s %d",
                     (i == 0) ? "Gold" : (i == 1) ? "Silver" : (i == 2) ? "Bronze" : "Iron", j);
            deck_num[index].number = j;
        }
    }
}

void Initialize_player() {
    for (int i = 0; i < player_count; i++) {
        pc[i].id = i + 1;
        pc[i].chips = 100;
        pc[i].bet[0] = pc[i].bet[1] = 0;
        pc[i].stat = 0;
        pc[i].fd = clientFd[i];
        memset(pc[i].name, 0, sizeof(pc[i].name));
        memset(pc[i].expression_high, 0, sizeof(pc[i].expression_high));
        memset(pc[i].expression_low, 0, sizeof(pc[i].expression_low));
        deal_cards(i);
    }
}

void deal_cards(int player_index) {
    char sendline[BUFFER_SIZE];
    snprintf(sendline, sizeof(sendline), "You have been dealt the following cards:\n");

    // Deal 4 cards and 3 operators to each player
    for (int i = 0; i < 4; i++) {
        int n = random_array[dealIdx++];
        pc[player_index].card[i] = deck_num[n];
        snprintf(sendline + strlen(sendline), sizeof(sendline) - strlen(sendline), "%s %d\n", deck_num[n].name, deck_num[n].number);
    }

    snprintf(sendline + strlen(sendline), sizeof(sendline) - strlen(sendline), "You have been dealt the following operators:\n");
    for (int i = 0; i < 3; i++) {
        pc[player_index].op[i] = deck_op[rand() % 4];
        snprintf(sendline + strlen(sendline), sizeof(sendline) - strlen(sendline), "%c ", pc[player_index].op[i]);
    }

    if (rand() % 5 == 0) {  // 20% chance to get a root operator
        pc[player_index].op[3] = deck_op[4];
        snprintf(sendline + strlen(sendline), sizeof(sendline) - strlen(sendline), "%c\n", pc[player_index].op[3]);
    } else {
        pc[player_index].op[3] = 0;   // 0 means no operator
    }
    
    // Use select to send the dealt cards and operators to the player
    fd_set writefds;
    FD_ZERO(&writefds);
    FD_SET(clientFd[player_index], &writefds);

    struct timeval tv;
    tv.tv_sec = 5;  // Timeout of 5 seconds
    tv.tv_usec = 0;

    int retval = select(clientFd[player_index] + 1, NULL, &writefds, NULL, &tv);
    if (retval == -1) {
        perror("select error");
    } else if (retval == 0) {
        fprintf(stderr, "Timeout occurred! No data sent.\n");
    } else {
        if (FD_ISSET(clientFd[player_index], &writefds)) {
            if (write(clientFd[player_index], sendline, strlen(sendline)) < 0) {
                perror("Error sending message to client");
            }
        }
    }
}