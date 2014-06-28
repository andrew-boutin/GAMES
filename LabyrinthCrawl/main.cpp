/*
 * Labyrinth Crawl
 * C++ Terminal Game
 *
 * Written by Andrew Boutin
 *
 * 8/6/2013
 *
 * www.EpicCrawl.com
 *
 * Features are represented by characters printed out to terminal.
 * W for walls, C for character, E for enemies, and S for stairs.
 * Object of the game is to get to the stairs without dieing.
 * There are 10 levels in the game.  In order to win you have to
 * get through all 10 levels.  Each level has x amount of enemies.
 * There are more enemies with each level that you advance to.
 * Move character around with w, a, s, and d keys.  Attempting
 * to move through an enemy attacks it.  When you attack an enemy
 * the attack doesn't stop until the player or enemy dies.
 */

// Includes
#include <iostream>
#include <stack>
#include <vector>
#include <stdlib.h>
#include <windows.h>
#include <ctime>

using namespace std;

// Defined variables
#define NumRows 15
#define NumCols 30
#define NumRandomGaps 10
#define enemyStartingHealth 50
#define playerStartHealth 100

// Different types of objects in the game
enum MazeObject{
    wall,
    emptySpace,
    stairs,
    enemy
};

// Grid is made up of cells
struct Cell{
    MazeObject cellType;
    int enemyHealth; // If MazeObject is enemy then this will be used
    int xLoc;
    int yLoc;
    bool visible;
};

// Relevent locations on grid during gameplay and player info.
int curX, curY, moveX, moveY, curLevel, stairsX, stairsY, playerHealth, damageToPlayer;
bool lostGame, levelOver, wasAttack;
Cell cells[NumRows][NumCols]; // The grid
HANDLE myHandle = GetStdHandle(STD_OUTPUT_HANDLE);

// Determine what squares need to become visible based on current location
void setCurrentViewVisible(){
    for(int i = -2; i < 3; ++i){
        for(int j = -2; j < 3; ++j){
            if(curX + i >= 0 && curX + i < NumCols && curY + j >= 0 && curY + j < NumRows){
                cells[curY + j][curX + i].visible = true;
            }
        }
    }
}

// Print out the current state of the grid and all other required info/messages.
void printState(){
    // 0x0A green ---- ,0x0B teal, 0x0C red ---- , 0x0D purple, 0x0E yellow ----, 0x0F sand, 0x00 black
    // 0x01 dark blue, 0x02 dark green, 0x03 dark teal, 0x04 dark red, 0x05 dark purple
    // 0x06 dark yellow, 0x07 dark white, 0x08 dark grey, 0x09 bright blue
    system("cls");

    for(int i = 0; i < NumRows; ++i){
        for(int j = 0; j < NumCols; ++j){
            if(cells[i][j].visible == false){ // Not visible yet, draw black space
                SetConsoleTextAttribute(myHandle, 0x0A);
                cout << " ";
            }
            else{
                if(curY == i && curX == j){ // Character space
                    SetConsoleTextAttribute(myHandle, 0x90);
                    cout << "C";
                }
                else if(cells[i][j].cellType == emptySpace){ // Floor space
                    SetConsoleTextAttribute(myHandle, 0x70);
                    cout << " ";
                }
                else if(cells[i][j].cellType == wall){ // Wall
                    SetConsoleTextAttribute(myHandle, 0x0E);
                    cout << "W";
                }
                else if(cells[i][j].cellType == enemy){ // Enemy
                    SetConsoleTextAttribute(myHandle, 0x0C);
                    cout << "E";
                }
                else if(cells[i][j].cellType == stairs){ // Stairs
                    SetConsoleTextAttribute(myHandle, 0x0B);
                    cout << "S";
                }
                else{ // Bad juju
                    cout << "?";
                }
            }
        }

        cout << endl;
    }

    SetConsoleTextAttribute(myHandle, 0x0A);

    cout << "============================================================" << endl;
    cout << "==== Commands ======== Map Key ============ Stats ==========" << endl;
    cout << "  W : Move up.    |  W - Wall       |  Current level  : " << curLevel << endl;
    cout << "  S : Move down.  |  E - Enemy      |  Current health : " << playerHealth << endl;
    cout << "  A : Move left.  |  S - Stairs     |                        " << endl;
    cout << "  D : Move right. |  C - Character  |                        " << endl;
    cout << "  Q : Quit game.  |                 |                        " << endl;
    cout << "============================================================" << endl;
    cout << "============= Press enter to execute a command =============" << endl;
    cout << "============================================================" << endl;

    if(wasAttack){ // Attack information
        cout << "Killed the enemy!  Player took " << damageToPlayer << " damage!" << endl;
    }
}

// Determine if grid location is suitable to become an empty space, used for creating the maze
bool isValidNeighbor(int curX, int curY, int dir){
    if(cells[curY][curX].cellType == emptySpace) // Already empty space
        return false;

    if(dir != 0){ // If direction is left then skip
        if((curX + 1 < NumCols) && cells[curY][curX + 1].cellType == emptySpace) // Check to right of (curX, curY)
            return false;
    }
    if(dir != 1){ // If direction is right then skip
        if((curX - 1 >= 0) && cells[curY][curX - 1].cellType == emptySpace){ // Check to left of (curX, curY)
            return false;
        }
    }
    if(dir != 2){ // If direction is up then skip
        if((curY + 1 < NumRows) && cells[curY + 1][curX].cellType == emptySpace) // Check to down of (curX, curY)
            return false;
    }
    if(dir != 3){ // If direction is down then skip
        if((curY - 1 >= 0) && cells[curY - 1][curX].cellType == emptySpace) // Check to up of (curX, curY)
            return false;
    }

    return true;
}

// Determine if grid location doesn't have 2 or more open paths connected to it
bool isValidPath(int curX, int curY){
    int numOpenNeighbors = 0;

    if((curX + 1 < NumCols) && cells[curY][curX + 1].cellType == emptySpace) numOpenNeighbors++;
    if((curX - 1 >= 0) && cells[curY][curX - 1].cellType == emptySpace) numOpenNeighbors++;
    if((curY + 1 < NumRows) && cells[curY + 1][curX].cellType == emptySpace) numOpenNeighbors++;
    if((curY - 1 >= 0) && cells[curY - 1][curX].cellType == emptySpace) numOpenNeighbors++;

    if(numOpenNeighbors > 1)
        return false;
    return true;
}

// Create the stairs in the grid
void setUpStairs(){
    bool invalid = true;

    while(invalid){
        do{
            stairsX = rand() % NumCols;
        } while(stairsX == 0 || stairsX == NumCols - 1 || abs(stairsX - curX) < ((NumCols / 2) - 1));

        do{
            stairsY = rand() % NumRows;
        } while(stairsY == 0 || stairsY == NumRows - 1 || abs(stairsY - curY) < ((NumRows / 2) - 1));

        if(cells[stairsY][stairsX].cellType == emptySpace)
            invalid = false;
    }

    cells[stairsY][stairsX].cellType = stairs;
}

// Open up a few random spots in the maze to create more path options
void createRandomGaps(){
    int randX, randY;
    bool invalid;

    for(int i = 0; i < NumRandomGaps; ++i){
        invalid = true;
        while(invalid){
            do{
                randX = rand() % NumCols;
            } while(randX == 0 || randX == NumCols - 1);

            do{
                randY = rand() % NumRows;
            } while(randY == 0 || randY == NumRows - 1);

            if(cells[randY][randX].cellType == wall)
                invalid = false;
        }
        cells[randY][randX].cellType = emptySpace;
    }
}

// Place given amount of enemies on grid based on current level
void createEnemies(){
    int randX, randY;
    bool invalid;

    for(int i = 0; i < (5 + (curLevel * 2 - 2)); ++i){
        invalid = true;
        while(invalid){
            do{
                randX = rand() % NumCols;
            } while(randX == 0 || randX == NumCols - 1);

            do{
                randY = rand() % NumRows;
            } while(randY == 0 || randY == NumRows - 1);

            if(cells[randY][randX].cellType == emptySpace && !(randX == curX && randY == curY))
                invalid = false;
        }
        cells[randY][randX].cellType = enemy;
        cells[randY][randX].enemyHealth = enemyStartingHealth;
    }
}

// Common function calls made in multiple spots
void initializeOthers(){
    setUpStairs();
    createEnemies();
    createRandomGaps();
    setCurrentViewVisible();
}

// Set up the grid, randomly create a maze, populate walls, enemies, stairs, place character
void initializeDungeon(){
    do{
        curX = rand() % NumCols;
    } while(curX == 0 || curX == NumCols - 1);

    do{
        curY = rand() % NumRows;
    } while(curY == 0 || curY == NumRows - 1);

    for(int i = 0; i < NumRows; ++i){
        for(int j = 0; j < NumCols; ++j){
            cells[i][j].cellType = wall;
            cells[i][j].xLoc = j;
            cells[i][j].yLoc = i;
            if(i == 0 || j == 0 || i == NumRows - 1 || j == NumCols - 1)
                cells[i][j].visible = true;
            else
                cells[i][j].visible = false;
        }
    }

    wasAttack = false;
    vector<Cell*> cellStack;
    int totalCells = NumRows * NumCols;
    Cell* currentCell = &cells[curY][curX]; // Choose cell at random
    int numVisitedCells = 1;
    vector<Cell*> neighbors;

    do{
        (*currentCell).cellType = emptySpace;

        neighbors.clear();
        int tmpX = (*currentCell).xLoc, tmpY = (*currentCell).yLoc;

        if((*currentCell).xLoc - 1 >= 1){
            if(isValidNeighbor(tmpX - 1, tmpY, 0))
                neighbors.push_back(&cells[tmpY][tmpX - 1]);
        }
        if((*currentCell).xLoc + 1 < NumCols - 1){
            if(isValidNeighbor(tmpX + 1, tmpY, 1))
                neighbors.push_back(&cells[tmpY][tmpX + 1]);
        }
        if((*currentCell).yLoc - 1 >= 1){
            if(isValidNeighbor(tmpX, tmpY - 1, 2))
                neighbors.push_back(&cells[tmpY - 1][tmpX]);
        }
        if((*currentCell).yLoc + 1 < NumRows - 1){
            if(isValidNeighbor(tmpX, tmpY + 1, 3))
                neighbors.push_back(&cells[tmpY + 1][tmpX]);
        }

        if(neighbors.size() > 0){ // if one or more found then choose one at random
            int randomInt = rand() % neighbors.size();
            Cell* tempCell = neighbors.at(randomInt);

            neighbors.erase(neighbors.begin() + randomInt);

            while(neighbors.size() > 0){
                randomInt = rand() % neighbors.size();
                cellStack.push_back(neighbors.at(randomInt));
                neighbors.erase(neighbors.begin() + randomInt);
            }

            cellStack.push_back(currentCell); // push currentcell location on the cellstack
            currentCell = tempCell; // make the new cell currentcell
            ++numVisitedCells;
        }
        else{
            do{
                if(cellStack.size() == 0){
                    initializeOthers();
                    return;
                }
                currentCell = cellStack.back(); // pop most recent cell entry off the cell stack, make it current cell
                cellStack.pop_back();
            } while((*currentCell).cellType == emptySpace || !isValidPath((*currentCell).xLoc, (*currentCell).yLoc));
        }
    } while(cellStack.size() > 0);

    initializeOthers();
}

// Determine if player can move to desired location
bool validMove(){
    if(!(moveX >= 0 && moveX < NumCols && moveY >= 0 && moveY < NumRows)) return false;
    if(cells[moveY][moveX].cellType == wall) return false;

    return true;
}

// Player attacks enemy, enemy attacks player, continues until one dies.
void attack(){
    wasAttack = true;
    int tmpDamage;
    damageToPlayer = 0;
    while(1){
        cells[moveY][moveX].enemyHealth -= rand() % 10; // Have player attack enemy, take health away from enemy

        if(cells[moveY][moveX].enemyHealth <= 0){ // check if enemy is dead
            cells[moveY][moveX].cellType = emptySpace; // if dead then change cell type and exit
            return;
        }

        tmpDamage = rand() % 5;
        playerHealth -= tmpDamage; // Have enemy attack player, take health away from player
        damageToPlayer += tmpDamage;

        if(playerHealth <= 0){ // check if player is dead
            lostGame = true; // if dead then exit, restart game
            levelOver = true;
            return;
        }
    }
}

// Either move the player or attack the enemy
void makeMove(){
    if(cells[moveY][moveX].cellType == enemy){
        attack();
    }
    else{
        curX = moveX;
        curY = moveY;
        setCurrentViewVisible();
    }
}

// Main game loop
void gameLoop(){
    bool validCommand = true;
    string input;
    levelOver = false;

    while(!levelOver){
        if(validCommand == true)
            printState();

        wasAttack = false;

        validCommand = true;
        cin >> input;

        switch(input[0]){
        case 119: case 87: // S, s
            moveX = curX;
            moveY = curY - 1;
            if(validMove())
                makeMove();
            break;
        case 115: case 83: // W, w
            moveX = curX;
            moveY = curY + 1;
            if(validMove())
                makeMove();
            break;
        case 100: case 68: // D, d
            moveX = curX + 1;
            moveY = curY;
            if(validMove())
                makeMove();
            break;
        case 97: case 65: // A, a
            moveX = curX - 1;
            moveY = curY;
            if(validMove())
                makeMove();
            break;
        case 113: case 81: // Q, q
            levelOver = true;
            lostGame = true;
            break;
        default:
            validCommand = false;
            break;
        }

        if(curX == stairsX && curY == stairsY){
            printState();
            cout << "You made it out of this level!" << endl;
            levelOver = true;
            break;
        }
    }
}

// Print message on game exit
void exitGame(){
    cout << "Exiting Labyrinth Crawl! " << endl
         << endl
         << "Check out our website at www.EpicCrawl.com for more awesome games!"
         << endl;
    cout << "Thanks for playing!" << endl;
}

// Print message on game start
void printIntroMessage(){
    cout << "Welcome to Labyrinth Crawl!" << endl
         << endl
         << "After waking up from a night of heavy drinking," << endl
         << "you have found yourself trapped in the Labyrinth!" << endl
         << endl
         << "In order to get out you'll have to fight your way" << endl
         << "through all 10 levels of the Labyrinth!" << endl
         << endl;

    cout << "Press the enter key to continue!" << endl;
        while(cin.get() != '\n'){}

    system("cls");

    cout << "In order to complete a level you have to reach the stairs." << endl
         << endl
         << "Beware, there are enemies in the Labyrinth!  In order to get" << endl
         << "by them you have to battle!" << endl
         << endl
         << "To battle an enemy, attempt to move through them.  Be warned though," << endl
         << "combat only ends when either the enemy or the player dies!" << endl
         << endl
         << "The player's health refills upon the start of each new level." << endl
         << endl;
}

// Set up game, enter loop, aquire input and handle
int main(){
    SetConsoleTextAttribute(myHandle, 0x0A);

    printIntroMessage();

    bool stillPlaying = true, validResponse = false;
    char response;

    while(stillPlaying){
        curLevel = 1;
        lostGame = false;
        cout << "Press the enter key to start your journey!" << endl;
        while(cin.get() != '\n'){}

        validResponse = false;
        srand((int)time(0));

        while(!lostGame && curLevel != 11){
            playerHealth = playerStartHealth;
            initializeDungeon();
            gameLoop();

            if(!lostGame){
                ++curLevel;
                cout << "Press the enter key to start the next level!" << endl;
                while(cin.get() != '\n'){}
            }
        }

        if(curLevel == 11 && !lostGame){
            cout << "Congratulations!  You made it out alive!" << endl;
        }

        if(lostGame){
            cout << "You died!" << endl;
        }

        do{
            cout << "Play again?  Y / N." << endl;
            cin >> response;

            // Y == 89, N == 78, y == 121, n == 110
            if(response == 89 || response == 78 || response == 121 || response == 110)
                validResponse = true;

        } while(!validResponse);

        if(response == 78 || response == 110)
            stillPlaying = false;
    }

    exitGame();
    return 0;
}
