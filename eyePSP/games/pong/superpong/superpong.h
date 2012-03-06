// Constant values
#define SCREEN_START               138
#define MAX_BALLS                   20
#define GAME_RUNNING                 1
#define GAME_OVER                    2
#define GAME_CONTINUE                3
#define GAME_PAUSED                  4
#define START_RES_BALLS              5
#define BTN_PRESSED                  1
#define BTN_RELEASED                 2

// Power callbacks as read from some newsgroup
#define POWER_CB_POWER      0x80000000     
#define POWER_CB_HOLDON     0x40000000    
#define POWER_CB_STANDBY    0x00080000   
#define POWER_CB_RESCOMP    0x00040000   
#define POWER_CB_RESUME     0x00020000    
#define POWER_CB_SUSPEND    0x00010000   
#define POWER_CB_EXT        0x00001000       
#define POWER_CB_BATLOW     0x00000100    
#define POWER_CB_BATTERY    0x00000080   
#define POWER_CB_BATTPOWER  0x0000007F 

// Colors used for various graphics and text
#define COLOR_GREEN             0x03E0
#define COLOR_RED               0x001F
#define COLOR_BLUE              0x7C00
#define COLOR_YELLOW            0x03FF
#define COLOR_BROWN             0x0110
#define COLOR_DARK_RED          0x0010
#define COLOR_DARK_BROWN        0x00AB
#define COLOR_LIGHT_BLUE        0x7F00

// Structures 

// Structure used to draw / control paddle
typedef struct PADDLE_STRUCT
{
  int h;
  int w;
  int xPos;
  int yPos;
  int xPosOld;
  int yPosOld;
  int dir;
  int speed;
  const unsigned short **bg;
  unsigned int color;
} Paddle;

// Structure used to draw / control a ball
typedef struct BALL_STRUCT
{
  int h;
  int w;
  int xPos;
  int yPos;
  int xPosOld;
  int yPosOld;
  int xDir;
  int yDir;
  int xSpeed;
  int ySpeed;
  int ballId;
  const unsigned short *image;
  const unsigned short **bg;
} Ball;


// Function Prototypes
void DisplayReservedBalls(int s);
void DisplayCurrentScore(int s);
void DisplayHighScore(int hs);
void DrawPaddle(Paddle *p);
int  DrawBall(Ball *p);
void DrawMainStory();
void DrawMainText();
void DrawGameOverMenu();
void DrawStartMenu(unsigned short tc);
void HandleGameOver();
void CheckUserInput();
void PressStartToContinue();
void InitPaddle(Paddle *p);
void CreateBall();
int  InitBall(Ball **b);
void BallThread(int size, Ball *b);
int  GetBallId();
int  RandomNumberGen(int, int);
int  SetupCallbacks();
int  ExitCallback();
void PowerCallback(int unknown, int pwrflags);
void CallbackThread(void *arg);
