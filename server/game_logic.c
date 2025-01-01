#include "game.h"

void betting() {
    printf("========Betting phase========\n");
    broadcast("======Betting phase======\n");
    int max_bet[2] = {0, 0};
    int available_players = 0;
    for (int i = 0; i < clicnt; i++) {
        if (pc[i].stat == SPEC) continue;        
        snprintf(sendline, sizeof(sendline), "Player %d, it's your turn to bet. High bet: %d   Low bet: %d\nPlease enter your bet number (0: Fold, 1: High, 2: Low, 3:Both): ", pc[i].id, max_bet[HIGH-1], max_bet[LOW-1]);
        if(write(clientFd[i], sendline, strlen(sendline)) < 0) {
            printf("Error writing message to client #%d.\n", pc[i].id);
            snprintf(sendline, sizeof(sendline), "Player #%d disconnected.\n", pc[i].id);
            broadcast(sendline);
            return;
        }
        printf("Send: %s\n", sendline);

        if(n = read(clientFd[i], recvline, sizeof(recvline)) < 0) {
            printf("Error reading message from client #%d.\n", pc[i].id);
            snprintf(sendline, sizeof(sendline), "Player #%d disconnected.\n", pc[i].id);
            broadcast(sendline);
            return;
        }

        recvline[n-1] = '\0';
        printf("Recv: %s\n", recvline);
        sscanf(recvline, "%d\n", &pc[i].stat);

        if (pc[i].stat == FOLD) {
            snprintf(sendline, sizeof(sendline), "You have folded.\n");
            if(write(clientFd[i], sendline, strlen(sendline)) < 0) {
                printf("Error writing message to client #%d.\n", pc[i].id);
                snprintf(sendline, sizeof(sendline), "Player #%d disconnected.\n", pc[i].id);
                broadcast(sendline);
                return;
            }
            printf("Send: %s\n", sendline);
            continue;
        }

        for(int j = 0; j < 2; j++) {
            if(pc[i].stat & (1 << j)) {
                snprintf(sendline, sizeof(sendline), "Enter your bet for %s: ", j == 0 ? "high" : "low");
                if(write(clientFd[i], sendline, strlen(sendline)) < 0) {
                    printf("Error writing message to client #%d.\n", pc[i].id);
                    snprintf(sendline, sizeof(sendline), "Player #%d disconnected.\n", pc[i].id);
                    broadcast(sendline);
                    return;
                }
                printf("Send: %s\n", sendline);
                
                if(n = read(clientFd[i], recvline, sizeof(recvline)) < 0) {
                    printf("Error reading message from client #%d.\n", pc[i].id);
                    snprintf(sendline, sizeof(sendline), "Player #%d disconnected.\n", pc[i].id);
                    broadcast(sendline);
                    return;
                }
                recvline[n-1] = '\0';
                printf("Recv: %s\n", recvline);
                sscanf(recvline, "%d", &pc[i].bet[j]);
                
                if(!pc[i].bet[j] || (pc[i].bet[j] < max_bet[j] && pc[i].bet[j] != pc[i].chips)) {
                    snprintf(sendline, sizeof(sendline), "Invalid bet! You must bet more than or equal to %d or go All-In.\n", max_bet[j]);
                    if(write(clientFd[i], sendline, strlen(sendline)) < 0) {
                        printf("Error writing message to client #%d.\n", pc[i].id);
                        snprintf(sendline, sizeof(sendline), "Player #%d disconnected.\n", pc[i].id);
                        broadcast(sendline);
                        return;
                    }
                    printf("Send: %s\n", sendline);
                    i--;
                    break;
                } 
                
                max_bet[j] = max_bet[j] < pc[i].bet[j] ? pc[i].bet[j] : max_bet[j];
                pc[i].chips -= pc[i].bet[j];
                main_pot[j] += pc[i].bet[j];

                snprintf(sendline, sizeof(sendline), "You bet %d chips on %s. Remaining chips: %d.\n", pc[i].bet[j], j == 0 ? "high" : "low", pc[i].chips);
                if(write(clientFd[i], sendline, strlen(sendline)) < 0) {
                    printf("Error writing message to client #%d.\n", pc[i].id);
                    snprintf(sendline, sizeof(sendline), "Player #%d disconnected.\n", pc[i].id);
                    broadcast(sendline);
                    return;
                }
                printf("Send: %s\n", sendline);
                available_players++;
            }
        }
    }
    if(available_players <= 1) {
        broadcast("Not enough players to continue the game. Game ended.\n");
        printf("Not enough players to continue the game. Game ended.\n");
        end_game = 0;
    }
    broadcast("======Betting phase completed======\n");
    printf("======Betting phase completed======\n");
}

void broadcast(const char *message){
    fd_set writefds;
    FD_ZERO(&writefds);

    int max_fd = 0;
    for (int i = 0; i < clicnt; i++) {
        FD_SET(clientFd[i], &writefds);
        if (clientFd[i] > max_fd) max_fd = clientFd[i];
    }

    struct timeval timeout;
    timeout.tv_sec = TIMEOUT;
    timeout.tv_usec = 0;

    int activity = select(max_fd + 1, NULL, &writefds, NULL, &timeout);
    if (activity < 0) {
        printf("Select error in broadcast_message\n");
        return;
    }

    for (int i = 0; i < clicnt; i++) {
        if (FD_ISSET(clientFd[i], &writefds)) {
            if (write(clientFd[i], message, strlen(message)) < 0) {
                printf("Client %d disconnected\n", i + 1);
                return;
            }
        }
    }
}

double evaluate(int player_index, int exp_index) {
    double num[4];
    char op[3];
    int num_index = 0, op_index = 0;
    char *exp = pc[player_index].exp[exp_index];

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
            for (int j = i; j < num_index - 1; j++) {
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

void findWinner() {
    printf("========Determining winners========\n");
    double best_val[2] = {1e9, 1e9};
    int winner[2] = {-1, -1};

    for (int i = 0; i < clicnt; i++) {
        if (pc[i].stat <= FOLD) continue;
        for(int j = 0; j < 2; j++) {
            if (pc[i].stat & (1 << j)) {
                double result = evaluate(i, j);
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
            broadcast(buffer);
        }
    }

    printf("======Winners determined======\n");
}

void collectAns() {
    printf("========Input player combinations========\n");
    fd_set read_fds;
    struct timeval timeout;
    int max_fd = 0;

    for (int i = 0; i < clicnt; i++) {
        if (pc[i].stat <= FOLD) continue;

        snprintf(buffer, sizeof(buffer), 
        "Player %d, enter your expressions. Enter \"H:\" before the high expression and \"L:\" before the low expression.\nExample: H:R4-5+2*3\n\nOperator priority:\n1. Root (R)\n2. Multiplication (*) and Division (/)\n3. Addition (+) and Subtraction (-)\n",
                 pc[i].id);
        write(clientFd[i], buffer, strlen(buffer));
        printf("Sent: %s\n", buffer);

        int flags = fcntl(clientFd[i], F_GETFL, 0);
        fcntl(clientFd[i], F_SETFL, flags | O_NONBLOCK);

        FD_ZERO(&read_fds);
        FD_SET(clientFd[i], &read_fds);
        if (clientFd[i] > max_fd) max_fd = clientFd[i];

        timeout.tv_sec = TIMEOUT;
        timeout.tv_usec = 0;

        int activity = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);

        if (activity > 0 && FD_ISSET(clientFd[i], &read_fds)) {
            read(clientFd[i], buffer, sizeof(buffer));
            if(strstr(buffer, "H:") == NULL || strstr(buffer, "L:") == NULL) {
                snprintf(buffer, sizeof(buffer), "Invalid expressions. Please include both high and low expressions.\n");
                write(clientFd[i], buffer, strlen(buffer));
                i--;
                continue;
            }                
            
            if(buffer[0] == 'H') {
                sscanf(buffer, "H:%s", pc[i].exp[0]);
            } else {
                sscanf(buffer, "L:%s", pc[i].exp[1]);
            }
            // Check if the expression is valid
        } else if (activity == 0) {
            printf("Player %d timed out.\n", pc[i].id);
            pc[i].stat = FOLD;
            snprintf(buffer, sizeof(buffer), "You have been folded due to timeout.\n");
            write(clientFd[i], buffer, strlen(buffer));
        }
    }
    printf("======Player combinations inputted======\n");
}

void start_game() {
    main_pot[0] = main_pot[1] = 0;
    initCard();  
    initArr();  
    initClient();
    while(end_game-- > 0) {
        betting();
        if(end_game <= 0) break;
        collectAns();
        findWinner();
    }
}