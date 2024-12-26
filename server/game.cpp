/*
Implements the core game mechanics:
Distributing cards to players.
Performing calculations based on operators and numbers.
Validating guesses and determining winners.

*/
#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <unistd.h> // for write()

using namespace std;

struct Player {
    int id;
    string name;
    int connfd;
    int chips; // 籌碼數量
};

vector<Player> players;
const int MAX_PLAYERS = 10;

void addPlayer(int id, const string& name, int connfd) {
    if (players.size() >= MAX_PLAYERS) {
        cout << "房間已滿，無法添加玩家" << endl;
        return;
    }
    players.push_back({id, name, connfd, 100}); // 初始籌碼數量為 100
    cout << "玩家 " << name << " 已加入遊戲房間" << endl;
}

void removePlayer(int id) {
    for (auto it = players.begin(); it != players.end(); ++it) {
        if (it->id == id) {
            cout << "玩家 " << it->name << " 離開遊戲房間" << endl;
            players.erase(it);
            return;
        }
    }
    cout << "玩家 ID " << id << " 不在房間內" << endl;
}

void generateExpression(string& expression, int& result) {
    int num1 = rand() % 10 + 1;
    int num2 = rand() % 10 + 1;
    int num3 = rand() % 10 + 1;

    char ops[] = {'+', '-', '*'};
    char op1 = ops[rand() % 3];
    char op2 = ops[rand() % 3];

    stringstream ss;
    ss << num1 << " " << op1 << " " << num2 << " " << op2 << " " << num3;
    expression = ss.str();

    switch (op1) {
        case '+': result = num1 + num2; break;
        case '-': result = num1 - num2; break;
        case '*': result = num1 * num2; break;
    }
    switch (op2) {
        case '+': result += num3; break;
        case '-': result -= num3; break;
        case '*': result *= num3; break;
    }
}

void sendMessage(int connfd, const string& message) {
    write(connfd, message.c_str(), message.size());
}

void letPlay(vector<int> id, vector<string> name, vector<int> connfd) {
    srand(time(nullptr));

    for (size_t i = 0; i < id.size(); ++i) {
        addPlayer(id[i], name[i], connfd[i]);
    }

    while (players.size() > 1) {
        cout << "開始新的一局遊戲" << endl;

        int maxResult = -1;
        int winnerIndex = -1;

        for (size_t i = 0; i < players.size(); ++i) {
            string expression;
            int result;
            generateExpression(expression, result);

            stringstream ss;
            ss << "你的數學式是: " << expression << "，結果是: " << result << "\n";
            sendMessage(players[i].connfd, ss.str());

            cout << "玩家 " << players[i].name << " 的數學式: " << expression << "，結果是 " << result << endl;

            if (result > maxResult) {
                maxResult = result;
                winnerIndex = i;
            }
        }

        for (size_t i = 0; i < players.size(); ++i) {
            if (i == winnerIndex) {
                players[i].chips += 10;
                sendMessage(players[i].connfd, "恭喜！你是本局贏家，獲得 10 籌碼！\n");
            } else {
                players[i].chips -= 10;
                sendMessage(players[i].connfd, "很遺憾，你輸了，本局失去 10 籌碼\n");
            }
        }

        for (auto it = players.begin(); it != players.end(); ) {
            if (it->chips <= 0) {
                sendMessage(it->connfd, "你已經破產，請離開遊戲。\n");
                cout << "玩家 " << it->name << " 被移除" << endl;
                it = players.erase(it);
            } else {
                ++it;
            }
        }
    }

    cout << "遊戲結束，玩家不足兩人" << endl;
    for (const auto& player : players) {
        sendMessage(player.connfd, "遊戲結束，感謝參與！\n");
    }
}

