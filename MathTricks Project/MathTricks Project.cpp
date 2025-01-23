/**
*
* Solution to course project # 2
* Introduction to programming course
* Faculty of Mathematics and Informatics of Sofia University
* Winter semester 2024/2025
*
* @author Nora Krasimirova Kirilova
* @idnumber 8MI0600461
* @compiler VS
*
* <file with game>
*
*/

#include <iostream>
#include <iomanip> //for setw
#include <fstream> //for file operations
#include <cstdlib> // for srand
#include <ctime> // for srand
#include <windows.h> //for console colours

using namespace std;

//console COLOR CONSTANTS
const int BG_BLUE_FG_WHITE = 31;
const int BG_GREEN_FG_WHITE = 47;
const int DEFAULT_COLOR = 7;

//defines bright yellow background for the current player's cell:
const int BG_BRIGHT_YELLOW_FG_BLACK = 224;

//constants needed for the board generation
const int MIN_SIZE = 4;
const int MAX_SIZE = 10;
const int COL_INDEX_DISTANCE = 6;
const int SCORE_DISTANCE = 8;
const int SCORE_DISTANCE_ADJUSTMENT = 2;
const int VALUE_OPERATION_DIVISOR = 3;
const int POSSIBLE_OPERATION_SIZE = 4;
const int MIN_DEFAULT_COLS_ROWS = 4;
const int MAX_NUM = 10;
const int MAX_STRING_SIZE = 256;
const long MAX_IGNORE = 10000;

//options in the game
const int NEW_GAME = 1;
const int LOAD_GAME = 2;

//identifications for the two players
const int FIRST_PLAYER = 1;
const int SECOND_PLAYER = 2;

//auto-save filename
const char* AUTO_SAVE_FILE = "autoSave.txt";

void allocateMatrices(int**& board, char**& operations, int**& visited, int rows, int cols) {
    board = new int* [rows];
    operations = new char* [rows];
    visited = new int* [rows];
    for (int i = 0; i < rows; i++) {
        board[i] = new int[cols]();
        operations[i] = new char[cols]();
        visited[i] = new int[cols]();
    }
}

void freeMatrices(const int* const* board, const char* const* operations, const int* const* visited, int rows) {

    for (int i = 0; i < rows; i++) {
        delete[] board[i];  // for values
        delete[] operations[i];  //for operations
        delete[] visited[i];  //saves if the cell is visited and who visited it
    }
    delete[] board;
    delete[] operations;
    delete[] visited;
}

void setColor(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

void clearScreen() {
#ifdef _WIN32
    system("cls"); //command clears the console screen in Windows
#else
    system("clear"); //for non-Windows systems
#endif
}

//safe string copy for status messages
void safeStrCopy(char* dest, const char* src, int destSize) {
    if (destSize <= 0) return;
    int i = 0;
    while (i < destSize - 1 && src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

struct Player {
    int row; //coordinates
    int col; //coordinates
    int score;
    int id; // 1 => Player1, 2 => Player2
};

//defines the last move
struct LastMoveData {
    bool valid; //last move
    int oldRow;
    int oldCol;
    int newRow;
    int newCol;
    int oldScore;
    int newScore;
    char opUsed; //operation in the cell
    int valUsed; //value in the cell
    int playerID;//saves the player's turn
};

//the first turn with default values
LastMoveData lastMoveData = { false,0,0,0,0,0,0,'\0',0,0 };
char statusMsg[MAX_STRING_SIZE] = "";  //string that keeps the status message

// Basic controls
void controlsInfo(int currentPlayerID) {
    cout << "\n=== CONTROLS (current turn: Player " << currentPlayerID
        << ") ===\n"
        << "Movement: 7,8,9,4,6,1,2,3\n"
        << "Help: 'H'\n\n";
}

// Rows
void printGridAndValues(int rows, int cols, int** board, char** operations,
    int** visited, int currentRow, int currentCol) {

    for (int i = 0; i <= rows; i++) {
        // Horizontal separators
        cout << "     ";
        for (int j = 0; j < cols; j++) {
            cout << "+------";
        }
        cout << "+\n";

        if (i < rows) {
            cout << setw(MIN_SIZE) << i << " ";
            for (int c = 0; c < cols; c++) {
                cout << "| ";

                int v = visited[i][c];
                char opChar = (operations[i][c] == '\0') ? ' ' : operations[i][c];
                int val = board[i][c];

                // If this cell is the current player's location,
                // mark the current player's cell in yellow
                if (i == currentRow && c == currentCol) {

                    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), BG_BRIGHT_YELLOW_FG_BLACK);
                    cout << opChar << setw(VALUE_OPERATION_DIVISOR) << val;

                    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), DEFAULT_COLOR);
                }
                else if (v == FIRST_PLAYER) {
                    // Visited by Player1 => BLUE background
                    setColor(BG_BLUE_FG_WHITE);
                    cout << opChar;
                    setColor(BG_BLUE_FG_WHITE);
                    cout << setw(VALUE_OPERATION_DIVISOR) << val;

                    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                        DEFAULT_COLOR);
                }
                else if (v == SECOND_PLAYER) {
                    // Visited by Player2 => GREEN background
                    setColor(BG_GREEN_FG_WHITE);
                    cout << opChar;
                    setColor(BG_GREEN_FG_WHITE);
                    cout << setw(3) << val;

                    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                        DEFAULT_COLOR);
                }
                else {
                    // Unvisited
                    cout << opChar << setw(VALUE_OPERATION_DIVISOR) << val;
                }

                cout << " ";
            }
            cout << "|\n";
        }
    }
}

void printBoard(int rows, int cols, int** board, char** operations,
    int** visited, const Player& p1, const Player& p2, int currentPlayerID) {

    clearScreen();

    // Column headers
    cout << "      ";
    for (int j = 0; j < cols; j++) {
        cout << setw(COL_INDEX_DISTANCE) << j;
    }
    cout << "\n";

    //who is the *current* player's position: currentPlayerID = 1 => p1 is current, else p2 is current.
    //highlight the player's cell in bright yellow
    int currentRow = (currentPlayerID == 1 ? p1.row : p2.row);
    int currentCol = (currentPlayerID == 1 ? p1.col : p2.col);

    printGridAndValues(rows, cols, board, operations, visited, currentRow, currentCol);

    // Score area
    int boardWidth = COL_INDEX_DISTANCE + SCORE_DISTANCE * cols;
    cout << "\n";
    int pad = boardWidth / SCORE_DISTANCE_ADJUSTMENT - MAX_SIZE;
    if (pad < 0) pad = 0;
    while (pad--) cout << ' ';
    cout << "BLUE(P1): " << p1.score;
    cout << "   GREEN(P2): " << p2.score << "\n\n";

    // Last move info
    if (lastMoveData.valid) {
        cout << "Player ";
        if (lastMoveData.playerID == 1) cout << "1 (Blue)";
        else cout << "2 (Green)";
        cout << " moved from ("
            << lastMoveData.oldRow << ":"
            << lastMoveData.oldCol << ") to ("
            << lastMoveData.newRow << ":"
            << lastMoveData.newCol << ") => Score "
            << lastMoveData.oldScore;

        if (lastMoveData.opUsed != '\0') {
            cout << " ";
            switch (lastMoveData.opUsed) {
            case '+': cout << "+ "; break;
            case '-': cout << "- "; break;
            case '*': cout << "* "; break;
            case '/': cout << "/ "; break;
            }
            cout << lastMoveData.valUsed
                << " = " << lastMoveData.newScore << "\n";
        }
        else {
            cout << " = " << lastMoveData.newScore << "\n";
        }
    }
    controlsInfo(currentPlayerID);
}

// Show help
void printKeybinds() {
    cin.ignore(MAX_IGNORE, '\n');
    cout << "\n----- HELP (Numeric Keypad) -----\n"
        << " 7  8  9 => diagonally up-left, up, up-right\n"
        << " 4  *  6 => left, [5 unused], right\n"
        << " 1  2  3 => diagonally down-left, down, down-right\n"
        << "---------------------------------\n"
        << "Press ENTER to continue...\n";
    cin.ignore(MAX_IGNORE, '\n');
}

// keypad => row/col deltas
bool keypadToDelta(char digit, int& dR, int& dC) {
    switch (digit) {
    case '7': dR = -1; dC = -1; return true;
    case '8': dR = -1; dC = 0; return true;
    case '9': dR = -1; dC = 1; return true;
    case '4': dR = 0; dC = -1; return true;
    case '6': dR = 0; dC = 1; return true;
    case '1': dR = 1; dC = -1; return true;
    case '2': dR = 1; dC = 0; return true;
    case '3': dR = 1; dC = 1; return true;
    }
    return false;
}


// movement logic
bool isValidMove(int newR, int newC, const Player& p,
    int rows, int cols, const int* const* visited, const Player& other) {

    int dR = newR - p.row;
    int dC = newC - p.col;
    if (dR == 0 && dC == 0) return false; //we don't move
    if (dR < -1 || dR>1 || dC < -1 || dC>1) return false; //too far
    if (newR < 0 || newR >= rows || newC < 0 || newC >= cols) return false; //out of the board
    if (visited[newR][newC] != 0) return false; //not empty cell
    return true;
}

bool hasAnyValidMoves(const Player& p, int rows, int cols, int** visited, const Player& other) {

    for (int rr = -1; rr <= 1; rr++) {
        for (int cc = -1; cc <= 1; cc++) {
            if (rr == 0 && cc == 0) continue;
            int nr = p.row + rr;
            int nc = p.col + cc;
            if (isValidMove(nr, nc, p, rows, cols, visited, other)) {
                return true;
            }
        }
    }
    return false;
}

void applyOperation(char op, int& score, int value) {

    switch (op) {
    case '+': score += value; break;
    case '-': score -= value; break;
    case '*': score *= value; break;
    case '/':
        if (value != 0) score /= value;
        break;
    }
}
void saveGame(const char* filename, int rows, int cols, int** board, char** operations,
    int** visited, const Player& p1, const Player& p2, int currentPlayerID)
{
    ofstream ofs(filename); // tries to open the file for writing
    if (!ofs) {
        cout << "Could not open \"" << filename << "\" for writing.\n";
        safeStrCopy(statusMsg, "Auto-save failed!", 256);
        return;
    }

    ofs << rows << " " << cols << "\n";

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) { // writes the board matrix to the file
            ofs << board[i][j];
            if (j + 1 < cols) ofs << " ";
        }
        ofs << "\n";
    }

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            char c = operations[i][j];
            if (c == '\0') c = '_';
            ofs << c;
            if (j + 1 < cols) ofs << " ";
        }
        ofs << "\n";
    }

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            ofs << visited[i][j];
            if (j + 1 < cols) ofs << " ";
        }
        ofs << "\n";
    }

    ofs << p1.row << " " << p1.col << " " << p1.score << " " << p1.id << "\n";
    ofs << p2.row << " " << p2.col << " " << p2.score << " " << p2.id << "\n";

    ofs << currentPlayerID << "\n";

    ofs.close();
}

bool tryPlayerMove(Player& currentP, Player& otherP, int rows, int cols, int** board,
    char** operations, int** visited, int& currentPlayerID) {

    while (true) {
        cout << "\n[Player " << currentP.id << "] Enter move or 'H': ";
        char digit;
        cin >> digit;
        if (!cin) {
            cin.clear();
            cin.ignore(MAX_IGNORE, '\n');
            cout << "Invalid input!\n";
            continue;
        }
        statusMsg[0] = '\0';

        if (digit == 'H' || digit == 'h') {
            safeStrCopy(statusMsg, "Help selected", 256);
            printKeybinds();
            return false;
        }
        else {
            int dR = 0, dC = 0;
            if (!keypadToDelta(digit, dR, dC)) {
                cout << "Invalid key!\n";
                safeStrCopy(statusMsg, "Invalid key pressed!", 256);
                continue;
            }
            int newR = currentP.row + dR;
            int newC = currentP.col + dC;
            if (!isValidMove(newR, newC, currentP, rows, cols,
                visited, otherP)) {
                cout << "Invalid move!\n";
                safeStrCopy(statusMsg, "Invalid move!", 256);
                continue;
            }

            lastMoveData.valid = true;   // Valid move
            lastMoveData.oldRow = currentP.row;
            lastMoveData.oldCol = currentP.col;
            lastMoveData.newRow = newR;
            lastMoveData.newCol = newC;
            lastMoveData.oldScore = currentP.score;
            lastMoveData.playerID = currentP.id;

            char opUsed = operations[newR][newC];
            lastMoveData.opUsed = opUsed;
            lastMoveData.valUsed = board[newR][newC];

            currentP.row = newR; // Move
            currentP.col = newC;
            visited[newR][newC] = currentP.id;

            if (opUsed != '\0') {
                applyOperation(opUsed, currentP.score, board[newR][newC]);
            }

            lastMoveData.newScore = currentP.score;
            currentPlayerID = (currentPlayerID == 1 ? 2 : 1); // Flip turn
            Player p1Local = (currentP.id == 1 ? currentP : otherP); // Auto-save
            Player p2Local = (currentP.id == 1 ? otherP : currentP);

            saveGame(AUTO_SAVE_FILE, rows, cols, board, operations, visited, p1Local, p2Local, currentPlayerID);
            return true;
        }
    }
    return false;
}

bool loadGame(const char* filename, int& rows, int& cols, int**& board, char**& operations, int**& visited,
    Player& p1, Player& p2, int& currentPlayerID) {

    ifstream ifs(filename);
    if (!ifs) {
        return false; // tries to open the file. If it fails, it returns false.
    }
    int r, c;
    ifs >> r >> c; //reads the size of the matrix
    if (!ifs) {
        return false; //If reading fails, the function returns false.
    }
    if (r != rows || c != cols) {  // possibly re-allocate

        freeMatrices(board, operations, visited, rows);
        rows = r;
        cols = c;
        allocateMatrices(board, operations, visited, rows, cols);
    }

    for (int i = 0; i < r; i++) {  //read board
        for (int j = 0; j < c; j++) {
            ifs >> board[i][j];
        }
    }

    for (int i = 0; i < r; i++) {  // read operations
        for (int j = 0; j < c; j++) {
            char c2;
            ifs >> c2;
            if (c2 == '_') c2 = '\0';
            operations[i][j] = c2;
        }
    }

    for (int i = 0; i < r; i++) { // read visited
        for (int j = 0; j < c; j++) {
            ifs >> visited[i][j];
        }
    }
    ifs >> p1.row >> p1.col >> p1.score >> p1.id; // read players
    ifs >> p2.row >> p2.col >> p2.score >> p2.id;
    int nextID;  // read turn
    ifs >> nextID;
    if (!ifs) {
        return false;
    }
    currentPlayerID = nextID;

    ifs.close(); // properly terminate the connection to the file, ensuring that resources are freed,
    return true; //buffers are flushed, and the file remains in a consistent state
}

// Randomly place operations
void randomlyPlaceOperation(int** board, char** operations, int rows, int cols, char operation, int value) {
    int rr, cc;
    do {
        rr = rand() % rows;
        cc = rand() % cols;
    } while ((rr == 0 && cc == 0) || (rr == rows - 1 && cc == cols - 1) || operations[rr][cc] != '\0');

    operations[rr][cc] = operation;
    board[rr][cc] = value;
    return;
}

//The function that initializes random operations in the matrix
void generateMatrices(int** board, char** operations, int rows, int cols) {
    //place a few random ops in random cells
    randomlyPlaceOperation(board, operations, rows, cols, '+', rand() % MAX_NUM);
    randomlyPlaceOperation(board, operations, rows, cols, '-', rand() % ((rows + cols) / VALUE_OPERATION_DIVISOR) + 1);
    randomlyPlaceOperation(board, operations, rows, cols, '*', 2);
    randomlyPlaceOperation(board, operations, rows, cols, '/', 2);
    randomlyPlaceOperation(board, operations, rows, cols, '*', 0);

    //fill remaining cells with random operations
    char possibleOps[POSSIBLE_OPERATION_SIZE] = { '/','-','*','+' };
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (operations[i][j] != '\0') continue;
            char op = possibleOps[rand() % POSSIBLE_OPERATION_SIZE];
            operations[i][j] = op;
            if (op == '/') {
                board[i][j] = rand() % ((rows + cols) / VALUE_OPERATION_DIVISOR) + 1;
            }
            else if (op == '-') {
                board[i][j] = rand() % ((rows + cols) / VALUE_OPERATION_DIVISOR) + 1;
            }
            else if (op == '*') {
                board[i][j] = rand() % ((rows + cols) / VALUE_OPERATION_DIVISOR + 1);
            }
            else {
                board[i][j] = rand() % MAX_NUM;
            }
        }
    }
}

void printFinalScore(const Player& p1, const Player& p2) {
    cout << "\n===== GAME OVER =====\n";
    cout << "Player 1 score = " << p1.score << "\n";
    cout << "Player 2 score = " << p2.score << "\n";

    if (p1.score > p2.score) {
        cout << "Player 1 wins!\n";
    }

    else if (p2.score > p1.score) {
        cout << "Player 2 wins!\n";
    }
    else {
        cout << "It's a tie!\n";
    }
}

int main() {

    cout << "Welcome to MathTricks!\n";
    cout << endl;
    // Ask if we want to start new or load a previous game
    int choice;
    while (true) {
        cout << "1) Start a new game\n";
        cout << "2) Load game\n\n";
        cout << "Enter choice (1 or 2): ";

        if (cin >> choice) {

            if (choice == 1 || choice == 2) {

                break;
            }
            else {
                //Number that is not 1 or 2
                cout << "Invalid choice! Please enter 1 or 2.\n";
            }
        }
        else {
            // for everything except numbers
            cout << "Invalid input! Please enter 1 or 2.\n";
            cin.clear(); // clears errors flags from the cin
            cin.ignore(INT_MAX, '\n');   // // discard characters from the input buffer
        }
    }

    int rows = 0;
    int cols = 0;
    int** board = nullptr;
    char** operations = nullptr;
    int** visited = nullptr;

    Player p1 = { 0,0,0,FIRST_PLAYER };
    Player p2 = { 0,0,0,SECOND_PLAYER };
    int currentPlayerID = 1;

    if (choice == LOAD_GAME) {

        // Attempt to load from autoSave
        // We will guess some minimal default for re-allocation if needed

        rows = MIN_DEFAULT_COLS_ROWS;
        cols = MIN_DEFAULT_COLS_ROWS;

        allocateMatrices(board, operations, visited, rows, cols);

        bool ok = loadGame(AUTO_SAVE_FILE, rows, cols, board,
            operations, visited, p1, p2, currentPlayerID);
        if (!ok) {
            cout << "Load failed or file was not found. Starting a new game.\n";

            freeMatrices(board, operations, visited, rows); // cleanup

            board = nullptr;
            operations = nullptr;
            visited = nullptr;
            choice = 1; // proceed as new game
        }
        else {
            cout << "Game loaded from \"" << AUTO_SAVE_FILE << "\"!\n";
        }
    }

    if (choice == NEW_GAME) {
        // Start new game
        do {
            cout << "Enter rows,cols [MIN:" << MIN_SIZE << " MAX:" << MAX_SIZE << "]: ";
            cin >> rows >> cols;
            if (rows < MIN_SIZE || cols < MIN_SIZE) {
                cout << "Grid must be at least " << MIN_SIZE << " x " << MIN_SIZE << "\n";
            }
            else if (rows > MAX_SIZE || cols > MAX_SIZE) {
                cout << "Grid must be at most " << MAX_SIZE << " x " << MAX_SIZE << "\n";
            }
        } while (rows < MIN_SIZE || cols < MIN_SIZE || rows > MAX_SIZE || cols > MAX_SIZE);

        srand((unsigned)time(nullptr)); //By using the current time as a seed, 
        //the program can generate a different sequence of random numbers every time it runs

        allocateMatrices(board, operations, visited, rows, cols);

        generateMatrices(board, operations, rows, cols);

        //Make start/end empty
        board[0][0] = 0;
        operations[0][0] = '\0';
        board[rows - 1][cols - 1] = 0;
        operations[rows - 1][cols - 1] = '\0';

        // Two players
        p1 = { 0,0,0,FIRST_PLAYER };
        p2 = { rows - 1, cols - 1, 0,SECOND_PLAYER };

        visited[p1.row][p1.col] = p1.id;
        visited[p2.row][p2.col] = p2.id;
        currentPlayerID = FIRST_PLAYER;

        cout << "Starting a NEW game!\n";
    }
    bool gameOver = false;
    printBoard(rows, cols, board, operations, visited, p1, p2, currentPlayerID);

    while (!gameOver) {
        Player& currentP = (currentPlayerID == 1 ? p1 : p2);
        Player& otherP = (currentPlayerID == 1 ? p2 : p1);

        if (!hasAnyValidMoves(currentP, rows, cols, visited, otherP)) {
            cout << "\n[Player " << currentP.id
                << "] has no valid moves! Game over.\n";
            break;
        }

        bool validMove = false;
        while (!validMove) {
            validMove = tryPlayerMove(currentP, otherP, rows, cols, board, operations, visited, currentPlayerID);
            if (!validMove) {
                // user might have pressed help or done invalid moves
                if (!hasAnyValidMoves(currentP, rows, cols, visited, otherP)) {
                    cout << "\n[Player " << currentP.id
                        << "] no valid moves left! Game over.\n";
                    gameOver = true;
                    break;
                }
                printBoard(rows, cols, board, operations, visited, p1, p2, currentPlayerID);
            }
        }
        if (gameOver) break;

        printBoard(rows, cols, board, operations, visited, p1, p2, currentPlayerID);  // re-draw
    }

    printFinalScore(p1, p2);
    freeMatrices(board, operations, visited, rows);

    return 0;
}