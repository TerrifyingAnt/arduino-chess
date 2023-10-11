#include "logger.h"
#include "ChessBoard.h"
#include "ChessMove.h"
#include "chess-minimax.h"
#include <Arduino.h>
#include <GyverMAX7219.h>
#include <Thread.h>


ChessBoard board;
bool whitePlays;

int mode = 0;
int depth = 2;
int maxSteps = 10;

MAX7219 <1, 1, 12, 14, 13> mtrx;   // одна матрица (1х1), пин CS на D5
bool even = false;
const int dataPin = 26;   /* Q7 */
const int clockPin = 27;  /* CP */
const int latchPin = 33;  /* PL */

int DELAY_TIME = 250;


Thread ledThread = Thread(); // создаём поток управления светодиодом
Thread shiftRegistersThread = Thread(); // создаём поток управления сиреной

const int numBits = 16; // Change this to match your specific number of bits
int buttonStates[numBits] = {1}; // Initialize all button states as not pressed (0)
int lightningButtonStates[numBits] = {1};

bool boolPlay = true;

void startGamePlayerComputer(int depth, int maxSteps);
int userMove(ChessBoard& board);
void computerMove(ChessBoard& board, int depth, int maxSteps);


void ledForFigures(ChessBoard& tempBoard) {
  mtrx.clear();
  for(int i = 0; i < 8; i++) {
    for(int j = 0; j < 8; j++) {
      if(!tempBoard.board[i * 8 + j].empty()) {
        mtrx.dot(i, j);
      }
    }
  }
  mtrx.update();
}


void setup() {
  mtrx.begin();       // запускаем
  mtrx.setBright(15);  // яркость 0..15
  mtrx.clear();
  mtrx.update();

  Serial.begin(9600);
  whitePlays = true;

  Println("$== CHESS MATH ==$");
    Println("Moves are written like 'e2 e4'");

    Print("Depth set to: ");
    Println(depth);

    Print("Max steps set to: ");
    Println(maxSteps);

    Println("");

  pinMode(dataPin, INPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(latchPin, OUTPUT);

  //ledThread.onRun(ledForFigures);
  //ledThread.setInterval(DELAY_TIME);

  //shiftRegistersThread.onRun(shiftRegisters);
  //shiftRegistersThread.setInterval(5);
}

int userMove(ChessBoard& board)
{
    if (board.whitePlays)
        Print("White");
    else
        Print("Black");

    Println(" goes next");

    Println("Perform move:");
    char buffer[12];
    Serial.readBytesUntil('\n', buffer, sizeof(buffer));

    if (strlen(buffer) == 3) {
        Print("Possible moves: ");
        ChessMove move = ChessMove(buffer);
        board.possibleMoves(move.from).printList();
        Println("");
        return 1;
    }

    if (strncmp(buffer, "exit\n", sizeof("exit\n")) == 0) return 2;

    ChessMove move(buffer);

    if (board.validMove(move)) {
        Print("Moving ");
        char name[16];
        board.board[move.from].name(name);
        Print(name);
        Print(" ");
        move.printMove();

        if (!board.board[move.to].empty()) {
            Print("Taking ");
            board.board[move.to].name(name);
            Println(name);
        }

        Println("");

        board.performMove(move);
        ledForFigures(board);
    } else {
        Println("Invalid move try again");
        return 1;
    }

    return 0;
}

void computerMove(ChessBoard& board, int depth, int maxSteps)
{
    Println("Computer is thinking...");

    ChessEngine engine;

    //ChessMove computerMove = engine.calculateMoveIterative(board, maxSteps);
    ChessMove computerMove = engine.calculateMove(board, depth, whitePlays);

    Print("Computer moving ");
    char name[16];
    board.board[computerMove.from].name(name);
    Print(name);
    Print(" ");
    computerMove.printMove();
    Print(" - With calculated score: ");
    Println(computerMove.score);
    Print(" - Total minimax calls: ");
    Println(engine.getSteps());
    Print(" - Total minimax optimization swaps: ");
    Println(engine.getSwaps());
    Print(" - Total transposition table size: ");
    Println(engine.getTransTableSize());
    Print(" - Total transposition table uses: ");
    Println(engine.getTransTableUses());
    Println("");
    if (!board.board[computerMove.to].empty()) {
        Print("Taking ");
        board.board[computerMove.to].name(name);
        Println(name);
    }

    board.performMove(computerMove);
}



void loop() {
  //ledForFigures();
  while (boolPlay) {
      Print("Current board score: ");
      Println(ChessEngine::evaluateMoveScore(board));

      board.printBoard();

      if (!board.whitePlays) {
          computerMove(board, depth, maxSteps);
          ledForFigures(board);
          continue;
      }

      int status;
      status = userMove(board);
      if (status == 1) continue;
      if (status == 2) boolPlay = false;

      int endState = board.gameEnded();
      if (endState != 0) {
          if (endState == 1)
              Print("Black");
          else
              Print("White");

          Println(" has won!");
          boolPlay = false;
      }
  }
}
