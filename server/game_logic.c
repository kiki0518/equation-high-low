#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include <fcntl.h>

#define MAX_PLAYERS 3
#define BUFFER_SIZE 1024

struct Card {
    char name[40];
    int number;
};

struct Player {
    int id;
    char name[40];
    struct Card card[4];
    char op[3];
    bool has_root;
    int chips;
    char expression[BUFFER_SIZE];
    int bet;
    char bet_type;  // 'B' for high, 'S' for low
};

struct Card deck_num[44];
char deck_op[5] = {'+', '-', '*', '/', 'R'};
int random_array[44];
int clientFd[MAX_PLAYERS];
int deal_index = 0;

void Initialize_random_array();
void Initialize_card();
void deal_cards(struct Player *player);
void handle_betting_phase(struct Player *players, int num_players, int *main_pot);
void broadcast_message(const char *message, int num_players);
double evaluate_expression(const char *expression);
void determine_winners(struct Player *players, int num_players, int *main_pot);

void Initialize_random_array() {
    for (int i = 0; i < 44; i++) {
        random_array[i] = i;
    }
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
                     (i == 0) ? "Hearts" : (i == 1) ? "Diamonds" : (i == 2) ? "Clubs" : "Spades", j);
            deck_num[index].number = j;
        }
    }
}

void deal_cards(struct Player *player) {
    for (int i = 0; i < 4; i++) {
        int n = random_array[deal_index++];
        player->card[i] = deck_num[n];
    }
    for (int i = 0; i < 3; i++) {
        player->op[i] = deck_op[rand() % 4];
    }
    player->has_root = (rand() % 5 == 0);
}

void handle_betting_phase(struct Player *players, int num_players, int *main_pot) {
    int max_bet = 0;
    for (int i = 0; i < num_players; i++) {
        char buffer[BUFFER_SIZE];
        snprintf(buffer, sizeof(buffer), "Player %d, it's your turn to bet. Max bet: %d, your chips: %d.\n",
                 players[i].id, max_bet, players[i].chips);
        write(clientFd[i], buffer, strlen(buffer));

        int bet = 0;
        read(clientFd[i], buffer, sizeof(buffer));
        sscanf(buffer, "%d %c", &bet, &players[i].bet_type);

        if (bet < max_bet && bet != players[i].chips) {
            snprintf(buffer, sizeof(buffer), "Your bet must be at least %d or you must go all-in.\n", max_bet);
            write(clientFd[i], buffer, strlen(buffer));
            i--;
            continue;
        }

        if (bet > players[i].chips) bet = players[i].chips;

        players[i].chips -= bet;
        players[i].bet = bet;
        max_bet = bet > max_bet ? bet : max_bet;
        *main_pot += bet;

        snprintf(buffer, sizeof(buffer), "You bet %d chips on %c. Remaining chips: %d.\n",
                 bet, players[i].bet_type, players[i].chips);
        write(clientFd[i], buffer, strlen(buffer));
    }
}

void broadcast_message(const char *message, int num_players) {
    fd_set writefds;
    FD_ZERO(&writefds);

    int max_fd = 0;
    for (int i = 0; i < num_players; i++) {
        FD_SET(clientFd[i], &writefds);
        if (clientFd[i] > max_fd) max_fd = clientFd[i];
    }

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    int activity = select(max_fd + 1, NULL, &writefds, NULL, &timeout);
    if (activity < 0) {
        perror("Select error");
        return;
    }

    for (int i = 0; i < num_players; i++) {
        if (FD_ISSET(clientFd[i], &writefds)) {
            if (write(clientFd[i], message, strlen(message)) < 0) {
                perror("Error sending message to client");
            }
        }
    }
}

double evaluate_expression(const char *expression) {
    double numbers[4];
    char operators[3];
    int num_index = 0, op_index = 0;

    for (int i = 0; expression[i] != '\0'; i++) {
        if (isdigit(expression[i])) {
            if (expression[i] == '1' && expression[i + 1] == '0') {
                numbers[num_index++] = 10;
                i++;
            } else {
                numbers[num_index++] = expression[i] - '0';
            }
        } else if (strchr("+-*/R", expression[i])) {
            operators[op_index++] = expression[i];
        }
    }

    for (int i = 0; i < op_index; i++) {
        if (operators[i] == 'R') {
            numbers[i] = sqrt(numbers[i]);
            for (int j = i; j < num_index - 1; j++) {
                numbers[j] = numbers[j + 1];
            }
            num_index--;
            for (int j = i; j < op_index - 1; j++) {
                operators[j] = operators[j + 1];
            }
            op_index--;
            i--;
        }
    }

    for (int i = 0; i < op_index; i++) {
        if (operators[i] == '*' || operators[i] == '/') {
            switch (operators[i]) {
                case '*':
                    numbers[i] = numbers[i] * numbers[i + 1];
                    break;
                case '/':
                    numbers[i] = numbers[i] / (numbers[i + 1] != 0 ? numbers[i + 1] : 1);
                    break;
            }
            for (int j = i + 1; j < num_index - 1; j++) {
                numbers[j] = numbers[j + 1];
            }
            num_index--;
            for (int j = i; j < op_index - 1; j++) {
                operators[j] = operators[j + 1];
            }
            op_index--;
            i--;
        }
    }

    double result = numbers[0];
    for (int i = 0; i < op_index; i++) {
        switch (operators[i]) {
            case '+':
                result += numbers[i + 1];
                break;
            case '-':
                result -= numbers[i + 1];
                break;
        }
    }

    return result;
}

void determine_winners(struct Player *players, int num_players, int *main_pot) {
    double best_high = -1e9, best_low = 1e9;
    int winner_high = -1, winner_low = -1;

    for (int i = 0; i < num_players; i++) {
        double result = evaluate_expression(players[i].expression);
        if (players[i].bet_type == 'B' && fabs(result - 20) < fabs(best_high - 20)) {
            best_high = result;
            winner_high = i;
        }
        if (players[i].bet_type == 'S' && fabs(result - 1) < fabs(best_low - 1)) {
            best_low = result;
            winner_low = i;
        }
    }

    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "Winner (High): Player %d\nWinner (Low): Player %d\n",
             winner_high + 1, winner_low + 1);
    broadcast_message(buffer, num_players);

    if (winner_high != -1) players[winner_high].chips += *main_pot / 2;
    if (winner_low != -1) players[winner_low].chips += *main_pot / 2;
}

void start_game(int players) {
    Initialize_card();
    Initialize_random_array();

    struct Player pc[players];
    for (int i = 0; i < players; i++) {
        pc[i].id = i + 1;
        pc[i].chips = 100; 
        deal_cards(&pc[i]);
    }

    int main_pot = 0;

    handle_betting_phase(pc, players, &main_pot);

    for (int i = 0; i < players; i++) {
        char buffer[BUFFER_SIZE];
        snprintf(buffer, sizeof(buffer), "Player %d, enter your expression (e.g., 1*2+3/4(B or S)):\n", pc[i].id);
        write(clientFd[i], buffer, strlen(buffer));
        read(clientFd[i], pc[i].expression, sizeof(pc[i].expression));
    }

    // Determine winners
    determine_winners(pc, players, &main_pot);
}

int main(int argc, char *argv[]) {
    for (int i = 0; i < argc - 1; i++) {
        clientFd[i] = atoi(argv[i + 1]);
    }
    start_game(MAX_PLAYERS);
    return 0;
}
