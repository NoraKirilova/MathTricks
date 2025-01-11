#include <iostream>     
#include <iomanip>      
#include <fstream>      
#include <cstdlib>      
#include <ctime>        
#include <windows.h>    

using namespace std;

constexpr int FG_WHITE = 0xF;
constexpr int FG_YELLOW = 0xE;
constexpr int BG_BLUE = 0x10;
constexpr int BG_GREEN = 0x20;
constexpr int DEFAULT_COLOR = 0x07;

constexpr int MIN_SIZE = 4;
constexpr int MAX_SIZE = 10;
constexpr int VALUE_DIVISOR = 3;
constexpr int POSSIBLE_OPERATION_SIZE = 4;


void setColorFG_BG(int fg, int bg) {
    int color = bg | fg;
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}


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
    int row;
    int col;
    int score;
    int id;  
};


struct LastMoveData {
    bool valid;
    int oldRow;
    int oldCol;
    int newRow;
    int newCol;
    int oldScore;
    int newScore;
    char opUsed;      
    int valUsed;
    int playerID;     
};
static LastMoveData lastMoveData = { false, 0,0,0,0, 0,0, '\0',0,0 };


static char statusMsg[256] = "";


void generateMatrices(int** board, char** operations, int rows, int cols);
bool isValidMove(int newR, int newC, const Player& p, int rows, int cols,
    int** visited, const Player& other);
bool hasAnyValidMoves(const Player& p, int rows, int cols,
    int** visited, const Player& other);
bool tryPlayerMove(Player& p, int rows, int cols,
    int** board, char** operations, int** visited,
    Player& other);
void applyOperation(char op, int& score, int value);


void saveGame(const char* filename,
    int rows, int cols,
    int** board, char** operations, int** visited,
    const Player& p1, const Player& p2);
bool loadGame(const char* filename,
    int& rows, int& cols,
    int**& board, char**& operations, int**& visited,
    Player& p1, Player& p2);


void printBoard(int rows, int cols,
    int** board, char** operations, int** visited,
    const Player& p1, const Player& p2)
{
    clearScreen();

   
    cout << "      ";
    for (int j = 0; j < cols; j++) {
        cout << setw(6) << j;
    }
    cout << endl;

    
    for (int i = 0; i <= rows; i++) {
        
        cout << "     ";
        for (int j = 0; j < cols; j++) {
            cout << "+------";
        }
        cout << "+" << endl;

        if (i < rows) {
            
            cout << setw(4) << i << " ";
            for (int j = 0; j < cols; j++) {
                cout << "| ";

                int v = visited[i][j];
                char opChar = (operations[i][j] == '\0') ? ' ' : operations[i][j];
                int val = board[i][j];

                if (v == 1) {
                   
                    setColorFG_BG(FG_WHITE, BG_BLUE);
                    cout << opChar;
                    setColorFG_BG(FG_YELLOW, BG_BLUE);
                    cout << setw(3) << val;
                    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), DEFAULT_COLOR);
                }
                else if (v == 2) {
                  
                    setColorFG_BG(FG_WHITE, BG_GREEN);
                    cout << opChar;
                    setColorFG_BG(FG_YELLOW, BG_GREEN);
                    cout << setw(3) << val;
                    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), DEFAULT_COLOR);
                }
                else {
                    
                    cout << opChar << setw(3) << val;
                }
                cout << " ";
            }
            cout << "|" << endl;
        }
    }

    
    int boardWidth = 6 + 8 * cols;

    
    cout << "\n";
    int pad = boardWidth / 2 - 10;
    if (pad < 0) pad = 0;
    for (int i = 0; i < pad; i++) {
        cout << ' ';
    }
    cout << "BLUE: " << p1.score << "     GREEN: " << p2.score << "\n\n";

    
    if (lastMoveData.valid) {
        cout << "Player ";
        if (lastMoveData.playerID == 1) { cout << "1 (Blue)"; }
        else { cout << "2 (Green)"; }
        cout << " moved from ("
            << lastMoveData.oldRow << ":"
            << lastMoveData.oldCol << ")"
            << " to ("
            << lastMoveData.newRow << ":"
            << lastMoveData.newCol << "). Score "
            << lastMoveData.oldScore;
        if (lastMoveData.opUsed != '\0') {
            switch (lastMoveData.opUsed) {
            case '+': cout << " + "; break;
            case '-': cout << " - "; break;
            case '*': cout << " * "; break;
            case '/': cout << " / "; break;
            }
            cout << lastMoveData.valUsed
                << " = " << lastMoveData.newScore << "\n";
        }
        else {
            cout << " = " << lastMoveData.newScore << "\n";
        }
    }

    
    cout << "\n=== CONTROLS ===\n"
        << "Movement: 7,8,9,4,6,1,2,3 (diagonals allowed)\n"
        << "Save: 'S'   Load: 'L'   Help: 'H'\n\n";

    
    if (statusMsg[0] != '\0') {
        cout << "[STATUS] " << statusMsg << "\n\n";
    }
}


void printHelpMessage() {
    cin.ignore(10000, '\n');
    cout << "\n----- HELP (Numeric Keypad) -----\n"
        << " 7  8  9 => diagonally up-left, up, up-right\n"
        << " 4  *  6 => left, [5 unused], right\n"
        << " 1  2  3 => diagonally down-left, down, down-right\n"
        << " S => Save   L => Load   H => This help\n"
        << "---------------------------------\n"
        << "Press ENTER to continue...\n";

    cin.ignore(10000, '\n');
}


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
    default:  return false;
    }
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


bool isValidMove(int newR, int newC, const Player& p, int rows, int cols,
    int** visited, const Player& other)
{
    int rowDiff = newR - p.row;
    int colDiff = newC - p.col;
    if (rowDiff == 0 && colDiff == 0) return false;
    if (rowDiff < -1 || rowDiff>1 || colDiff < -1 || colDiff>1) return false;
    if (newR < 0 || newR >= rows || newC < 0 || newC >= cols) return false;
    if (visited[newR][newC] != 0) return false;
    if (newR == other.row && newC == other.col) return false;
    return true;
}


bool hasAnyValidMoves(const Player& p, int rows, int cols, int** visited, const Player& other) {
    for (int rDiff = -1; rDiff <= 1; rDiff++) {
        for (int cDiff = -1; cDiff <= 1; cDiff++) {
            if (rDiff == 0 && cDiff == 0) continue;
            int rr = p.row + rDiff;
            int cc = p.col + cDiff;
            if (isValidMove(rr, cc, p, rows, cols, visited, other)) return true;
        }
    }
    return false;
}


bool tryPlayerMove(Player& p, int rows, int cols,
    int** board, char** operations, int** visited,
    Player& other)
{
    while (true) {
        cout << "\n[Player " << p.id << "] Enter your move or 'S','L','H': ";
        char digit;
        cin >> digit;
        if (!cin) {
            cin.clear();
            cin.ignore(10000, '\n');
            cout << "Invalid input!\n";
            continue;
        }

        
        statusMsg[0] = '\0';

        if (digit == 'H' || digit == 'h') {
            
            safeStrCopy(statusMsg, "Help was selected. See instructions above!", 256);
            printHelpMessage();
            return false; 
        }
        else if (digit == 'S' || digit == 's') {
           
            safeStrCopy(statusMsg, "SAVE selected. Saving to \"save.txt\"...", 256);
            if (p.id == 1) {
                saveGame("save.txt", rows, cols, board, operations, visited, p, other);
            }
            else {
                saveGame("save.txt", rows, cols, board, operations, visited, other, p);
            }
            return false; 
        }
        else if (digit == 'L' || digit == 'l') {
            
            safeStrCopy(statusMsg, "LOAD selected. Loading from \"save.txt\"...", 256);
            if (p.id == 1) {
                if (!loadGame("save.txt", rows, cols, board, operations, visited, p, other)) {
                    safeStrCopy(statusMsg, "Load canceled/failed!", 256);
                }
            }
            else {
                if (!loadGame("save.txt", rows, cols, board, operations, visited, other, p)) {
                    safeStrCopy(statusMsg, "Load canceled/failed!", 256);
                }
            }
            return false; 
        }
        else {
            
            int dR = 0, dC = 0;
            if (!keypadToDelta(digit, dR, dC)) {
                cout << "[Player " << p.id << "] Invalid key!\n";
                safeStrCopy(statusMsg, "Invalid key pressed!", 256);
                continue;
            }
            int newR = p.row + dR;
            int newC = p.col + dC;
            if (!isValidMove(newR, newC, p, rows, cols, visited, other)) {
                cout << "[Player " << p.id << "] Invalid move!\n";
                safeStrCopy(statusMsg, "Invalid move!", 256);
                continue;
            }
            
            lastMoveData.valid = true;
            lastMoveData.oldRow = p.row;
            lastMoveData.oldCol = p.col;
            lastMoveData.newRow = newR;
            lastMoveData.newCol = newC;
            lastMoveData.oldScore = p.score;
            lastMoveData.playerID = p.id;
            char opUsed = operations[newR][newC];
            lastMoveData.opUsed = opUsed;
            lastMoveData.valUsed = board[newR][newC];

            
            p.row = newR;
            p.col = newC;
            visited[newR][newC] = p.id;

            
            if (opUsed != '\0') {
                applyOperation(opUsed, p.score, board[newR][newC]);
            }
            lastMoveData.newScore = p.score;

            
            return true;
        }
    }
    
    return false;
}


void randomlyPlaceOperation(int** board, char** operations, int rows, int cols,
    char operation, int value)
{
    while (true) {
        int r = rand() % rows;
        int c = rand() % cols;
        if (operations[r][c] == '\0') {
            operations[r][c] = operation;
            board[r][c] = value;
            return;
        }
    }
}

void generateMatrices(int** board, char** operations, int rows, int cols) {
    randomlyPlaceOperation(board, operations, rows, cols, '+', rand() % 10);
    randomlyPlaceOperation(board, operations, rows, cols, '-', rand() % ((rows + cols) / VALUE_DIVISOR) + 1);
    randomlyPlaceOperation(board, operations, rows, cols, '*', 2);
    randomlyPlaceOperation(board, operations, rows, cols, '/', 2);
    randomlyPlaceOperation(board, operations, rows, cols, '*', 0);

    static char possibleOps[POSSIBLE_OPERATION_SIZE] = { '/','-','*','+' };
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (operations[i][j] != '\0') continue;
            char op = possibleOps[rand() % POSSIBLE_OPERATION_SIZE];
            operations[i][j] = op;
            if (op == '/') {
                board[i][j] = rand() % ((rows + cols) / VALUE_DIVISOR) + 1;
            }
            else if (op == '-') {
                board[i][j] = rand() % ((rows + cols) / VALUE_DIVISOR) + 1;
            }
            else if (op == '*') {
                board[i][j] = rand() % ((rows + cols) / VALUE_DIVISOR + 1);
            }
            else {
                board[i][j] = rand() % 10;
            }
        }
    }
}


void saveGame(const char* filename, int rows, int cols,
    int** board, char** operations, int** visited,
    const Player& p1, const Player& p2)
{
    ofstream ofs(filename);
    if (!ofs) {
        cout << "Could not open \"" << filename << "\" for writing.\n";
        safeStrCopy(statusMsg, "Save failed - can't open file!", 256);
        return;
    }
    ofs << rows << " " << cols << "\n";
    
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
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

    ofs.close();
    cout << "Game saved to \"" << filename << "\"!\n";
    safeStrCopy(statusMsg, "Game saved successfully!", 256);
}


bool loadGame(const char* filename, int& rows, int& cols,
    int**& board, char**& operations, int**& visited,
    Player& p1, Player& p2)
{
    ifstream ifs(filename);
    if (!ifs) {
        cout << "Could not open \"" << filename << "\" for reading.\n";
        safeStrCopy(statusMsg, "Load failed - can't open file!", 256);
        return false;
    }
    int r, c;
    ifs >> r >> c;
    if (!ifs) {
        cout << "Invalid format for rows/cols.\n";
        safeStrCopy(statusMsg, "Load failed - invalid format!", 256);
        return false;
    }
    if (r != rows || c != cols) {
        for (int i = 0; i < rows; i++) {
            delete[] board[i];
            delete[] operations[i];
            delete[] visited[i];
        }
        delete[] board;
        delete[] operations;
        delete[] visited;

        rows = r;
        cols = c;
        board = new int* [rows];
        operations = new char* [rows];
        visited = new int* [rows];
        for (int i = 0; i < rows; i++) {
            board[i] = new int[cols]();
            operations[i] = new char[cols]();
            visited[i] = new int[cols]();
        }
    }
    
    for (int i = 0; i < r; i++) {
        for (int j = 0; j < c; j++) {
            ifs >> board[i][j];
        }
    }
    
    for (int i = 0; i < r; i++) {
        for (int j = 0; j < c; j++) {
            char c2;
            ifs >> c2;
            if (c2 == '_') c2 = '\0';
            operations[i][j] = c2;
        }
    }
    
    for (int i = 0; i < r; i++) {
        for (int j = 0; j < c; j++) {
            ifs >> visited[i][j];
        }
    }
    
    ifs >> p1.row >> p1.col >> p1.score >> p1.id;
    ifs >> p2.row >> p2.col >> p2.score >> p2.id;
    ifs.close();
    cout << "Game loaded from \"" << filename << "\"!\n";
    safeStrCopy(statusMsg, "Game loaded successfully!", 256);
    return true;
}


int main() {
    int rows, cols;
    cout << "Enter the rows and the cols of the grid [MIN:" << MIN_SIZE << " MAX:" << MAX_SIZE << "]: ";
    do {
        cin >> rows >> cols;
        if (rows < MIN_SIZE || cols < MIN_SIZE) {
            cout << "Grid must be at least " << MIN_SIZE << " x " << MIN_SIZE << "\n";
        }
        else if (rows > MAX_SIZE || cols > MAX_SIZE) {
            cout << "Grid must be at most " << MAX_SIZE << " x " << MAX_SIZE << "\n";
        }
    } while (rows<MIN_SIZE || cols<MIN_SIZE || rows>MAX_SIZE || cols>MAX_SIZE);

    srand((unsigned)time(0));

    
    int** board = new int* [rows];
    char** operations = new char* [rows];
    int** visited = new int* [rows];
    for (int i = 0; i < rows; i++) {
        board[i] = new int[cols]();
        operations[i] = new char[cols]();
        visited[i] = new int[cols]();
    }

    
    generateMatrices(board, operations, rows, cols);
   
    board[0][0] = 0; operations[0][0] = '\0';
    board[rows - 1][cols - 1] = 0; operations[rows - 1][cols - 1] = '\0';

   
    Player p1 = { 0,0,0,1 };
    Player p2 = { rows - 1, cols - 1, 0, 2 };
    visited[p1.row][p1.col] = p1.id;
    visited[p2.row][p2.col] = p2.id;

   
    printBoard(rows, cols, board, operations, visited, p1, p2);

    bool gameOver = false;

   
    while (!gameOver) {
       
        if (!hasAnyValidMoves(p1, rows, cols, visited, p2)) {
            cout << "\n[Player 1] has no valid moves! Game over.\n";
            break;
        }
        bool validMove = false;
        while (!validMove) {
            validMove = tryPlayerMove(p1, rows, cols, board, operations, visited, p2);
            if (!validMove) {
                if (!hasAnyValidMoves(p1, rows, cols, visited, p2)) {
                    cout << "\n[Player 1] no valid moves left! Game over.\n";
                    gameOver = true;
                    break;
                }
                printBoard(rows, cols, board, operations, visited, p1, p2);
            }
        }
        if (gameOver) break;
        printBoard(rows, cols, board, operations, visited, p1, p2);

        
        if (!hasAnyValidMoves(p2, rows, cols, visited, p1)) {
            cout << "\n[Player 2] has no valid moves! Game over.\n";
            break;
        }
        validMove = false;
        while (!validMove) {
            validMove = tryPlayerMove(p2, rows, cols, board, operations, visited, p1);
            if (!validMove) {
                if (!hasAnyValidMoves(p2, rows, cols, visited, p1)) {
                    cout << "\n[Player 2] no valid moves left! Game over.\n";
                    gameOver = true;
                    break;
                }
                printBoard(rows, cols, board, operations, visited, p1, p2);
            }
        }
        if (gameOver) break;
        printBoard(rows, cols, board, operations, visited, p1, p2);
    }

    
    cout << "\n===== GAME OVER =====\n";
    cout << "Player 1 score = " << p1.score << "\n";
    cout << "Player 2 score = " << p2.score << "\n";
    if (p1.score > p2.score)       cout << "Player 1 wins!\n";
    else if (p2.score > p1.score)  cout << "Player 2 wins!\n";
    else                        cout << "It's a tie!\n";

    
    for (int i = 0; i < rows; i++) {
        delete[] board[i];
        delete[] operations[i];
        delete[] visited[i];
    }
    delete[] board;
    delete[] operations;
    delete[] visited;

    return 0;
}
