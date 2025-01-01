#include "game.h"

void handle_betting_phase() {
    int max_bet[2] = {0, 0};
    for (int i = 0; i < player_count; i++) {
        if (pc[i].stat == OBSERVE) continue;

        char buffer[BUFFER_SIZE];
        
        snprintf(buffer, sizeof(buffer), "Player %d, it's your turn to bet.\nHigh bet: %d\nLow bet: %d\nEnter \'0\': fold\n\'1\':bet high\n\'2\':bet low\n\'3\':bet both high and low\n", pc[i].id, max_bet[HIGH-1], max_bet[LOW-1]);
    
        if(write(clientFd[i], buffer, strlen(buffer)) < 0) {
            perror("Error sending message to client");
            return;
        }
        if(read(clientFd[i], buffer, sizeof(buffer)) < 0) {
            perror("Error reading message from client");
            return;
        }
        sscanf(buffer, "%d", &pc[i].stat);

        if (pc[i].stat == FOLD) {
            snprintf(buffer, sizeof(buffer), "You have folded.\n");
            write(clientFd[i], buffer, strlen(buffer));
            continue;
        }

        for(int j = 0; j < 2; j++) {
            if(pc[i].stat & (1 << j)) {
                snprintf(buffer, sizeof(buffer), "Enter your bet for %s: ", j == 0 ? "high" : "low");
                write(clientFd[i], buffer, strlen(buffer));
                read(clientFd[i], buffer, sizeof(buffer));
                sscanf(buffer, "%d", &pc[i].bet[j]);
                if(!pc[i].bet[j] || pc[i].bet[j] < max_bet[j] && pc[i].bet[j] != pc[i].chips) {
                    snprintf(buffer, sizeof(buffer), "Invalid bet! You must bet more than oe equal to %d or go All-In.\n", max_bet[j]);
                    write(clientFd[i], buffer, strlen(buffer));
                    i--;
                    break;
                } else {
                    max_bet[j] = max_bet[j] < pc[i].bet[j] ? pc[i].bet[j] : max_bet[j];
                }
                pc[i].chips -= pc[i].bet[j];
                main_pot[j] += pc[i].bet[j];

                snprintf(buffer, sizeof(buffer), "You bet %d chips on %s. Remaining chips: %d.\n", pc[i].bet[j], j == 0 ? "high" : "low", pc[i].chips);
                write(clientFd[i], buffer, strlen(buffer));
            }
        }
    }
}

void broadcast_message(const char *message){
    fd_set writefds;
    FD_ZERO(&writefds);

    int max_fd = 0;
    for (int i = 0; i < player_count; i++) {
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

    for (int i = 0; i < player_count; i++) {
        if (FD_ISSET(clientFd[i], &writefds)) {
            if (write(clientFd[i], message, strlen(message)) < 0) {
                perror("Error sending message to client");
                return;
            }
        }
    }
}

double evaluate_expression(const char *exp) {
    double num[4];
    char op[3];
    int num_index = 0, op_index = 0;

    for (int i = 0; exp[i] != '\0'; i++) {
        if (isdigit(exp[i])) {
            if (exp[i] == '1' && exp[i + 1] == '0') {
                num[num_index++] = 10;
                i++;
            } else {
                num[num_index++] = exp[i] - '0';
            }
        } else if (strchr("+-*/R", exp[i])) {
            op[op_index++] = exp[i];
        }
    }

    for (int i = 0; i < op_index; i++) {
        if (op[i] == 'R') {
            num[i] = sqrt(num[i]);
            for (int j = i; j < op_index - 1; j++) {
                op[j] = op[j + 1];
            }
            op_index--;
            break;
        }
    }

    for (int i = 0; i < op_index; i++) {
        if (op[i] == '*' || op[i] == '/') {
            switch (op[i]) {
                case '*':
                    num[i] = num[i] * num[i + 1];
                    break;
                case '/':
                    num[i] = num[i] / (num[i + 1] != 0 ? num[i + 1] : 1);
                    break;
            }
            for (int j = i + 1; j < num_index - 1; j++) {
                num[j] = num[j + 1];
            }
            num_index--;
            for (int j = i; j < op_index - 1; j++) {
                op[j] = op[j + 1];
            }
            op_index--;
            i--;
        }
    }

    double result = num[0];
    for (int i = 0; i < op_index; i++) {
        switch (op[i]) {
            case '+':
                result += num[i + 1];
                break;
            case '-':
                result -= num[i + 1];
                break;
        }
    }

    return result;
}

void determine_winners(int *main_pot) {
    double best_val[2] = {1e9, 1e9};
    int winner[2] = {-1, -1};

    for (int i = 0; i < player_count; i++) {
        if (pc[i].stat == FOLD) continue;
        for(int j = 0; j < 2; j++) {
            if (pc[i].stat & (1 << j)) {
                double result = evaluate_expression(pc[i].expression[j]);
                if(fabs(result - (j == 0 ? 20 : 1)) < fabs(best_val[j] - (j == 0 ? 20 : 1))) {
                    best_val[j] = result;
                    winner[j] = i;
                }
            }
        }
    }

    for(int j = 0; j < 2; j++) {
        if(winner[j] != -1) {
            pc[winner[j]].chips += *main_pot;
            snprintf(buffer, sizeof(buffer), "Player %d wins %s bet with expression %s. Result: %.2f\n", pc[winner[j]].id, j == 0 ? "high" : "low", pc[winner[j].expression[j]], best_val[j]);
            broadcast_message(buffer);
        }
    }

    if (winner_high != -1) {
        pc[winner_high].chips += pc[winner_high].bet_high * 2; // High bet payout
    }

    if (winner_low != -1) {
        pc[winner_low].chips += pc[winner_low].bet_low * 2; // Low bet payout
    }

    *main_pot = 0; // Reset main pot after payout
}

void input_player_combination() {
    fd_set read_fds;
    struct timeval timeout;
    int max_fd = 0;

    for (int i = 0; i < player_count; i++) {
        if (pc[i].folded) continue;

        char buffer[BUFFER_SIZE];
        snprintf(buffer, sizeof(buffer), "Player %d, enter your expressions: first for high bet, second for low bet.\nExample: R4-5+2*3 (High Bet)\nExample: 5+R9-3*2 (Low Bet)\n\nOperator priority:\n1. Root (R)\n2. Multiplication (*) and Division (/)\n3. Addition (+) and Subtraction (-)\n",
                 pc[i].id);
        write(clientFd[i], buffer, strlen(buffer));

        // Set the socket to non-blocking mode
        int flags = fcntl(clientFd[i], F_GETFL, 0);
        fcntl(clientFd[i], F_SETFL, flags | O_NONBLOCK);

        FD_ZERO(&read_fds);
        FD_SET(clientFd[i], &read_fds);
        if (clientFd[i] > max_fd) max_fd = clientFd[i];

        timeout.tv_sec = 10;
        timeout.tv_usec = 0;

        int activity = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);

        if (activity > 0 && FD_ISSET(clientFd[i], &read_fds)) {
            bool used_numbers[4] = {false};
            bool used_operators[4] = {false};

            read(clientFd[i], buffer, sizeof(buffer));
            sscanf(buffer, "%s %s", pc[i].expression_high, pc[i].expression_low);

            // Check if the expressions contain player's hand symbols
            bool valid_high = true, valid_low = true;
            for (int j = 0; j < 4; j++) {
                if (strstr(pc[i].expression_high, pc[i].card[j].name) == NULL) {
                    valid_high = false;
                    break;
                }
                if (strstr(pc[i].expression_low, pc[i].card[j].name) == NULL) {
                    valid_low = false;
                    break;
                }
            }

            if (!valid_high || !valid_low) {
                printf("Player %d's expressions do not contain their hand symbols or contain invalid symbols.\n", pc[i].id);
                // Handle invalid input, e.g., prompt again or set default values
                snprintf(buffer, sizeof(buffer), "Invalid expressions. Please include all your hand symbols and do not use any other symbols.\n");
                write(clientFd[i], buffer, strlen(buffer));
                i--; // Retry for the same player
                continue;
            }

            // TODO: Handle division by zero, maybe need to tell the player to re-enter the expression
            for(int j = 0; j < 4; j++) {
                if(pc[i].expression_high[j] == '/' && pc[i].expression_high[j + 1] == '0') {
                    snprintf(buffer, sizeof(buffer), "Invalid expression. Cannot take division by zero.\n");
                    write(clientFd[i], buffer, strlen(buffer));
                    i--;
                    break;
                }
            }

            for(int j = 0; j < 4; j++) {
                if(pc[i].expression_low[j] == '/' && pc[i].expression_low[j + 1] == '0') {
                    snprintf(buffer, sizeof(buffer), "Invalid expression. Cannot take division by zero.\n");
                    write(clientFd[i], buffer, strlen(buffer));
                    i--;
                    break;
                }
            }

        } else if (activity == 0) {
            printf("Player %d timed out.\n", pc[i].id);
            pc[i].folded = true;
            // send message to player to tell them they have been folded

            snprintf(buffer, sizeof(buffer), "You have been folded due to timeout.\n");
            write(clientFd[i], buffer, strlen(buffer));

            // You can set default values or take other actions here

        }
    }
}

void start_game() {
    main_pot[0] = main_pot[1] = 0;
    Initialize_card();  // Initialize deck of cards with index
    Initialize_random_array();  // Initialize random array for card shuffling
    Initialize_player();

    handle_betting_phase();
    input_player_combination();
    determine_winners();
}