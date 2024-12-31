/*
Header file for game.cpp:
Defines functions for card generation, equation calculation, and game state management.
Includes shared game-related data structures.

*/
#ifndef GAME_LOGIC_HPP
#define GAME_LOGIC_HPP

#include <vector>
#include <string>

// 玩家數據結構
struct Player {
    int id;                     // 玩家 ID
    std::string name;           // 玩家名稱
    int number_cards[2];        // 數字卡片
    char operator_card;         // 操作符號
    int bet_choice;             // 玩家下注選擇 (1: high, 0: low)
};

// 分發卡片
void distribute_cards(std::vector<Player>& players);

// 計算玩家結果
int calculate_result(const Player& player);

// 判斷勝負
std::string determine_winner(const std::vector<Player>& players);

#endif // GAME_LOGIC_HPP
