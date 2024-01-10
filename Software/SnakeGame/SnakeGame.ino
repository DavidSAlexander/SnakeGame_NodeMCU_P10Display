
/*******************************************************************************************************
 *  [FILE NAME]   :      <SnakeGame.ino>                                                               *
 *  [AUTHOR]      :      <David S. Alexander>                                                          *
 *  [DATE CREATED]:      <Jan 9, 2024>                                                                 *
 *  [Description} :      <Simple Snake game using NodeMCU Microcontroller + P10 Display>               *
 *******************************************************************************************************/


/*******************************************************************************************************
 *                                           Includes                                                  *
 *******************************************************************************************************/
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <RoninDMD.h>
#include <fonts/Arial_Black_16.h>

/*******************************************************************************************************
 *                                       Macros Definitions                                            *
 *******************************************************************************************************/
// WiFi credentials
#define WIFISSID     "David"
#define WIFIPASSWORD "your-password"

// TCP HOST & PORT "Configured as static IP in the router settings"
#define HOSTIP       "192.168.1.100"
#define HOSTPORT     8080

#define MAX_RETRIES  10

// Number of P10 Display boards
#define WIDTH        3
#define HEIGHT       1
#define FONT Arial_Black_16

// Controler Directions
#define UP       4
#define DOWN     3
#define LEFT     2
#define RIGHT    1

// Pixels states
#define PIXELON  1
#define PIXELOFF 0

/*******************************************************************************************************
 *                                          Variables                                                  *
 *******************************************************************************************************/
uint8_t xPosition = 0;
uint8_t yPosition = 0;

// Snake initial x-coordinates
uint8_t x[512] = { 10, 9, 8, 7 };

// Snake initial y-coordinates
uint8_t y[512] = { 10, 10, 10, 10 };

uint8_t SnakeDirection = 0;
uint8_t lastDirection  = 0;

//snake initial length
uint16_t SnakeLength = 4;
boolean Food = false;
// Snake Food coordinates
uint8_t FoodX = 0;
uint8_t FoodY = 0;

unsigned long previousTimer = 0;
uint16_t score = 0;

// Lower value makes the snake faster
uint16_t DifficultyLevel = 150;

WiFiClient client;
uint8_t retryCount = 0;
RoninDMD P10(WIDTH, HEIGHT);

/*******************************************************************************************************
 *                                   Functions Declaration                                             *
 *******************************************************************************************************/
void GenerateFood();
void SetSnakeLength();
void NewGame();
void GameOver();
void GetScore();

void WiFi_Init();
bool WiFi_Connect();
bool WiFi_Status();

void TCP_Init();
void TCP_Disconnect();
bool TCP_Connect();
bool TCP_Client_Status();
bool TCP_Incoming_Message();

/*******************************************************************************************************
 *                                          Setup Function                                             *
 *******************************************************************************************************/
void setup()
{
  WiFi_Init();
  TCP_Init();
  WiFi_Connect();
  TCP_Connect();

  P10.begin();            // Begin the display & font
  P10.setBrightness(20);  // Set the brightness
  P10.setFont(FONT);
  P10.clear(0);
  P10.drawRect(0, 0, (WIDTH * 32 - 1), (HEIGHT * 16 - 1)); // Set the display frame
  randomSeed(analogRead(A0));
}

/*******************************************************************************************************
 *                                       Main loop function                                            *
 *******************************************************************************************************/

void loop()
{
  // if Connection lost, re-connect
  if (!WiFi_Status() || !TCP_Client_Status())
  {
    setup();
  }
  else
  {
    // New message received
    if (TCP_Incoming_Message())
    {
      String message = client.readStringUntil('\n');
      // Ignore wrong, repeated and forbidden directions
      if      (message.indexOf("Right") != -1 && lastDirection != LEFT)   SnakeDirection = RIGHT;
      else if (message.indexOf("Left")  != -1 && lastDirection != RIGHT)  SnakeDirection = LEFT;
      else if (message.indexOf("Up")    != -1 && lastDirection != DOWN)   SnakeDirection = UP;
      else if (message.indexOf("Down")  != -1 && lastDirection != UP)     SnakeDirection = DOWN;
    }
    P10.loop();  // Run DMD loop

    // Clear last pixel
    P10.setPixel(x[SnakeLength - 1], y[SnakeLength - 1], PIXELOFF);
    if (millis() - previousTimer >= DifficultyLevel)
    {
      // Snake moves right
      if (SnakeDirection == RIGHT)
      {
        // check if the snake doesn't exceed the right edge
        if (x[0] == (WIDTH * 32 - 1))
        {
          GameOver();
          return;
        }
        SetSnakeLength();
        x[0]++;
      }

      // Snake moves left
      if (SnakeDirection == LEFT)
      {
        // Check if the snake doesn't exceed the left edge
        if (x[0] == 1)
        {
          GameOver();
          return;
        }
        SetSnakeLength();
        x[0]--;
      }

      // Snake moves down
      if (SnakeDirection == DOWN)
      {
        // Check if the snake doesn't exceed the bottom edge
        if (y[0] == (HEIGHT * 16 - 1))
        {
          GameOver();
          return;
        }
        SetSnakeLength();
        y[0]++;
      }

      // Snake moves up
      if (SnakeDirection == UP)
      {
        // Check if the snake doesn't exceed the top edge
        if (y[0] == 1)
        {
          GameOver();
          return;
        }
        SetSnakeLength();
        y[0]--;
      }
      lastDirection = SnakeDirection;
      previousTimer = millis();
    }

    // Draw the snake
    for (uint16_ti = 0; i < SnakeLength; i++)
    {
      P10.loop();  // Run DMD loop
      P10.setPixel(x[i], y[i], PIXELON);
    }

    // Check if the snake eats food
    if (FoodX == x[0] && FoodY == y[0])
    {
      SnakeLength++;
      Food = false;
    }

    // Check if Food exists
    if (Food == false) GenerateFood();

    // Check if the snake doesn't eat itself
    for (uint16_ti = 1; i < SnakeLength; i++)
    {
      if (x[0] == x[i] && y[0] == y[i]) GameOver();
    }
  }
}

/*******************************************************************************************************
 *                                    Functions Definition                                             *
 *******************************************************************************************************/

// Set the new coordinates
void SetSnakeLength()
{
  for (uint16_ti = SnakeLength; i > 0; i--)
  {
    // shift every x-coordinate in the array
    x[i] = x[i - 1];
  }

  for (uint16_ti = SnakeLength; i > 0; i--)
  {
    // shift every y-coordinate in the array
    y[i] = y[i - 1];
  }
}

// generate Food in random pixels
void GenerateFood()
{
  // Draw the food on random pixel inside the frame
  FoodX = random(1, WIDTH * 32 - 1);
  FoodY = random(1, HEIGHT * 16 - 1);

  // Check if food isn't generated on the snake body, otherwise try again.
  for (uint16_ti = 0; i < SnakeLength; i++)
  {
    if (FoodX == x[i] && FoodY == y[i])
    {
      GenerateFood();
      return;
    }
  }
  // Draw the new generated food
  P10.setPixel(FoodX, FoodY, PIXELON);
  Food = true;
}

// Game over screen
void GameOver()
{
  bool State = true;
  // Blinking effect
  for (uint16_tj = 0; j < 10; j++)
  {
    for (uint16_ti = 0; i < SnakeLength; i++)
    {
      P10.loop();  // Run DMD loop
      P10.setPixel(x[i], y[i], !State);
      P10.setPixel(FoodX, FoodY, !State);
    }
    State = !State;
    delay(100);
  }
  P10.clear(0);
  for (uint16_ti = 0; i < 1500; i++)
  {
    // Write "YOU LOST!" over the display
    P10.loop();
    P10.drawText(0, 0, "YOU LOST !"); // P10.drawText(position x , position y, String type text);
    delay(1);
  }
  GetScore();
  NewGame();
}

// Display your score
void GetScore()
{
  P10.clear(0);
  for (uint16_ti = 0; i < 1500; i++)
  {
    // Write "SCORE: " over the display
    P10.loop();
    P10.drawText(0, 0, " Score: " + String(SnakeLength - 4));   // P10.drawText(position x , position y, String type text);
    delay(1);
  }
}

// Reset all variables to start a new game
void NewGame()
{
  P10.clear(0);
  P10.drawRect(0, 0, (WIDTH * 32 - 1), (HEIGHT * 16 - 1));
  x[0] = 10;
  x[1] = 9;
  x[2] = 8;
  x[3] = 7;
  y[0] = 7;
  y[1] = 7;
  y[2] = 7;
  y[3] = 7;
  SnakeDirection = 0;
  lastDirection = 0;
  SnakeLength = 4;
  Food = false;

  // Different sequences of random numbers each time program runs.
  randomSeed(analogRead(A0));
}

// Initialize WiFi
void WiFi_Init()
{
  delay(10);
  WiFi.begin(WIFISSID, WIFIPASSWORD);
}

// Connect to WiFi
bool WiFi_Connect()
{
  retryCount = 0;
  // Keep trying for MAX_RETRIES times
  while (WiFi.status() != WL_CONNECTED && retryCount < MAX_RETRIES)
  {
    delay(1000);
    retryCount++;
  }
  // Return the WiFi connection status
  return WiFi.status() == WL_CONNECTED;
}

// Return the WiFi connection status
bool WiFi_Status()
{
  return WiFi.status() == WL_CONNECTED;
}

// Initialize the TCP Local Server (Client)
void TCP_Init()
{
  if (client.connected())
  {
    client.stop();
  }
}

// Disconnect from the TCP server
void TCP_Disconnect()
{
  if (WiFi_Status())
  {
    client.stop();
  }
}

// Connect to the TCP server
bool TCP_Connect()
{
  if (WiFi_Status())
  {
    retryCount = 0;
    // Keep trying for MAX_RETRIES times
    while (!client.connect(HOSTIP, HOSTPORT) && retryCount < MAX_RETRIES)
    {
      delay(1000);
      retryCount++;
    }
  }
  return client.connected();
}

// Return the TCP client connection status
bool TCP_Client_Status()
{
  return client.connected();
}

// Check for new message
bool TCP_Incoming_Message()
{
  if (TCP_Client_Status())
  {
    return client.available();
  }
  return false;
}