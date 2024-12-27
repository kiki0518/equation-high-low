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
    char op[4];
    int chips;
    char expression_high[BUFFER_SIZE]; // For "high" bet expression
    char expression_low[BUFFER_SIZE];  // For "low" bet expression
    int bet_high;    // Bet amount for "high"
    int bet_low;     // Bet amount for "low"
    bool folded;     // Indicates if the player has folded
};

struct Card deck_num[44];
struct Player pc[MAX_PLAYERS];
char deck_op[5] = {'+', '-', '*', '/', 'R'};
int random_array[44];
int clientFd[MAX_PLAYERS];
int deal_index = 0;
int player_count = 0;

void Initialize_random_array();
void Initialize_card();
void deal_cards(int player_index);
void handle_betting_phase(int *main_pot);
void broadcast_message(const char *message, int num_players);
double evaluate_expression(const char *expression);
void determine_winners(int *main_pot);
void Initialize_player();
void start_game();

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

void deal_cards(int player_index) {
    // Deal 4 cards and 3 operators to each player
    for (int i = 0; i < 4; i++) {
        int n = random_array[deal_index++];
        pc[player_index].card[i] = deck_num[n];
    }
    for (int i = 0; i < 3; i++) {
        pc[player_index].op[i] = deck_op[rand() % 4];
    }
    if (rand() % 5 == 0) {  // 20% chance to get an root operator
        pc[player_index].op[3] = deck_op[4];
    } 
}

void handle_betting_phase(int *main_pot) {
    int max_bet = 0;
    for (int i = 0; i < player_count; i++) {
        if (pc[i].folded) continue;

        char buffer[BUFFER_SIZE];
        snprintf(buffer, sizeof(buffer), "Player %d, it's your turn to bet. Max bet: %d, your chips: %d.\n",
                 pc[i].id, max_bet, pc[i].chips);
        write(clientFd[i], buffer, strlen(buffer));

        read(clientFd[i], buffer, sizeof(buffer));

        if (strncmp(buffer, "fold", 4) == 0) {
            pc[i].folded = true;
            snprintf(buffer, sizeof(buffer), "Player %d folded. You are out of this round.\n", pc[i].id);
            write(clientFd[i], buffer, strlen(buffer));
            continue;
        }

        int bet_high = 0, bet_low = 0;
        sscanf(buffer, "%d %d", &bet_high, &bet_low);

        if (bet_high < max_bet && bet_high != pc[i].chips) {
            snprintf(buffer, sizeof(buffer), "Invalid high bet! You must bet more than %d or go All-In.\n", max_bet);
            write(clientFd[i], buffer, strlen(buffer));
            i--;
            continue;
        }

        if (bet_high > pc[i].chips) bet_high = pc[i].chips;

        pc[i].chips -= bet_high;
        pc[i].bet_high = bet_high;
        max_bet = bet_high > max_bet ? bet_high : max_bet;
        *main_pot += bet_high;

        if (bet_low > pc[i].chips) bet_low = pc[i].chips;

        pc[i].chips -= bet_low;
        pc[i].bet_low = bet_low;
        *main_pot += bet_low;

        snprintf(buffer, sizeof(buffer), "You bet %d chips on high and %d chips on low. Remaining chips: %d.\n",
                 bet_high, bet_low, pc[i].chips);
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
            for (int j = i; j < op_index - 1; j++) {
                operators[j] = operators[j + 1];
            }
            op_index--;
            break;
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
                    // TODO: Handle division by zero, maybe need to tell the player to re-enter the expression
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

void determine_winners(int *main_pot) {
    double best_high = -1e9, best_low = 1e9;
    int winner_high = -1, winner_low = -1;

    for (int i = 0; i < player_count; i++) {
        if (pc[i].folded) continue;

        double result_high = evaluate_expression(pc[i].expression_high);
        double result_low = evaluate_expression(pc[i].expression_low);


        if (pc[i].bet_high > 0 && fabs(result_high - 20) < fabs(best_high - 20)) {
            best_high = result_high;
            winner_high = i;
        }

        if (pc[i].bet_low > 0 && fabs(result_low - 1) < fabs(best_low - 1)) {
            best_low = result_low;
            winner_low = i;
        }
    }

    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "Winner (High): Player %d\nWinner (Low): Player %d\n",
             winner_high + 1, winner_low + 1);
    broadcast_message(buffer, player_count);

    if (winner_high != -1) {
        pc[winner_high].chips += pc[winner_high].bet_high * 2; // High bet payout
    }

    if (winner_low != -1) {
        pc[winner_low].chips += pc[winner_low].bet_low * 2; // Low bet payout
    }

    *main_pot = 0; // Reset main pot after payout
}

void Initialize_player() {
    for (int i = 0; i < player_count; i++) {
        pc[i].id = i + 1;
        pc[i].chips = 100; 
        pc[i].folded = false;
        pc[i].bet_high = 0;
        pc[i].bet_low = 0;
        deal_cards(i);
    }
}

void input_player_combination() {
    for (int i = 0; i < player_count; i++) {
        if (pc[i].folded) continue;

        char buffer[BUFFER_SIZE];
        snprintf(buffer, sizeof(buffer), "Player %d, enter your expressions: first for high bet, second for low bet.\nExample: R4-5+2*3 (High Bet)\nExample: 5+R9-3*2 (Low Bet)\n\nOperator priority:\n1. Root (R)\n2. Multiplication (*) and Division (/)\n3. Addition (+) and Subtraction (-)\n",
                 pc[i].id);
        write(clientFd[i], buffer, strlen(buffer));

        read(clientFd[i], buffer, sizeof(buffer));
        sscanf(buffer, "%s %s", pc[i].expression_high, pc[i].expression_low);
    }
}

void start_game() {
    int main_pot = 0;
    Initialize_card();  // Initialize deck of cards with index
    Initialize_random_array();  // Initialize random array for card shuffling
    Initialize_player();
    handle_betting_phase(&main_pot);
    input_player_combination();
    determine_winners(&main_pot);
}

int main(int argc, char *argv[]) {
    for (int i = 0; i < argc - 1; i++) {
        clientFd[i] = atoi(argv[i + 1]);
    }
    player_count = argc - 1;
    start_game();
    return 0;
}