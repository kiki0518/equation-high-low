# Hi-Lo Equation Game - README

## **Overview**
A multiplayer game using TCP socket programming in C/C++. Players calculate results from random number and operator cards, then bet on whether the outcome is "high" or "low." The server manages connections and game logic, while the client handles user input.

---

## **Setup**

### **1. Clone the Repository**
```bash
git clone https://github.com/your-username/HiLoGame.git
cd HiLoGame
```

### **2. Build the Project**
```bash
make
```

### **3. Run the Server**
```bash
./game
./server
```

### **4. Run a Client**
Open a new terminal and run:
```bash
./client <IP Address>
```

---

## **Directory Structure**
```
HiLoGame/
├── server/             # Server-side logic
│   ├── server.c
│   ├── game_init.c
│   ├── game_logic.c
│   ├── game.h

├── client/             # Client-side logic
│   ├── client.c
│   ├── client_utils.c
├── shared/             # Shared utilities and constants
│   ├── protocol.h
│   ├── common_utils.c
├── Makefile            # Build automation
├── README.md           # Project documentation
```

---

## **How to Play**
1. **Server Setup**: Start the server.
2. **Client Interaction**: Enter your name and bet (high/low).
3. **Game Flow**:
   - Players get random number and operator cards.
   - Calculate the result and place bets.
   - The server announces the winner.

---

## **Contributing**
1. Fork and clone the repository.
2. Create a feature branch:
   ```bash
   git checkout -b feature-name
   ```
3. Commit and push changes:
   ```bash
   git commit -m "Add feature"
   git push origin feature-name
   ```
4. Open a pull request.


Enjoy the **Hi-Lo Equation Game**! 😊
