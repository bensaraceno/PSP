//-------------------------------------------------------------------------------
//                        Super "Cool" Pong V 1.10 for PSP
//-------------------------------------------------------------------------------
//
//  FILE NAME   : superpong.c
//
//  DESCRIPTION : The classic story of good versus evil.  Take part in the 
//                ongoing battle that is Pong.
//
//  CREATED     : 2005.07.16
//-------------------------------------------------------------------------------

#include <pspkernel.h> 
#include <pspdebug.h> 
#include <stdlib.h> 
#include <string.h> 
#include "superpong.h"
#include "pg.h"
#include "bitmap.c"

// Global data used by program
Paddle  _paddle;
Ball    _ball;
Ball*   _activeBalls[MAX_BALLS];
char*   _pgVramTop;
int     _gameState;
int     _score;
int     _ballCount;
int     _resBalls;
int     _squareState;
int     _startState;
int     _circleState;
int     _pauseSem;
int     _highScore;
const unsigned short* _bgPtr;

// This call is required by the Development kit
PSP_MODULE_INFO("SUPER_PONG", 0, 1, 1);

  
//------------------------------------------------------------------------------
// Name:     main
// Summary:  Main driver of program.  Initialises variables drawns main 
//           graphics, and enters main program loop
// Inputs:   none
// Outputs:  none
// Globals:  _highScore, _score, _ballCount, _squareState, _startState, 
//           _pgVramTop, _resBalls, _gameState, _bgPtr, _pauseSem
// Returns:  0 on success, non-zero on failure
// Cautions: none
//------------------------------------------------------------------------------
int main(void)
{
  int x;
  // initialize global data
  _highScore    = 0;
  _score        = 0;
  _ballCount    = 0;
  _squareState  = BTN_RELEASED;
  _startState   = BTN_RELEASED;
  _circleState  = BTN_RELEASED;
  _pgVramTop    = (char *)0x04000000;
  _resBalls     = START_RES_BALLS;
  _gameState    = GAME_RUNNING;
  _bgPtr        = _BACKGROUND1;
  _pauseSem     = sceKernelCreateSema("PauseSem", 0, 1, 1, 0);

  // initialize array used to keep track of balls in play
  for (x=0; x < MAX_BALLS; x++)
  { 
    _activeBalls[x] = 0;
  }
  
  // Sets up power callback and exit game callback
  SetupCallbacks();

  // initialize the graphics library
  pgInit();
  pgScreenFrame(2,0);

  // Draw main graphics and supporting text to the screen
  DrawMainStory();
  InitPaddle(&_paddle);  // Initialses the mighty paddle
  DrawMainText();
  DrawStartMenu(COLOR_DARK_BROWN);
  CreateBall(); // Create a ball AND put it in play
 
  // main loop of program.  This loop handles game state changes and
  // refreshes the screen
  while(1)
  {
     DrawPaddle(&_paddle);  // Draws the paddle to the screen
     // only check user input if game is in the running state
     if (_gameState == GAME_RUNNING)
     { 
       CheckUserInput();
     }
     // Handles the game over state
     else if (_gameState == GAME_OVER)
     {
       if (_highScore < _score)
       {
         _highScore = _score;  // update high score if necessary
         DisplayHighScore(_highScore);
       }
       // draws game over graphics and waits for user to continue
       HandleGameOver();  
       _gameState = GAME_RUNNING;
       _score     = 0;
       _resBalls  = START_RES_BALLS;
       DisplayReservedBalls(_resBalls-1);
       DisplayCurrentScore(_score);
       CreateBall();
     }
     // handles the game continue state (I.E. all balls on screen go out of 
     // play, and a new ball prepars to enter play)
     else if (_gameState == GAME_CONTINUE)
     {
       char buffer[50];
       if(_resBalls == 1)  // allways use proper grammer
          sprintf(buffer, "%d Ball Remaining...", _resBalls);
       else
          sprintf(buffer, "%d Balls Remaining...", _resBalls);
       pgPrint2(10,6, COLOR_YELLOW, buffer);
       sceKernelDelayThread(2000000); 
       pgRestoreBackground(SCREEN_START+1, 80, 
                           SCREEN_WIDTH-SCREEN_START-1, 30, _bgPtr);
       _gameState = GAME_RUNNING;
       CreateBall();  // Game on...
     } 
     // sleep here to ensure main loop runs at a reasonable speed      
     sceKernelDelayThread(100); 
     sceDisplaySetFrameBuf(_pgVramTop+FRAMESIZE,LINESIZE,PIXELSIZE,0);
  }
  return 0;
}

//------------------------------------------------------------------------------
// Name:     HandleGameOver
// Summary:  Draws the game over information and waits for user to press Enter
//           to begin a new game
// Inputs:   none
// Outputs:  none
// Globals:  _bgPtr
// Returns:  none
// Cautions: none
//------------------------------------------------------------------------------
void HandleGameOver()
{
  DrawGameOverMenu();
  PressStartToContinue();
  pgRestoreBackground(150, 50, 300, 123, _bgPtr);
}

//------------------------------------------------------------------------------
// Name:     HandlePauseGame
// Summary:  Pauses the game and waits for the user to press Start to un-pause
//           the game
// Inputs:   none
// Outputs:  none
// Globals:  _pgVramTop, _pauseSem, _bgPtr
// Returns:  none
// Cautions: none
//------------------------------------------------------------------------------
void HandlePauseGame()
{
  pgPrint(1,30,COLOR_RED, "Game Paused");
  sceDisplaySetFrameBuf(_pgVramTop+FRAMESIZE,LINESIZE,PIXELSIZE,0);
  
  // Decrament semaphore to 0 (it is 1 by default).  This will force all current 
  // ball threads to block
  sceKernelSignalSema(_pauseSem, -1); 
  _gameState = GAME_PAUSED;  
   
  PressStartToContinue();  // block until user presses 'Start'
  _gameState = GAME_RUNNING;
  sceKernelSignalSema(_pauseSem, 1); // signal semaphore to wait up ball threads
  pgRestoreBackground(1, 240, SCREEN_START-1, 10, _bgPtr);  
}

//------------------------------------------------------------------------------
// Name:     PressStartToContinue
// Summary:  Loops until the user presses the 'Start' button on the game pad
// Inputs:   none
// Outputs:  none
// Globals:  _startState
// Returns:  none
// Cautions: none
//------------------------------------------------------------------------------
void PressStartToContinue()
{
  int loop = 1;
  ctrl_data_t ctrl;  // structure used to read user input
  while (loop)       // loop until user presses start
  {
    sceCtrlReadBufferPositive(&ctrl, 1); // reads user input
    if (ctrl.buttons & CTRL_START) 
    { 
      // This code is used to simulate a digital on/off reading of the button.
      // The start button state begins in the released state, then enters the
      // pressed state, and when the user finally releses the button goes back
      // to the released sate.  
      _startState = BTN_PRESSED;  // set state to pressed
    }
    // if in pressed state, and start button is not currently pressed
    // place button back in released state
    else if ( _startState == BTN_PRESSED)  
    {
      _startState = BTN_RELEASED;
      // break out of loop, user has pressed and released the start button
      loop = 0;  
    }
    // sleep for a while so we are not just spinning our wheels
    sceKernelDelayThread(2000); 
  }
}

//------------------------------------------------------------------------------
// Name:     DrawMainStory
// Summary:  Draws the exciting background story to the screen to captivate and
//           the game player and draw him/her into the exciting worls of Super
//           Pong.  User must press enter to continue.
// Inputs:   none
// Outputs:  none
// Globals:  _pgVramTop
// Returns:  none
// Cautions: none
//------------------------------------------------------------------------------
void DrawMainStory()
{
  pgBitBlt(0,0, 480, 272, 1, _STORY1);
  sceDisplaySetFrameBuf(_pgVramTop+FRAMESIZE,LINESIZE,PIXELSIZE,0);
  PressStartToContinue();
}

//------------------------------------------------------------------------------
// Name:     DrawMainText
// Summary:  Draws startup text to the screen concerning:
//           1.  High Score
//           2.  current Score
//           3. Reserved balls
// Inputs:   none
// Outputs:  none
// Globals:  _bgPtr, _highScore, _score, _resBalls
// Returns:  none
// Cautions: none
//------------------------------------------------------------------------------
void DrawMainText()
{
  pgBitBlt(0,0,480,272,1, _bgPtr);
  pgPrint(1,1,COLOR_RED, "High Score:");
  DisplayHighScore(_highScore);
  DisplayCurrentScore(_score);
  pgPrint(1,10,COLOR_RED,"Reserved");
  pgPrint(1,11,COLOR_RED,"Balls");
  DisplayReservedBalls(_resBalls);
}

//------------------------------------------------------------------------------
// Name:     DrawStartMenu
// Summary:  Draws the start menu to the screen.  This menu displays game title
//           and instructions.  This menu is displayed until user presses
//           'Start'.
// Inputs:   Color used to draw text to screen
// Outputs:  none
// Globals:  _pgVramTop, _bgPtr
// Returns:  none
// Cautions: none
//------------------------------------------------------------------------------
void DrawStartMenu(unsigned short tc)
{
  // get Coordinates to of where data will be written (center of game section
  // of screen)
  int x = SCREEN_START + ((SCREEN_WIDTH-SCREEN_START)/2) - 160;
  int y = (SCREEN_HEIGHT/2)-50;
  pgPrint2(14,6,tc,"SUPER PONG");
  pgPrint2(10,8,tc,"Press Start to play");
  pgPrint(20,19,tc,"Press Start to pause during game play");
  pgPrint(20,21,tc," Press the Square key to activate a");
  pgPrint(20,22,tc,"           reserved ball");        
  pgPrint(20,24,tc,"      Press the Circle key to");
  pgPrint(20,25,tc,"        change backgrounds");
  sceDisplaySetFrameBuf(_pgVramTop+FRAMESIZE,LINESIZE,PIXELSIZE,0);
  
  PressStartToContinue();
  pgRestoreBackground(x, y, 325, 150, _bgPtr);
}

//------------------------------------------------------------------------------
// Name:     DrawGameOverMenu
// Summary:  Draws the start menu to the screen.  This menu displays game title
//           and instructions.  This menu is displayed until user presses
//           'Start'.
// Inputs:   none
// Outputs:  none
// Globals:  _pgVramTop, _bgPtr
// Returns:  none
// Cautions: none
//------------------------------------------------------------------------------
void DrawGameOverMenu()
{
  int x;
  for (x=0; x < 5; x++)
  {
    pgPrint2(15,5,COLOR_DARK_RED,"GAME OVER!");
    sceDisplaySetFrameBuf(_pgVramTop+FRAMESIZE,LINESIZE,PIXELSIZE,0);
    sceKernelDelayThread(1000000); 
    pgRestoreBackground(150, 50, 300, 100, _bgPtr);
    sceKernelDelayThread(1000000); 
    sceDisplaySetFrameBuf(_pgVramTop+FRAMESIZE,LINESIZE,PIXELSIZE,0);
  }
  pgPrint2(14,5,COLOR_DARK_RED,   " GAME OVER");
  pgPrint2(13,7,COLOR_LIGHT_BLUE, " Press Start");
  pgPrint2(13,9,COLOR_LIGHT_BLUE, "to play again");
  sceDisplaySetFrameBuf(_pgVramTop+FRAMESIZE,LINESIZE,PIXELSIZE,0);
}

//------------------------------------------------------------------------------
// Name:     DrawPaddle
// Summary:  Draws the padle to the screen, and ensures the paddle stays within 
//           the bounds of the game area
// Inputs:   Pointer to the paddle structure
// Outputs:  none
// Globals:  none
// Returns:  none
// Cautions: none
//------------------------------------------------------------------------------
void DrawPaddle(Paddle *p)
{
  // Restore background of paddles previous position
  pgRestoreBackground(p->xPosOld, p->yPosOld, p->w, p->h, *(p->bg));
  // ensure paddle stays in the playable game area
  if (p->xPos + p->w >= SCREEN_WIDTH)
  {
    p->xPos = SCREEN_WIDTH - p->w;
  }
  else if (p->xPos <= SCREEN_START)
  {
    p->xPos = SCREEN_START;
  }
  
  p->xPosOld = p->xPos; // mark the paddles current position
  p->yPosOld = p->yPos;
  // draw the mighty paddle
  pgDrawRect(p->xPos, p->yPos, p->w, p->h, p->color);  
}

//------------------------------------------------------------------------------
// Name:     DrawBall
// Summary:  Handles drawinng the ball to the screen.  Has the following duties:
//           1.  Ensure ball stays in game area of screen
//           2.  Alter balls direction if ball is in contact with the paddle
//           3.  Remove ball from play if it goes below the paddle
//           4.  Adjust score when ball hits paddle, and increment number of 
//               reserved balls when score is a multiple of 5
// Inputs:   Pointer to the ball structure
// Outputs:  none
// Globals:  _paddle, _score, _resBalls, _ballCount
// Returns:  1 if ball is in play, 0 otherwise
// Cautions: This functions runs inside a thread.  The global variables it 
//           shares are thread safe, as they only increment/decrament counter
//           variables.  The order these variables are modified by each thread
//           does not matter.
//------------------------------------------------------------------------------
int DrawBall(Ball *b)
{
  int inPlay = 1;
  pgRestoreBackground(b->xPosOld, b->yPosOld, b->w, b->h, *(b->bg));
  // change ball's coordinates
  b->xPos = b->xPos + (b->xDir * b->xSpeed);
  b->yPos = b->yPos + (b->yDir * b->ySpeed);
  //t balls direction as needed
  if (b->xPos + b->w >= SCREEN_WIDTH)  // right side
  {
    b->xPos = SCREEN_WIDTH - b->w;
    b->xDir = b->xDir * -1;
  }
  else if (b->xPos <= SCREEN_START)  // left side
  {
    b->xPos = SCREEN_START;
    b->xDir = b->xDir * -1;
  }
  if (b->yPos <= 0)  // top of screen
  {
    b->yPos = 0;
    b->yDir = b->yDir * -1;
  }
  
  // check to see if ball is in contact with the paddle  
  if (b->yPos+b->h >= _paddle.yPos && 
     (b->yPos+(b->h/2)) <= _paddle.yPos+_paddle.h &&
      b->xPos+b->w >= _paddle.xPos && b->xPos <= _paddle.xPos+_paddle.w)
  {
    b->yPos = _paddle.yPos - b->h;
    b->yDir = b->yDir * -1;
    DisplayCurrentScore(++_score);
    // add new ball every 5 points
    if (_score % 5 == 0 && (_resBalls+_ballCount) < MAX_BALLS)
    {
      DisplayReservedBalls(++_resBalls);
    }
  }
  // check to see if ball has missed the paddle
  else if ( b->yPos >= SCREEN_HEIGHT)
  {
    _ballCount--;  // decrement ball count (number of balls currently in play)
    if (_ballCount == 0 )
    {
      // change state depending on number of balls in reserve
      if (_resBalls > 0)
        _gameState = GAME_CONTINUE;
      else
        _gameState = GAME_OVER;
    }
    inPlay = 0;
  }
  
  if (inPlay)  // update ball image if it is in play
  {
    b->xPosOld = b->xPos;
    b->yPosOld = b->yPos;
    pgDrawTransparentImage(b->xPos, b->yPos, b->w, b->h, b->image);  
  }
  return(inPlay);
}

//------------------------------------------------------------------------------
// Name:     DisplayCurrentScore
// Summary:  Draws score to screen
// Inputs:   current score
// Outputs:  none
// Globals:  _bgPtr
// Returns:  none
// Cautions: none
//------------------------------------------------------------------------------
void DisplayCurrentScore(int s)
{
  static char scoreStr[20];
  sprintf(scoreStr, "SCORE: %d", s);
  pgRestoreBackground(1, 40, SCREEN_START-1, 10, _bgPtr);  
  pgPrint(1,5,COLOR_RED, scoreStr);
}

//------------------------------------------------------------------------------
// Name:     DisplayReservedBalls
// Summary:  Draws number of reserved balls to screen
// Inputs:   Number of reserved balls
// Outputs:  none
// Globals:  _bgPtr
// Returns:  none
// Cautions: none
//------------------------------------------------------------------------------
void DisplayReservedBalls(int rBalls)
{
  static char rb[10]; 
  pgRestoreBackground(80, 77, SCREEN_START-81, 20, _bgPtr);  
  sprintf(rb, "%d", rBalls);
  pgPrint2(5,5,COLOR_RED, rb);
}

//------------------------------------------------------------------------------
// Name:     DisplayHighScore
// Summary:  Draws high score to screen
// Inputs:   High Score
// Outputs:  none
// Globals:  _bgPtr
// Returns:  none
// Cautions: none
//------------------------------------------------------------------------------
void DisplayHighScore(int hs)
{
  static char hScoreStr[20];
  pgRestoreBackground(1, 15, SCREEN_START-1, 10, _bgPtr);  
  sprintf(hScoreStr, "%d", hs);
  pgPrint(1,2,COLOR_RED, hScoreStr);
} 
 
//------------------------------------------------------------------------------
// Name:     CreateBall
// Summary:  Creates thread to handle ball, calls function to Initialize a 
//           new ball, and starts the new ball thread
// Inputs:   none
// Outputs:  none
// Globals:  _ballCount, _activeBalls, _resBalls
// Returns:  none
// Cautions: This function must be thread safe since it modifies the 
//           _activeBalls array.  Since this function is only called in the 
//           main thread it is thread safe by default.  If balls are ever 
//           allowed to be created outside the main thread, this function must
//           be semaphore protected.
//------------------------------------------------------------------------------
void CreateBall()
{
  int tId;
    
  if (_resBalls > 0)
  {
    char buffer[30];  // create a semi unique thread name
    sprintf(buffer, "Ball_Thread%d", _ballCount);
    // create the thread
    tId = sceKernelCreateThread(buffer, BallThread, 0x18, 0x10000, 0, 0); 
    if(tId >= 0)  
    {
      int ret;
      Ball *b;  // create ball pointer and call function to initialize ball
      ret = InitBall(&b);
      if (ret == 0)
      {
        // start thread, pass newly created ball in as paramate
        ret = sceKernelStartThread(tId, sizeof(Ball), (void*) b );
        if (ret == 0)
        {
          _activeBalls[b->ballId] = b;  // store ball in ball tracking array
          _ballCount++;                 // update number of balls in play
          // update/display reserved ball count
          DisplayReservedBalls(--_resBalls);  
        }
        else
        { // free allocated memory if thread did not start correctly
          free(b);
        }
      }
    }
  }
}

//------------------------------------------------------------------------------
// Name:     GetBallId
// Summary:  Examines the array of balls currently in play.  Finds an open spot
//           if available, and returns the id/open index to user
// Inputs:   none
// Outputs:  none
// Globals:  _activeBalls
// Returns:  Ball Id >= 0 if a slot is open
//           Ball Id < 0  if no slots are open
// Cautions: none
//------------------------------------------------------------------------------
int GetBallId()
{
  int x;
  for (x=0; x < MAX_BALLS; x++)
  { 
    if (_activeBalls[x] == 0)
    {
      return(x); 
    }
  }
  return(-1);    
}

//------------------------------------------------------------------------------
// Name:     BallThread
// Summary:  Thread that controls the placement and lifespan of a ball in play
// Inputs:   1.  Size of ball structure (SCE required paramater)
//           2.  Pointer to the ball structure
// Outputs:  none
// Globals:  _gameState, _pauseSem, _activeBalls
// Returns:  none
// Cautions: This function runs as a thread and its paramaters were provided
//           when sceKernelStartThread was called.  The order of the paramaters
//           in this function's prototype is specified by the SCE API.  As a 
//           note, the size paramater seems useless, but the API will allways
//           pass it to this function so be aware that it is allways present.
//------------------------------------------------------------------------------
void BallThread(int size, Ball *b)
{
  int inPlay = 1;
  
  // loop until game ends or until ball is no longer in play
  while ( _gameState != GAME_OVER && inPlay)
  {
    if(_gameState == GAME_PAUSED)  // if pause state is entered
    {
      // block on semaphore
      sceKernelWaitSema(_pauseSem, 1, 0); 
      // once awake, signal semaphore so next in line wakes up 
      sceKernelSignalSema(_pauseSem, 1);  
    }
    inPlay = DrawBall(b);
    sceKernelDelayThread(10000); 
  }
  _activeBalls[b->ballId] = 0;  // remove this ball from array of active balls
  free(b);                      // free memory allocated to this ball
  sceKernelExitThread(0);
}

//------------------------------------------------------------------------------
// Name:     InitBall
// Summary:  Allocates memory and created default values for a new ball
//           structure
// Inputs:   none
// Outputs:  pointer to a ball pointer.  This pointer will be set to the ball
//           structure created within this function.
// Globals:  _bgPtr
// Returns:  0 on success, > 0 if ball could not be created
// Cautions: none
//------------------------------------------------------------------------------
int InitBall(Ball **bAdr)
{
  int bId;
  int ret = 0;
  Ball *b = malloc(sizeof(Ball));
  bId     = GetBallId();
  *bAdr   = b;
 
  if (b != 0 && bId >= 0)
  {
    // set ball starting position, style, speed, ect. with quasi-random values
    int bType  = RandomNumberGen(1, 5);
    b->w       = 20;
    b->xPos    = RandomNumberGen(SCREEN_START, SCREEN_WIDTH-b->w);
    b->h       = 20;
    b->yPos    = RandomNumberGen(0, SCREEN_HEIGHT - 150);
    b->xPosOld = 250;  // any place in the playable game area will be ok
    b->yPosOld = 200;
    b->xSpeed  = RandomNumberGen(1, 4);
    b->ballId  = bId;
    // set ball's x & y  direction
    if (b->xPos % 2 == 0)
    {
      b->xDir = -1;
    }
    else
    {
      b->xDir = 1;
    }
    if ( b->yPos > SCREEN_HEIGHT / 2 || (b->yPos%2) == 0)
    {
      b->yDir = -1;
    }
    else
    {
      b->yDir = 1;
    }
    b->ySpeed = RandomNumberGen(1, 2);
    
    // sets the ball style
    if ( bType == 1)
      b->image = _BASKETBALL;
    else if ( bType == 2)
      b->image = _FLOWER_YELLOW;
    else if ( bType == 3)
      b->image = _SMILEY_FACE;
    else if ( bType == 4)
      b->image = _SPHERE_BLUE;
    else if ( bType == 5)
      b->image = _SPHERE_RED;
    else
      b->image = _SPHERE_BLUE;
    
    b->bg = &_bgPtr;
    ret = 0;  // go for launch
  }
  else
  {
    if (b)  // if ball was allocated, free memory
    {
      free(b);
      ret = 2; // return of 2 indicates no ball ids were free
    }
    else  //else ball not allocated succesfully, set return value to 1
    {
      ret = 1;
    }
    *bAdr = 0; 
  }
  return(ret);
}

//------------------------------------------------------------------------------
// Name:     InitPaddle
// Summary:  Set the default values for the paddle structure
// Inputs:   pointer to the paddle structure
// Outputs:  none
// Globals:  _bgPtr
// Returns:  none
// Cautions: none
//------------------------------------------------------------------------------
void InitPaddle(Paddle *p)
{
  p->h       = 10;
  p->w       = 40;
  p->xPos    = 180;
  p->yPos    = 250;
  p->xPosOld = 250;
  p->yPosOld = 250;
  p->speed   = 10;
  p->dir     = 1;
  p->color   = COLOR_LIGHT_BLUE;
  p->bg      = &_bgPtr;
}

//------------------------------------------------------------------------------
// Name:     RandomNumberGen
// Summary:  creates a random number in the specified range
// Inputs:   1.  Lower limit for number (inclusive)
//           2.  Upper limit for number (inclusive)
// Outputs:  none
// Globals:  _bgPtr
// Returns:  none
// Cautions: none
//------------------------------------------------------------------------------
int RandomNumberGen(int lower, int upper)
{
  int rNum  = rand();
  int range = upper - lower + 1;
  rNum      = (rNum % range) + lower;
  return(rNum);
}

//------------------------------------------------------------------------------
// Name:     CheckUserInput
// Summary:  Used by main loop to determine what keys user has pressed
// Inputs:   none
// Outputs:  none
// Globals:  _squareState, _circleState, _startState, _bgPtr, _paddle, 
//           _pgVramTop
// Returns:  none
// Cautions: none
//------------------------------------------------------------------------------
void CheckUserInput()
{
  ctrl_data_t ctrl;

  sceCtrlReadBufferPositive(&ctrl, 1); 
  if (ctrl.buttons & CTRL_SQUARE) // if square is pressed, attempt to put a new
  {                               // ball in play
    // This function will be called many times during the split second in which
    // the user pressed a button. This logic ensures a new ball is drawn only
    // when a button goes from a released state to the pressed state
    if (_squareState == BTN_RELEASED)
    {
      CreateBall();
      _squareState = BTN_PRESSED;
    }
  }
  // if button is in pressed state, but the user is not currently pressing
  // the button, place the button back in the released state
  else if ( _squareState == BTN_PRESSED)
  {
    _squareState = BTN_RELEASED;
  }
  
  // same logic as above is used to denote when start is pressed/released
  if (ctrl.buttons & CTRL_START) 
  {
    if (_startState == BTN_RELEASED)
    {  
      _startState = BTN_PRESSED;
    }
  }
  else if ( _startState == BTN_PRESSED)
  { 
    // enter paused mode when user releases the start button
    // NOTE: _startState must be set back to released before the pause
    // function is called, as the pause function will aslo use this variable
    _startState = BTN_RELEASED;
    HandlePauseGame();
  }
  
  // same logic used to track pressed/released sate
  if (ctrl.buttons & CTRL_CIRCLE) 
  {
    // change background as soon as user presses button
    if (_circleState == BTN_RELEASED)
    {  
      _circleState = BTN_PRESSED;
      if (_bgPtr == _BACKGROUND1) // alter between 2 possible BGs
      {
        _bgPtr = _BACKGROUND2;
      }
      else
      {
        _bgPtr = _BACKGROUND1;
      }
      pgBitBlt(0,0, 480, 272, 1, _bgPtr);  // draw new background
      DrawMainText();  // re-draw game status info
      sceDisplaySetFrameBuf(_pgVramTop+FRAMESIZE,LINESIZE,PIXELSIZE,0);
    }
  }
  else if ( _circleState == BTN_PRESSED)
  { 
    _circleState = BTN_RELEASED;
  }
  
  // paddle controls
  if (ctrl.buttons & CTRL_DOWN) 
  { 
    // not used
  } 
  else if (ctrl.buttons & CTRL_LEFT)   // move paddle left
  { 
    _paddle.xPos -= _paddle.speed;
  } 
  else if (ctrl.buttons & CTRL_RIGHT) // move paddle right
  { 
    _paddle.xPos += _paddle.speed;
  }
  else if (ctrl.buttons & CTRL_UP) 
  {
    // not used
  }
}

//------------------------------------------------------------------------------
// Name:     SetupCallbacks
// Summary:  Creates thread that will run power and main menu callbacks
// Inputs:   none
// Outputs:  none
// Globals:  none
// Returns:  thread id
// Cautions: none
//------------------------------------------------------------------------------
int SetupCallbacks()
{
  int thid = 0;

  thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, 0, 0);
  if(thid >= 0)
  {
    sceKernelStartThread(thid, 0, 0);
  }

  return thid;
}

//------------------------------------------------------------------------------
// Name:     ExitCallback
// Summary:  Function called when user chooses to return to main menu.  
//           Frees any memory allocated for balls currently in play.  Frees
//           memory used for pause semaphore
// Inputs:   none
// Outputs:  none
// Globals:  none
// Returns:  0 on success
// Cautions: none
//------------------------------------------------------------------------------
int ExitCallback()
{
  int x;
  // free memory used by active balls
  for (x=0; x < MAX_BALLS; x++)
  { 
    if (_activeBalls[x] != 0)
      free(_activeBalls[x]);
  }
  // destroy the pause semaphore
  if (_pauseSem) sceKernelDeleteSema(_pauseSem);
  sceKernelExitGame();
  return 0;
}

//------------------------------------------------------------------------------
// Name:     CallbackThread
// Summary:  Creates and registers the Power and Game Exit callbacks
// Inputs:   none
// Outputs:  none
// Globals:  none
// Returns:  none
// Cautions: none
//------------------------------------------------------------------------------
void CallbackThread(void *arg)
{
  int cbid;
  cbid = sceKernelCreateCallback("Exit Callback", ExitCallback, NULL);
  sceKernelRegisterExitCallback(cbid);
  cbid = sceKernelCreateCallback("Power Callback", PowerCallback, NULL); 
  scePowerRegisterCallback(0, cbid); 
  sceKernelSleepThreadCB();
}

//------------------------------------------------------------------------------
// Name:     CallbackThread
// Summary:  Places the game in the paused state should the user or PSP 
//           decide to power down while the game is running
// Inputs:   1.  unknown
//           2.  Power Flags (as defined in superpong.h)
// Outputs:  none
// Globals:  none
// Returns:  none
// Cautions: The powerflags are defined in superpong.h, but later releases of
//           the PSP DevKit will probably include these values by default.
//           The power callback must be re-registered each time it is called.
//           Otherwise, the callback will only execute 1 time.
//------------------------------------------------------------------------------
void PowerCallback(int unknown, int pwrflags) 
{ 
  int cbid;
  if (pwrflags & POWER_CB_POWER || pwrflags & POWER_CB_SUSPEND ||
      pwrflags & POWER_CB_EXT)
  {
    // put game in paused state if game is running
    if ( _gameState == GAME_RUNNING)
    {
      HandlePauseGame();
    }
    // if game is in continue state, wait until it enters the running state
    // before pausing.  Game is only in continue state for about 1 second, 
    // and it cannot be paused from this sate
    else if (_gameState == GAME_CONTINUE)
    {
      while ( _gameState == GAME_CONTINUE )
      {
        sceKernelDelayThread(1000); 
      }
      HandlePauseGame();
    }
  }
  
  // re-register power callback so it executes again the next time
  // a power event occurs
  cbid = sceKernelCreateCallback("Power Callback", PowerCallback, NULL); 
  scePowerRegisterCallback(0, cbid); 
}
