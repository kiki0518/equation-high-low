#include "game.h"

void handle_betting_phase() {
    int max_bet[2] = {0, 0};
    for (int i = 0; i < player_count; i++) {
        if (pc[i].stat == SPEC) continue;

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
                if(!pc[i].bet[j] || (pc[i].bet[j] < max_bet[j] && pc[i].bet[j] != pc[i].chips)) {
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

void determine_winners() {
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
            pc[winner[j]].chips += main_pot[j] + pc[winner[j]].bet[j];
            snprintf(buffer, sizeof(buffer), "Player %d wins the %s bet with a value of %.2f.\n", pc[winner[j]].id, j == 0 ? "high" : "low", best_val[j]);
            broadcast_message(buffer);
        }
    }
}

void input_player_combination() {
    fd_set read_fds;
    struct timeval timeout;
    int max_fd = 0;

    for (int i = 0; i < player_count; i++) {
        if (pc[i].stat == FOLD) continue;

        snprintf(buffer, sizeof(buffer), 
        "Player %d, enter your expressions. Enter \"H:\" before the high expression and \"L:\" before the low expression.\nExample: H:R4-5+2*3\n\nOperator priority:\n1. Root (R)\n2. Multiplication (*) and Division (/)\n3. Addition (+) and Subtraction (-)\n",
                 pc[i].id);
        write(clientFd[i], buffer, strlen(buffer));

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
            if(strstr(buffer, "H:") == NULL || strstr(buffer, "L:") == NULL) {
                snprintf(buffer, sizeof(buffer), "Invalid expressions. Please include both high and low expressions.\n");
                write(clientFd[i], buffer, strlen(buffer));
                i--;
                continue;
            }                
            
            if(buffer[0] == 'H') {
                sscanf(buffer, "H:%s", pc[i].expression[0]);
            } else {
                sscanf(buffer, "L:%s", pc[i].expression[1]);
            }

            // Check if the expressions contain player's hand symbols
            /*bool valid[2] = {true, true};
            for (int j = 0; j < 4; j++) {
                for(int k = 0; k < 2; k++) {
                    if (strstr(pc[i].expression[k], pc[i].card[j].name) == NULL) {
                        valid[k] = false;
                        break;
                    }
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
            for(int j = 0; j < 2; j++) {
                for(int k = 0; k < 4; k++) {
                    if(pc[i].expression[j][k] == '/' && pc[i].expression[j][k + 1] == '0') {
                        snprintf(buffer, sizeof(buffer), "Invalid expression. Cannot take division by zero.\n");
                        write(clientFd[i], buffer, strlen(buffer));
                        i--;
                        break;
                    }
            }*/
            
        } else if (activity == 0) {
            printf("Player %d timed out.\n", pc[i].id);
            pc[i].stat = FOLD;
            snprintf(buffer, sizeof(buffer), "You have been folded due to timeout.\n");
            write(clientFd[i], buffer, strlen(buffer));
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