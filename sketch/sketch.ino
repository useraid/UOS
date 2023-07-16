// External Libraries
#include <ESP8266WiFi.h>

// Local Headers
#include "menuAssets.h"
#include "display.h"
#include "flappyBirdGame.h"
#include "pongGame.h"
#include "internetClock.h"
#include "counter.h"
#include "timer.h"
#include "stopwatch.h"
#include "resourceMonitor.h"
#include "eReader.h"
#include "OTA.h"
#include "packetMonitor.h"
#include "wifiScanner.h"
#include "wlanManager.h"
#include "battery.h"

// External ESP8266 NON RTOS SDK
extern "C" {
  #include "user_interface.h"
  #include "gpio.h"   
}

// Debugging Mode 
#define DEBUG true
#define Serial if(DEBUG)Serial

// Display Init
Adafruit_SSD1306 display(128, 64, &Wire, -1);
// UDP Wifi Init
WiFiUDP ntpUDP;
// NTP Time Client Init
NTPClient timeClient(ntpUDP, "time.nist.gov", 19800, 60000); // 5.5 hour offset = 19800 secs
// eReader dependency Init
MenuState menuState = FILE_SELECTION;
// Webserver for OTA Init
ESP8266WebServer httpServer(80);
// ESP Webserver based OTA updator Init
ESP8266HTTPUpdateServer httpUpdater;
// WiFi Multi Init
ESP8266WiFiMulti wifiMulti;
// Captive Portal Based WiFi Login Init
WiFiManager wifiManager;

// Button Pin Declaration
#define BUTTON_UP_PIN 0
#define BUTTON_SELECT_PIN 12
#define BUTTON_DOWN_PIN 14
#define BUTTON_BACK_PIN 3

void setup() {
  // Debug Serial
  Serial.begin(115200);
  // Display init
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  Serial.println("[DISPLAY] Display Initialized");
  // Buttons Init
  pinMode(BUTTON_UP_PIN, INPUT_PULLUP); // up button
  pinMode(BUTTON_SELECT_PIN, INPUT_PULLUP); // select button
  pinMode(BUTTON_DOWN_PIN, INPUT_PULLUP); // down button
  pinMode(BUTTON_BACK_PIN, INPUT_PULLUP); // back button
  // Setup Multiple Wifi Access Points Credentials
  configureAccessPoints(); 
  // Put Wifi Modem to sleep on startup
  WiFi.forceSleepBegin();

}

void loop() {
  // UP DOWN SELECT Handler for main menu
  menuButtonHandler();
  // Global BACK Button Handler
  globalBackHandler();  
  // Set Items on menu
  itemSelection();
  // Render Screens
  if (current_screen == 0) { // MENU SCREEN
    renderMenu();
  }
  else if (current_screen == 1) {
    programHandler();
  }
}

void itemSelection(){
  // set correct values for the previous and next items
  item_sel_previous = item_selected - 1;
  if (item_sel_previous < 0) {item_sel_previous = NUM_ITEMS - 1;} // previous item would be below first = make it the last
  item_sel_next = item_selected + 1;  
  if (item_sel_next >= NUM_ITEMS) {item_sel_next = 0;} // next item would be after last = make it the first
}

void menuButtonHandler(){
  if (current_screen == 0) { // MENU SCREEN

    if ((digitalRead(BUTTON_UP_PIN) == LOW) && (button_up_clicked == 0)) { // up button clicked
      item_selected = item_selected - 1; // select previous item
      button_up_clicked = 1; 
      Serial.println("[BUTTON] Up");
      if (item_selected < 0) { // first item was selected
        item_selected = NUM_ITEMS-1;
      }
    }    
    else if ((digitalRead(BUTTON_DOWN_PIN) == LOW) && (button_down_clicked == 0)) { // down button clicked
      item_selected = item_selected + 1; // select next item
      button_down_clicked = 1;
      Serial.println("[BUTTON] Down");
      if (item_selected >= NUM_ITEMS) { // last item 
        item_selected = 0;
      }
    } 

    if ((digitalRead(BUTTON_UP_PIN) == HIGH) && (button_up_clicked == 1)) { // unclick 
      button_up_clicked = 0;
    }
    if ((digitalRead(BUTTON_DOWN_PIN) == HIGH) && (button_down_clicked == 1)) { // unclick
      button_down_clicked = 0;
    }
    if ((digitalRead(BUTTON_SELECT_PIN) == LOW) && (button_select_clicked == 0)) { 
      button_select_clicked = 1;
      Serial.println("[BUTTON] Select");
      if (current_screen == 0) {
        delay(200);
        current_screen = 1;
      }
      else if (current_screen == 1) {current_screen = 2;} 
      else {current_screen = 0;} 
  
    }
    if ((digitalRead(BUTTON_SELECT_PIN) == HIGH) && (button_select_clicked == 1)) { // unclick 
      button_select_clicked = 0;
    }

  }
}

void globalBackHandler(){

  if ((digitalRead(BUTTON_BACK_PIN) == LOW) && (button_back_clicked == 0)) {
    button_back_clicked = 1;
    Serial.println("[BUTTON] Back"); 
    if (current_screen == 0){
      light_sleep();
      delay(100);
    }
    else if (current_screen == 1) {
      inProgram = false;
      if (item_selected == 0){ stopPong(); }      
      else if (item_selected == 4){ stopFlappyBird(); }
      else if (item_selected == 3){ 
        WifiTurnOFFSTA();
        display.clearDisplay();
        }
      else if (item_selected == 10){ 
        WifiTurnOFFAP();
        display.clearDisplay();
        }
      else if (item_selected == 11){ 
        WifiPromiscuousOFF();
        stopPacMon();
        display.clearDisplay();
        }
      else if (item_selected == 12){ 
        WifiTurnOFFSTA();
        display.clearDisplay();
        }
      else if (item_selected == 13){ 
        WifiTurnOFFSTA();
        display.clearDisplay();
        }
      else if (item_selected == 16){ 
        WifiTurnOFFSTA();
        display.clearDisplay();
        }
      else if (item_selected == 5){ display.clearDisplay(); }
      current_screen = 0;
      }
    else if (current_screen == 2) {current_screen = 1;} 
    else {current_screen = 0;}
  }
  if ((digitalRead(BUTTON_BACK_PIN) == HIGH) && (button_back_clicked == 1)) { // unclick 
    button_back_clicked = 0;
  }
}

void renderMenu(){
  // selection box 
      display.clearDisplay();
      display.drawBitmap(0, 22, bitmap_item_sel_outline, 128, 21, WHITE);
      // previous item in menu
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(12, 8);
      display.println(menu_items[item_sel_previous]);
      // current item in menu
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(12, 8+20+2);
      display.println(menu_items[item_selected]);       
      // next item in menu
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(12, 8+20+20+2+2);
      display.println(menu_items[item_sel_next]);       
      // scrollbar background
      display.drawBitmap(128-8, 0, bitmap_scrollbar_background, 8, 64, WHITE);
      // scrollbar handle
      display.fillRect(125, 64/NUM_ITEMS * item_selected, 3, 64/NUM_ITEMS, WHITE);            
      display.display();
}

void programHandler(){
  if (item_selected == 0){
    inProgram = true;
    Serial.println("[APP] Pong Started");
    while (inProgram){
      display.clearDisplay();
      setupPong();
      mainPong();
      globalBackHandler();
    }
    Serial.println("[APP] Pong Exited");

  }
  else if (item_selected == 1) {
    inProgram = true;
    Serial.println("[APP] eReader Started");
    setupEReader();
    while (inProgram){
      display.clearDisplay();
      mainEReader();
      globalBackHandler();
    }
    Serial.println("[APP] eReader Exited");
  
  }
  else if (item_selected == 2 ) {
    inProgram = true;
    Serial.println("[APP] Resource Monitor Started");
    while (inProgram){
      display.clearDisplay();
      mainResMon();
      globalBackHandler();
    }
    Serial.println("[APP] Resource Monitor Exited");
  
  }
  else if (item_selected == 3) {
    inProgram = true;
    Serial.println("[APP] Internet Clock Started");
    timeClient.begin();
    WifiTurnONSTA();
    while (inProgram){
      display.clearDisplay();
      timeFetch();
      globalBackHandler();
    }
    Serial.println("[APP] Internet Clock Exited");
  
  }
  else if (item_selected == 4) {
    inProgram = true;
    Serial.println("[APP] Flappy Bird Started");
    while (inProgram){
      display.clearDisplay();
      mainFlappyBird();
      globalBackHandler();
    }
    Serial.println("[APP] Flappy Bird Exited");
  
}      
  else if (item_selected == 5) {
    inProgram = true;
    Serial.println("[APP] Counter Started");
    setupCounterScreen();
    while (inProgram){
      display.clearDisplay();
      mainCounter();
      globalBackHandler();
    }
    Serial.println("[APP] Counter Exited");

  
}  
  else if (item_selected == 6) {
    Serial.println("[APP] Light/Dark Mode");
    if (DispColorMode){
      display.invertDisplay(true);
      DispColorMode = false; 
    }
    else{
      display.invertDisplay(false);
      DispColorMode = true;
    }    
    current_screen = 0;

}     
  else if (item_selected == 7) {
    Serial.println("[APP] Dim Display");
    if (DispDimMode){
      display.dim(false);
      DispDimMode = false; 
    }
    else{
     display.dim(true);
      DispDimMode = true;
    }    
    current_screen = 0;

}     
  else if (item_selected == 8) {
    inProgram = true;
    Serial.println("[APP] Timer Started");
    setupTimerScreen();
    while (inProgram){
      display.clearDisplay();
      mainTimer();
      globalBackHandler();
    }
    Serial.println("[APP] Timer Exited");
  
}
  else if (item_selected == 9) {
    inProgram = true;
    Serial.println("[APP] Stopwatch Started");
    setupStopwatchScreen();
    while (inProgram){
      display.clearDisplay();
      mainStopwatch();
      globalBackHandler();
    }
    Serial.println("[APP] Stopwatch Exited");
  
} 
  else if (item_selected == 10) {
    inProgram = true;
    Serial.println("[APP] OTA Started");
    setupOTAScreen();
    while (inProgram){
      display.clearDisplay();
      buttonServerHandler();
      globalBackHandler();
    }
    Serial.println("[APP] OTA Exited");
  
  } 
  else if (item_selected == 11) {
    inProgram = true;
    Serial.println("[APP] Packet Monitor Started");
      WifiPromiscuousON();
      display.clearDisplay();
      mainPacMon();
      globalBackHandler();
  
  } 
  else if (item_selected == 12) {
    inProgram = true;
    Serial.println("[APP] WiFi Monitor Started");
    WifiTurnONSTA();
    while (inProgram){
      display.clearDisplay();
      displayWiFiStatus();
      globalBackHandler();
    }
    Serial.println("[APP] WiFi Monitor Exited");
  
  } 
  else if (item_selected == 13) {
    inProgram = true;
    Serial.println("[APP] Wifi Scanner Started");
    WifiTurnONSTANC();
    while (inProgram){
      display.clearDisplay();
      mainWifiScanner();
      globalBackHandler();
    }
    Serial.println("[APP] Wifi Scanner Exited");
  
  } 
  else if (item_selected == 14) {
    inProgram = true;
    Serial.println("[SYSTEM] Restart");
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Restarting....");
    display.display();
    ESP.restart();
    delay(100);
  
  } 
  else if (item_selected == 15) {
    inProgram = true;
    Serial.println("[APP] Wifi Setup Started");
    while (inProgram){
      display.clearDisplay();
      mainWifiSetup();
      globalBackHandler();
    }
    Serial.println("[APP] Wifi Setup Exited");
  
  }
  else if (item_selected == 16) {
    inProgram = true;
    Serial.println("[APP] Battery Started");
    while (inProgram){
      display.clearDisplay();
      mainBattery();
      globalBackHandler();
    }
    Serial.println("[APP] Battery Exited");
  
  }
  else {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(12, 8+20+20+2+2);
    display.println("not implemented lol"); 
    display.display();
  }

}
///////////////////////
/// Pong Functions ///
/////////////////////

// Draw Rectangular Court on Screen
void drawCourt() 
{
    display.drawRect(0, 0, 128, 54, WHITE);
}

// Main Game Function
void mainPong() {
    bool update_needed = false;
    unsigned long time = millis();
    bool courtDrawn = true;
    if (courtDrawn){
      drawCourt();
      courtDrawn = false;
    }

    static bool up_state = false;
    static bool down_state = false;
    
    up_state |= (digitalRead(BUTTON_UP_PIN) == LOW);
    down_state |= (digitalRead(BUTTON_DOWN_PIN) == LOW);

    if(time > ball_update){
        uint8_t new_x = ball_x + ball_dir_x;
        uint8_t new_y = ball_y + ball_dir_y;

        // Check if we hit the vertical walls
        if(new_x == 0 || new_x == 127){
            ball_dir_x = -ball_dir_x;
            new_x += ball_dir_x + ball_dir_x;

            if (new_x < 64){
                player_score++;
            }
            else{
                mcu_score++;
            }

            if (player_score == SCORE_LIMIT || mcu_score == SCORE_LIMIT){
                win = player_score > mcu_score;
                game_over = true;
            }
        }

        // Check if we hit the horizontal walls.
        if(new_y == 0 || new_y == 53){
            ball_dir_y = -ball_dir_y;
            new_y += ball_dir_y + ball_dir_y;
        }

        // Check if we hit the CPU paddle
        if(new_x == MCU_X && new_y >= mcu_y && new_y <= mcu_y + PADDLE_HEIGHT){
            ball_dir_x = -ball_dir_x;
            new_x += ball_dir_x + ball_dir_x;
        }

        // Check if we hit the player paddle
        if(new_x == PLAYER_X && new_y >= player_y && new_y <= player_y + PADDLE_HEIGHT){
            ball_dir_x = -ball_dir_x;
            new_x += ball_dir_x + ball_dir_x;
        }

        display.drawPixel(ball_x, ball_y, BLACK);
        display.drawPixel(new_x, new_y, WHITE);
        ball_x = new_x;
        ball_y = new_y;

        ball_update += BALL_RATE;

        update_needed = true;
    }

    if(time > paddle_update){
        paddle_update += PADDLE_RATE;

        // CPU paddle
        display.drawFastVLine(MCU_X, mcu_y, PADDLE_HEIGHT, BLACK);
        const uint8_t half_paddle = PADDLE_HEIGHT >> 1;

        if(mcu_y + half_paddle > ball_y){
            int8_t dir = ball_x > MCU_X ? -1 : 1;
            mcu_y += dir;
        }

        if(mcu_y + half_paddle < ball_y){
            int8_t dir = ball_x > MCU_X ? 1 : -1;
            mcu_y += dir;
        }

        if(mcu_y < 1){
            mcu_y = 1;
        }

        if(mcu_y + PADDLE_HEIGHT > 53){
            mcu_y = 53 - PADDLE_HEIGHT;
        }

        // Player paddle
        display.drawFastVLine(MCU_X, mcu_y, PADDLE_HEIGHT, WHITE);
        display.drawFastVLine(PLAYER_X, player_y, PADDLE_HEIGHT, BLACK);

        if(up_state){
            player_y -= 1;
        }

        if(down_state){
            player_y += 1;
        }

        up_state = down_state = false;

        if(player_y < 1){
            player_y = 1;
        }

        if(player_y + PADDLE_HEIGHT > 53){
            player_y = 53 - PADDLE_HEIGHT;
        }

        display.drawFastVLine(PLAYER_X, player_y, PADDLE_HEIGHT, WHITE);
        update_needed = true;
    }

    if(update_needed){
        if (game_over){
            const char* text = win ? "YOU WIN!!" : "YOU LOSE!";
            display.clearDisplay();
            display.setCursor(40, 28);
            display.print(text);
            display.display();

            delay(5000);

            display.clearDisplay();
            ball_x = 53;
            ball_y = 26;
            ball_dir_x = 1;
            ball_dir_y = 1;
            mcu_y = 16;
            player_y = 16;
            mcu_score = 0;
            player_score = 0;
            game_over = false;
            drawCourt();
        }

        display.setTextColor(WHITE, BLACK);
        display.setCursor(0, 56);
        display.print(mcu_score);
        display.setCursor(122, 56);
        display.print(player_score);
        display.display();
    }
}

// Resetting all values to intial ones.
void stopPong(){
  ball_x = 53;
  ball_y = 26;
  ball_dir_x = 1;
  ball_dir_y = 1;
  mcu_y = 16;
  player_y = 16;
  mcu_score = 0;
  player_score = 0;
}

// Setup Game Timer
void setupPong(){
    ball_update = millis();
    paddle_update = ball_update;
}

/////////////////////////////
/// Flappy Bird Functions///
///////////////////////////

// Centre Text on OLED
void textAtCenter(int y, String txt) {
  display.setCursor(128 / 2 - txt.length() * 3, y);
  display.print(txt);
}

//  Main Game Function
void mainFlappyBird(){ 
  if (game_state == 0) {    
    display.clearDisplay();
    if (digitalRead(BUTTON_UP_PIN) == LOW) {
      momentum = -4;
      yield();
    }
    momentum += 1;
    bird_y += momentum;
    if (bird_y < 0 ) {
      bird_y = 0;
    }
    if (bird_y > 64 - 16) {
      bird_y = 64 - 16;
      momentum = -2;
    }
    if (momentum < 0) {
      if (random(2) == 0) {
        display.drawBitmap(bird_x, bird_y, wing_down_bmp, 16, 16, WHITE);
      }
      else {
        display.drawBitmap(bird_x, bird_y, wing_up_bmp, 16, 16, WHITE);
      }
    }
    else {
      display.drawBitmap(bird_x, bird_y, wing_up_bmp, 16, 16, WHITE);
    }
    for (int i = 0 ; i < 2; i++) {
      display.fillRect(wall_x[i], 0, wall_width, wall_y[i], WHITE);
      display.fillRect(wall_x[i], wall_y[i] + wall_gap, wall_width, 64 - wall_y[i] + wall_gap, WHITE);
      if (wall_x[i] < 0) {
        wall_y[i] = random(0, 64 - wall_gap);
        wall_x[i] = 128;
      }
      if (wall_x[i] == bird_x) {
        birdScore++;
        high_score = max(birdScore, high_score);
      }
      if (
        (bird_x + 16 > wall_x[i] && bird_x < wall_x[i] + wall_width) // level with wall
        &&
        (bird_y < wall_y[i] || bird_y + 16 > wall_y[i] + wall_gap) // not level with the gap
      ) {        
        display.display();
        delay(500);
        game_state = 1;
      }
      wall_x[i] -= 4;
    }
    textAtCenter(0, (String)birdScore);
    display.display();
    delay(50);
    yield();
  }
  else {
    display.clearDisplay();
    textAtCenter(1, "Flappy Bird");    
    textAtCenter(64 / 2 - 8, "GAME OVER");
    textAtCenter(64 / 2, String(birdScore));    
    textAtCenter(64 - 16, "HIGH SCORE");
    textAtCenter(64  - 8, String(high_score));
    display.display();
    while (digitalRead(BUTTON_UP_PIN) == LOW);
    bird_y = 64 / 2;
    momentum = -4;
    wall_x[0] = 128 ;
    wall_y[0] = 64 / 2 - wall_gap / 2;
    wall_x[1] = 128 + 128 / 2;
    wall_y[1] = 64 / 2 - wall_gap / 1;
    birdScore = 0;
    while (digitalRead(BUTTON_UP_PIN) == HIGH);
    display.clearDisplay();
    game_state = 0;        
  }
  yield();
}

// Stopping and resetting Game
void stopFlappyBird(){
  display.clearDisplay();
  game_state = 1;
}

///////////////////////
/// Internet Clock ///
/////////////////////

// Fetch Time from NTP
// Format the Output in Time Array and print on Display
void timeFetch(){
  timeClient.update();
  unsigned long unix_epoch = timeClient.getEpochTime();   // get UNIX Epoch time
 
  second_ = second(unix_epoch);        // get seconds from the UNIX Epoch time
  if (last_second != second_){
    minute_ = minute(unix_epoch);      // get minutes (0 - 59)
    hour_   = hour(unix_epoch);        // get hours   (0 - 23)
    wday    = weekday(unix_epoch);     // get minutes (1 - 7 with Sunday is day 1)
    day_    = day(unix_epoch);         // get month day (1 - 31, depends on month)
    month_  = month(unix_epoch);       // get month (1 - 12 with Jan is month 1)
    year_   = year(unix_epoch) - 2000; // get year with 4 digits - 2000 results 2 digits year (ex: 2018 --> 18)
 
    Time[7] = second_ % 10 + '0';
    Time[6] = second_ / 10 + '0';
    Time[4] = minute_ % 10 + '0';
    Time[3] = minute_ / 10 + '0';
    Time[1] = hour_   % 10 + '0';
    Time[0] = hour_   / 10 + '0';
    Date[9] = year_   % 10 + '0';
    Date[8] = year_   / 10 + '0';
    Date[4] = month_  % 10 + '0';
    Date[3] = month_  / 10 + '0';
    Date[1] = day_    % 10 + '0';
    Date[0] = day_    / 10 + '0';
 
    display_wday();
    draw_text(4,  19, Date, 2);        // display date (format: dd-mm-yyyy)
    draw_text(16, 40, Time, 2);        // display time (format: hh:mm:ss)
    display.display();
 
    last_second = second_;
 
  } 
  delay(200);
}

// Display Weekday
void display_wday()
{
  switch(wday){
    case 1:  draw_text(40, 5, " SUNDAY  ", 1); break;
    case 2:  draw_text(40, 5, " MONDAY  ", 1); break;
    case 3:  draw_text(40, 5, " TUESDAY ", 1); break;
    case 4:  draw_text(40, 5, "WEDNESDAY", 1); break;
    case 5:  draw_text(40, 5, "THURSDAY ", 1); break;
    case 6:  draw_text(40, 5, " FRIDAY  ", 1); break;
    default: draw_text(40, 5, "SATURDAY ", 1);
  }
}

// Custom text display Function
void draw_text(byte x_pos, byte y_pos, char *text, byte text_size)
{
  display.setCursor(x_pos, y_pos);
  display.setTextSize(text_size);
  display.print(text);
}

////////////////
/// Counter ///
//////////////

// Initial waiting screen for input
void setupCounterScreen(){
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Counter");
  display.display();
}

// Counter Runner
void mainCounter(){
  if (digitalRead(BUTTON_UP_PIN) == LOW) {
    delay(60); // Debounce delay
    if (digitalRead(BUTTON_UP_PIN) == LOW) {
      buttonCount++;
      updateDisplay();
    }
  }

  if (digitalRead(BUTTON_DOWN_PIN) == LOW) {
    delay(50); // Debounce delay
    if (digitalRead(BUTTON_DOWN_PIN) == LOW) {
      buttonCount = 0; // Reset the count
      updateDisplay();
    }
  }
  yield();
}

// Count Updator
void updateDisplay() {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(2);
  textAtCenter(10, "Count");
  textAtCenter(30, String(buttonCount));
  display.display();
}

//////////////
/// Timer ///
////////////

// Timer Updator
void updateScreen() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(35, 25);
  display.print(timerMinutes);
  display.print(":");
  if (timerSeconds < 10) {
    display.print("0"); // Add leading zero for single-digit seconds
  }
  display.print(timerSeconds);
  display.display();
}

// Initial waiting screen for input
void setupTimerScreen(){
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Timer");
  display.display();
}

// Main Timer 
// Reads Input and controls timer
void mainTimer(){
  // Button 1: Add minutes
  if (digitalRead(BUTTON_UP_PIN) == LOW) {
    delay(100);
    if (digitalRead(BUTTON_UP_PIN) == LOW && !timerRunning) {
      timerMinutes++;
      updateScreen();
      yield();
    }
    yield();
  }

  // Button 2: Add seconds / Reset button when timer is running
  if (digitalRead(BUTTON_DOWN_PIN) == LOW) {
    delay(100);
    if (digitalRead(BUTTON_DOWN_PIN) == LOW) {
      if (timerRunning) {
        // Reset the timer
        timerMinutes = 0;
        timerSeconds = 0;
        timerRunning = false;
        updateScreen();
      } else {
        timerSeconds++;
        if (timerSeconds >= 60) {
          timerSeconds = 0;
        }
        updateScreen();
        yield();
      }
      yield();
    }
    yield();
  }

  // Button 3: Start/Stop timer
  if (digitalRead(BUTTON_SELECT_PIN) == LOW) {
    delay(100);
    if (digitalRead(BUTTON_SELECT_PIN) == LOW) {
      if (timerRunning) {
        timerRunning = false;
      } else {
        timerRunning = true;
        if (timerMinutes > 0 || timerSeconds > 0) {
          previousTimeTimer = millis();  // Start the timer
          yield();
        }
      }
      yield();
    }
    yield();
  }

  // Timer logic
  if (timerRunning) {
    unsigned long currentTime = millis();
    if (currentTime - previousTimeTimer >= timerSecInterval) {
      previousTimeTimer = currentTime;
      if (timerSeconds > 0 || timerMinutes > 0) {
        timerSeconds--;
        if (timerSeconds < 0) {
          timerMinutes--;
          timerSeconds = 59;
        }
        updateScreen();
      } else {
        // Timer finished
        timerRunning = false;
      }
    }
  }
  yield();
}

//////////////////
/// Stopwatch ///
////////////////

// Initial waiting screen for input
void setupStopwatchScreen(){
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Stopwatch");
  display.display();
}

// Main runner
// Button input and process stopwatch
void mainStopwatch(){
  static unsigned long previousTime = 0;
  const unsigned long interval = 100;  // Update display every 100 milliseconds

  if (digitalRead(BUTTON_UP_PIN) == LOW) {
    if (!isRunning) {
      startTime = millis();
      isRunning = true;
    } else {
      isRunning = false;
    }
    delay(100); // Debounce delay
  }

  if (isRunning) {
    elapsedTime = millis() - startTime;
  }

  unsigned long currentTime = millis();
  if (currentTime - previousTime >= interval) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("Time: ");
    display.setTextSize(2);
    display.setCursor(35, 8);
    display.print(elapsedTime / 1000); // Display seconds
    display.print(".");
    display.print((elapsedTime % 1000) / 10); // Display milliseconds
    display.display();
    previousTime = currentTime;
  }
}

/////////////////////////
/// Resource Monitor ///
///////////////////////

// Display Stats
void mainResMon(){
  // Clear the display
  unsigned long currentMillis = millis();
  if (currentMillis - previousUpdateTime >= 2000) {
    previousUpdateTime = currentMillis;
  display.setTextSize(1);
  display.clearDisplay();

  // Read and print the free RAM
  int freeRam = ESP.getFreeHeap();
  display.setCursor(0, 0);
  display.print("Free RAM: ");
  display.print(freeRam);
  display.println(" bytes");

  // Read and print the flash size
  uint32_t flashSize = ESP.getFlashChipRealSize();
  display.setCursor(0, 12);
  display.print("Flash Size: ");
  display.print(flashSize / 1024);
  display.println(" KB");

  // Read and print the CPU speed
  uint32_t cpuFreq = ESP.getCpuFreqMHz();
  display.setCursor(0, 24);
  display.print("CPU Speed: ");
  display.print(cpuFreq);
  display.println(" MHz");

  // Read and print the CPU usage
  uint8_t cpuUsage = ESP.getFreeContStack();
  display.setCursor(0, 36);
  display.print("CPU Usage: ");
  display.print(100 - cpuUsage);
  display.println(" %");

  // Read and print the free sketch space
  uint32_t freeSketchSpace = ESP.getFreeSketchSpace();
  display.setCursor(0, 48);
  display.print("Free Space: ");
  display.print(freeSketchSpace / 1024);
  display.println(" KB");

  // Display the information on the OLED
  display.display();
  yield();
  }
  yield();
}

////////////////
/// eReader ///
//////////////

// Display list of texts available
void displayFileSelection() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);

  // Display the file headings
  for (int i = 0; i < numberOfFiles; i++) {
    if (i == selectedFileIndex) {
      display.print("> "); // Add a pointer for the selected heading
      display.setTextColor(INVERSE);
    } else {
      display.print("  "); // Add empty spaces for non-selected headings
      display.setTextColor(WHITE);
    }
    display.println(fileHeadings[i]);
    yield();
  }

  display.display();
  yield();
}

// Display the choosen text
void displayFileContent() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0 - (scrollOffset * 8)); // Adjust scrollOffset to pixels

  String content = fileContents[selectedFileIndex];

  // Calculate the number of lines required to display the text
  int numLines = ceil((float)content.length() / (128 / 6.5));

  // Calculate the maximum scroll offset based on the number of lines
  int maxScrollOffset = numLines - (64 / 8);
  if (maxScrollOffset < 0) {
    maxScrollOffset = 0;
  }

  // Adjust the scroll offset if it exceeds the maximum
  if (scrollOffset > maxScrollOffset) {
    scrollOffset = maxScrollOffset;
  }

  // Display the content of the selected file
  for (int i = 0; i < numLines; i++) {
    int startIndex = i * (128 / 6.5);
    String line = content.substring(startIndex, startIndex + (128 / 6.5));
    display.println(line);
    yield();
  }

  display.display();
  yield();
}

// Scroll text with buttons
void scrollUp() {
  scrollOffset--;
  if (scrollOffset < 0) {
    scrollOffset = 0;
  }
  displayFileContent();
  delay(200);
  yield();
}

void scrollDown() {
  scrollOffset++;
  displayFileContent();
  delay(200);
  yield();
}

// Text selection
void selectFile() {
  if (menuState == FILE_SELECTION) {
    menuState = FILE_VIEWING;
    scrollOffset = 0; // Reset the scroll offset to 0
    displayFileContent();
    yield();
  } else {
    menuState = FILE_SELECTION;
    displayFileSelection();
    yield();
  }
  delay(200);
}

// Setup initial number of texts available
void setupEReader(){
  numberOfFiles = numFiles;
}

// eReader handler
void mainEReader(){
    // Check button states
  int upButtonState = digitalRead(BUTTON_UP_PIN);
  int downButtonState = digitalRead(BUTTON_DOWN_PIN);
  int selectButtonState = digitalRead(BUTTON_SELECT_PIN);

  // Handle button actions based on the menu state
  if (menuState == FILE_SELECTION) {
    if (upButtonState == LOW) {
      selectedFileIndex--;
      if (selectedFileIndex < 0) {
        selectedFileIndex = numberOfFiles - 1;
      }
      displayFileSelection();
      delay(200);
      yield();
    }
    else if (downButtonState == LOW) {
      selectedFileIndex++;
      if (selectedFileIndex >= numberOfFiles) {
        selectedFileIndex = 0;
      }
      displayFileSelection();
      delay(200);
      yield();
    }
    else if (selectButtonState == LOW) {
      selectFile();
      yield();
    }
    else {
      displayFileSelection();
      yield();
    }
  }
  else if (menuState == FILE_VIEWING) {
    if (upButtonState == LOW) {
      scrollUp();
      yield();
    }
    else if (downButtonState == LOW) {
      scrollDown();
      yield();
    }
    else if (selectButtonState == LOW) {
      menuState = FILE_SELECTION;
      displayFileSelection();
      delay(200);
      yield();
    }
    yield();
  }
  yield();
}

/////////////////////////
/// System Functions ///
///////////////////////

// Wifi Functions
// All these functions put WiFi Modem to sleep after Turning OFF
// List of Known Common APs around
void configureAccessPoints() {
  wifiMulti.addAP("EACCESS", "hostelnet");
  wifiMulti.addAP("TU", "tu@inet1");
  wifiMulti.addAP("LC", "lc@tiet1");
  wifiMulti.addAP("CSED", "csed@123#");
  wifiMulti.addAP("Placement Cell", "Cilp@98765");
  wifiMulti.addAP("Machine Tool", "workshop@54321");
  wifiMulti.addAP("HostelB", "hostelnet");
  wifiMulti.addAP("Audi", "audi@net");
}

// Turn ON Access Point Mode and create a AP with SSID ESPOTA
void WifiTurnONAP(){
  // isWiFiRunning = true;
  Serial.println("[WIFI] AP ON");
  WiFi.forceSleepWake();
  WiFi.mode(WIFI_AP); 
  WiFi.softAP("ESPOTA", "password");
}

// Turn ON Station Mode and connect to Network
void WifiTurnONSTA(){
  // isWiFiRunning = true;
  Serial.println("[WIFI] STA ON");
  WiFi.forceSleepWake();
  WiFi.mode(WIFI_STA);
  wifi_station_connect();

  while (wifiMulti.run() != WL_CONNECTED) {
  Serial.println("WiFi not connected!");
  delay(500);
  }
}

// Turn ON Station Mode but do not connect to Network
void WifiTurnONSTANC(){
  // isWiFiRunning = true;
  Serial.println("[WIFI] STA NC ON");
  WiFi.forceSleepWake();
  WiFi.mode(WIFI_STA);
}

// Turn ON Promiscuous Mode
void WifiPromiscuousON(){
  // isWiFiRunning = true;
  Serial.println("[WIFI] Promiscuous Mode ON");
  WiFi.forceSleepWake();
  wifi_set_opmode(STATION_MODE);
  wifi_promiscuous_enable(0);
  WiFi.disconnect();
  wifi_set_promiscuous_rx_cb(sniffer);
  wifi_set_channel(curChannel);
  wifi_promiscuous_enable(1);
}

// Turn OFF Promiscuous Mode
void WifiPromiscuousOFF(){
  // isWiFiRunning = false;
  Serial.println("[WIFI] Promiscuous Mode OFF");
  wifi_promiscuous_enable(0);
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();
  delay(1);
}

// Disconnect and Turn OFF Station Mode
void WifiTurnOFFSTA(){
  // isWiFiRunning = false;
  Serial.println("[WIFI] STA OFF");
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();
  delay(1);
}

// Disconnect and Turn OFF Access Point Mode
void WifiTurnOFFAP(){
  // isWiFiRunning = false;
  Serial.println("[WIFI] AP OFF");
  WiFi.softAPdisconnect();
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();
  delay(1);
}

////////////
/// OTA ///
//////////

void startServer() {
  // Start in access point mode
  WifiTurnONAP();
  IPAddress apIP = WiFi.softAPIP();
  httpUpdater.setup(&httpServer);

  // Serve the OTA update page
  httpServer.on("/", HTTP_GET, [](void) {
    httpServer.send(200, "text/html", "<html><body><h1>ESP8266 OTA Update</h1></body></html>");
  });

  // Start the HTTP server
  httpServer.begin();
  Serial.println("HTTP server started");

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(50, 0);
  display.println("OTA");
  display.println("Server: Running");
  display.println("IP: " + apIP.toString());
  display.println("AP: ESPOTA");
  display.display();
}

void setupOTAScreen(){
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("OTA Update Server");
  display.display();
}

void stopServer() {
  // Stop the HTTP server
  httpServer.stop();

  // Turn off access point
  WifiTurnOFFAP();

  Serial.println("HTTP server stopped");

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(50, 0);
  display.println("OTA");
  display.println("Server: Stopped");
  display.display();
}

void buttonServerHandler(){
    if (digitalRead(BUTTON_UP_PIN) == LOW) {
    startServer();
    Serial.println("Start button pressed");
    delay(1000);
  }

  if (digitalRead(BUTTON_DOWN_PIN) == LOW) {
    stopServer();
    Serial.println("Stop button pressed");
    delay(1000);
  }

  // Handle incoming client requests
  httpServer.handleClient();
}


void mainPacMon(){
  curTime = millis();

  // Button
  if (digitalRead(BUTTON_UP_PIN) == LOW) {
    if (canBtnPress) canBtnPress = false;
  } else if (!canBtnPress) {
    canBtnPress = true;

    // Change Channel
    curChannel++;
    if (curChannel > maxCh) curChannel = 1;
    wifi_set_channel(curChannel);
    for (int i = 0; i < maxRow; i++) val[i] = 0;
    pkts = 0;
    multiplicator = 1;

    if (pkts == 0) pkts = deauths;
    no_deauths = pkts - deauths;

  }

  // Every second
  if (curTime - prevTime >= 1000) {
    prevTime = curTime;

    // Scrolling
    for (int i = 0; i < maxRow; i++) {
      val[i] = val[i + 1];
    }
    val[127] = pkts;

    // Scaling factor
    getMultiplicator();

    if (pkts == 0) pkts = deauths;
    no_deauths = pkts - deauths;

    //draw display
    display.clearDisplay();
    display.setCursor(Row1, 0);
    display.print("Ch:");
    display.setCursor(Row3, 0);    
    display.print("Pkts:");
    display.setCursor(Row5, 0);
    display.print("DA:");
    display.setCursor(Row2, 0);
    display.print((String)curChannel);
    display.setCursor(Row4, 0);
    display.print((String)no_deauths);
    display.setCursor(Row6, 0);
    display.print((String)deauths);
    for (int i = 0; i < maxRow; i++) display.drawLine(i, maxLine, i, maxLine - val[i]*multiplicator, WHITE);
    display.display();

    //reset counters
    deauths    = 0;
    pkts       = 0;
  }

}

void sniffer(uint8_t *buf, uint16_t len) {
  pkts++;
  if (buf[12] == 0xA0 || buf[12] == 0xC0) {
    deauths++;
  }
}

void getMultiplicator() {
  maxVal = 1;
  for (int i = 0; i < maxRow; i++) {
    if (val[i] > maxVal) maxVal = val[i];
  }
  if (maxVal > LineVal) multiplicator = (double)LineVal / (double)maxVal;
  else multiplicator = 1;
}

void stopPacMon(){
  unsigned long prevTime   = 0;
  unsigned long curTime    = 0;
  unsigned long pkts       = 0;
  unsigned long no_deauths = 0;
  unsigned long deauths    = 0;
  int curChannel           = 1;
  unsigned long maxVal     = 0;
  double multiplicator     = 0.0;
  bool canBtnPress         = true;
  unsigned int val[128];
}

void displayWiFiStatus() {
  display.clearDisplay();
  
  display.setCursor(0, 0);
  display.print("Status: ");
  
  if (WiFi.status() == WL_CONNECTED) {
    display.println("Connected");
    display.print("IP: ");
    display.println(WiFi.localIP());
    display.print("RSSI: ");
    display.print(WiFi.RSSI());
    display.println(" dBm");
    display.print("SSID: ");
    display.println(WiFi.SSID());
    display.print("BSSID:");
    display.println(WiFi.BSSIDstr());
    display.print("Channel: ");
    display.println(WiFi.channel());
    display.print("MAC:");
    display.println(WiFi.macAddress());
  } else {
    display.println("Disconnected");
  }
  
  display.display();
}


void mainWifiScanner(){
  static unsigned long lastScanTime = 0;
  if (millis() - lastScanTime >= 7000) {
    lastScanTime = millis();
    numNetworksScan = WiFi.scanNetworks();
  }

  // Read button states
  bool buttonUpState = digitalRead(BUTTON_UP_PIN);
  bool buttonDownState = digitalRead(BUTTON_DOWN_PIN);
  bool buttonSelectState = digitalRead(BUTTON_SELECT_PIN);

  // Handle button presses
  if (buttonUpState == LOW && lastButtonUpState == HIGH) {
    if (!displayScanInfo && selectedNetworkIndex > 0) {
      selectedNetworkIndex--;
    }
    displayScanInfo = false; // Reset displayScanInfo flag when changing the selection
    delay(200);
  }
  if (buttonDownState == LOW && lastButtonDownState == HIGH) {
    if (!displayScanInfo && selectedNetworkIndex < numNetworksScan - 1) {
      selectedNetworkIndex++;
    }
    displayScanInfo = false; // Reset displayScanInfo flag when changing the selection
    delay(200);
  }
  if (buttonSelectState == LOW && lastButtonSelectState == HIGH) {
    if (!displayScanInfo) {
      displayScanInfo = true; // Toggle displayScanInfo flag to show/hide network info
    } else {
      displayScanInfo = false;
    }
    delay(200);
  }

  // Store current button states for the next iteration
  lastButtonUpState = buttonUpState;
  lastButtonDownState = buttonDownState;
  lastButtonSelectState = buttonSelectState;

  // Update display
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);

  if (!displayScanInfo) {
    display.setCursor(0, 0);
    display.print("Wi-Fi Networks:");
    display.println(numNetworksScan);

    for (int i = 0; i < numNetworksScan; i++) {
      display.setCursor(0, (i + 1) * 8);
      if (i == selectedNetworkIndex) {
        display.print(">");
      }
      display.print(WiFi.SSID(i));
    }

    display.display();
  } else {
    showNetworkInfo();
  }
}

void showNetworkInfo() {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println(WiFi.SSID(selectedNetworkIndex)); // Use the stored selected SSID instead of retrieving it directly
  display.println(" ");
  display.print("MAC:");
  display.println(WiFi.BSSIDstr(selectedNetworkIndex));
  display.print("RSSI:");
  display.print(WiFi.RSSI(selectedNetworkIndex));
  display.println(" dBm");
  display.print("Channel:");
  display.println(WiFi.channel(selectedNetworkIndex));
  display.print("Encryption:");
  display.println(getEncryptionType(WiFi.encryptionType(selectedNetworkIndex)));
  display.print("Hidden:");
  display.println(WiFi.isHidden(selectedNetworkIndex) ? "Yes" : "No");
  display.display();
}

String getEncryptionType(int encryptionType) {
  switch (encryptionType) {
    case ENC_TYPE_NONE:
      return "Open";
    case ENC_TYPE_WEP:
      return "WEP";
    case ENC_TYPE_TKIP:
      return "WPA";
    case ENC_TYPE_CCMP:
      return "WPA2";
    case ENC_TYPE_AUTO:
      return "Auto";
    default:
      return "Unknown";
  }
}

void light_sleep(){
  Serial.println("[POWER] Sleeping");
   wifi_station_disconnect();
   wifi_set_opmode_current(NULL_MODE);
   wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
   wifi_fpm_open(); // Enables force sleep
   wifi_fpm_set_wakeup_cb(callback);
   Serial.println("[DISPLAY] Display OFF");  
   display.ssd1306_command(SSD1306_DISPLAYOFF);
   gpio_pin_wakeup_enable(GPIO_ID_PIN(BUTTON_DOWN_PIN), GPIO_PIN_INTR_LOLEVEL); // GPIO_ID_PIN(2) corresponds to GPIO2 on ESP8266-01 , GPIO_PIN_INTR_LOLEVEL for a logic low, can also do other interrupts, see gpio.h above
   gpio_pin_wakeup_enable(GPIO_ID_PIN(BUTTON_UP_PIN), GPIO_PIN_INTR_LOLEVEL);
   wifi_fpm_do_sleep(0xFFFFFFF); // Sleep for longest possible time
 }

 void callback(){
  Serial.println("[DISPLAY] Display ON");   
   display.ssd1306_command(SSD1306_DISPLAYON);
   display.clearDisplay();
   inProgram = false;
   current_screen = 0;
 }


void mainWifiSetup(){  
  // Check for button press
  if (digitalRead(BUTTON_UP_PIN) == LOW) {
    wifiEnabled = !wifiEnabled; // Toggle WiFi state
    isDirty = true; // Mark display for refresh
    setWiFiState(wifiEnabled); // Set WiFi state
    delay(100); // Button debouncing delay
  }

  if (digitalRead(BUTTON_DOWN_PIN) == LOW) {
    apEnabled = !apEnabled; // Toggle AP state
    isDirty = true; // Mark display for refresh
    setAPState(apEnabled); // Set AP state
    delay(100); // Button debouncing delay
  }

  if (digitalRead(BUTTON_SELECT_PIN) == LOW) {
    captivePortalEnabled = !captivePortalEnabled; // Toggle captive portal state
    isDirty = true; // Mark display for refresh
    setCaptivePortalState(captivePortalEnabled); // Set captive portal state
    delay(100); // Button debouncing delay
  }

  drawWifiMain();
  yield();
}

void drawWifiMain(){
  
  if (isDirty) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.print("WiFi: ");
    display.println(wifiEnabled ? "ON " : "OFF");

    display.print("AP: ");
    display.println(apEnabled ? "ON " : "OFF");

    display.print("Captive Portal: ");
    display.println(captivePortalEnabled ? "ON " : "OFF");

    if (wifiEnabled && WiFi.status() == WL_CONNECTED) {
      display.println("IP: " + WiFi.localIP().toString());
      display.println("SSID: " + String(WiFi.SSID()));
      display.println("Signal: " + String(WiFi.RSSI()) + " dBm");
      display.println("Gateway:" + WiFi.gatewayIP().toString());
      display.println("Subnet:" + WiFi.subnetMask().toString());

    }

    if (captivePortalEnabled && WiFi.status() == WL_CONNECTED) {
      display.println("IP: " + WiFi.localIP().toString());
      display.println("SSID: " + String(WiFi.SSID()));
      display.println("Signal: " + String(WiFi.RSSI()) + " dBm");
      display.println("Gateway:" + WiFi.gatewayIP().toString());
      display.println("Subnet:" + WiFi.subnetMask().toString());

    }

    if (apEnabled) {
      display.println();
      display.println("AP IP:");
      display.println(WiFi.softAPIP().toString());
    }

    display.display();
    isDirty = false;
  }
  yield();
}

void setWiFiState(bool state) {
  display.clearDisplay();
  display.setCursor(0, 0);

  if (state) {
    display.print("Connecting");
    display.display();
    WifiTurnONSTA();

  } else {
    display.println("Disconnecting");
    display.display();
    WifiTurnOFFSTA();
  }
}

void setAPState(bool state) {
  display.clearDisplay();
  display.setCursor(0, 0);

  if (state) {
    display.println("Enabling AP");
    display.display();
    WifiTurnONAP();
  } else {
    display.println("Disabling AP");
    display.display();
    WifiTurnOFFAP();
  }
}

void setCaptivePortalState(bool state) {
  display.clearDisplay();
  display.setCursor(0, 0);

  if (state) {
    display.println("Starting Captive Portal");
    display.display();
    wifiManager.setConfigPortalTimeout(60);
    wifiManager.startConfigPortal("ESPOTA", "password");

  } else {
    display.println("Stopping Captive Portal");
    display.display();
    wifiManager.stopConfigPortal();
  }
}

void mainBattery(){
  unsigned long currentMillis = millis();
  if (currentMillis - prevTimeBat >= pollingBat)
  {
    prevTimeBat = currentMillis;

    sensorValue = analogRead(analogInPin);
    float voltage = (((sensorValue * 3.3) / 1024) * 2 + calibration);
    bat_percentage = mapfloat(voltage, 2.9, 4.1, 0, 100);
    if (bat_percentage >= 100) { bat_percentage = 100; }
    if (bat_percentage <= 0) { bat_percentage = 1; }

    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Voltage ");
    display.println(voltage);
    display.print("Percentage ");
    display.println(bat_percentage);
    display.display();

    Serial.print("Output Voltage = ");
    Serial.print(voltage);
    Serial.print("  Battery Percentage = ");
    Serial.println(bat_percentage);
  }
}
float mapfloat(float x, float in_min, float in_max, float out_min, float out_max){
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}