# Hi-Lo Equation Game

This is a multiplayer Hi-Lo Equation game where players bet on the outcome of mathematical expressions. Each player can place two bets: one for a "high" value and one for a "low" value. The game supports up to 3 players.

## Features

- Players can place two bets: one for a high value and one for a low value.
- Supports up to 3 players.
- Players can use basic arithmetic operators and the square root operator in their expressions.
- The game evaluates the expressions and determines the winners based on the closest values to predefined targets.

## Game Rules

1. Each player is dealt 4 cards and 3 operators.
2. Players place their bets for high and low values.
3. Players submit their mathematical expressions for high and low bets.
4. The game evaluates the expressions and determines the winners.
5. The winners are the players whose expressions are closest to the target values.

## Expression Operators

- Addition (`+`)
- Subtraction (`-`)
- Multiplication (`*`)
- Division (`/`)
- Square Root (`R`)

## Operator Priority

1. Square Root (`R`)
2. Multiplication (`*`) and Division (`/`)
3. Addition (`+`) and Subtraction (`-`)

## Example Expressions

- High Bet: `R4-5+2*3`
- Low Bet: `5+R9-3*2`

## Getting Started

### Prerequisites

- GCC (GNU Compiler Collection)
- Make

### Building the Project

1. Clone the repository:
    ```sh
    git clone https://github.com/your-repo/hi-lo-equation-game.git
    cd hi-lo-equation-game
    ```

2. Build the project:
    ```sh
    make
    ```

### Running the Game

1. Start the server:
    ```sh
    ./server
    ```

2. Connect players to the server:
    ```sh
    ./client <server_ip> <server_port>
    ```

### File Structure

- [game_logic.c](http://_vscodecontentref_/2): Contains the game logic, including card dealing, betting, and expression evaluation.
- `server.c`: Contains the server code to handle player connections and game flow.

### Functions in [game_logic.c](http://_vscodecontentref_/3)

- `Initialize_random_array()`: Initializes the random array for card shuffling.
- `Initialize_card()`: Initializes the deck of cards.
- `deal_cards(int player_index)`: Deals cards and operators to a player.
- `handle_betting_phase(int *main_pot)`: Handles the betting phase for all players.
- `broadcast_message(const char *message, int num_players)`: Broadcasts a message to all players.
- `evaluate_expression(const char *expression)`: Evaluates a mathematical expression.
- `determine_winners(int *main_pot)`: Determines the winners based on the evaluated expressions.
- `Initialize_player()`: Initializes the players.
- `start_game()`: Starts the game.

### Functions in `server.c`

- `main(int argc, char *argv[])`: Entry point of the server application. Initializes the server and starts the game.

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- Thanks to all contributors and open-source projects that made this game possible.