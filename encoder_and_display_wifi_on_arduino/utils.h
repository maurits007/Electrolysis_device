#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Display instance is declared elsewhere
extern Adafruit_SSD1306 display;

void resetTime(int deviceStats[]); //resets the time
void drawPlayButton();
void drawStopButton();
void drawWifiIcon(int x, int y);
int pauseTimestamp = 0; //holds the time when paused
extern int timeFinished; //holds the time when it finished
extern int elapsedTime; //holds the elapsed time when paused
extern bool showAdvancedMenu; //if goes high, it shows the advanced menu
extern bool overVoltage; //if goes high, it should pause
bool showEndingScreen = false; //if goes high, it shows the ending screen

typedef const char* (*AdvancedItemFn)(float correctionVariables[][2], //function pointer type for advanced menu items
                                      bool* isEditing,
                                      bool* encoderButtonPressed,
                                      int* encoderValue,
                                      bool isSelected);


// ======================= MENU RENDERING FUNCTIONS =======================

int renderStatsMenu( //displays the stats menu, returns if it should stay in stats menu or go to ending screen
    const char* (*menuFunctions[])(int*, bool*, bool*, int*, bool), //functions for each menu item 
    int deviceStats[], //array holding device stats ([0] Last reset time, [1] Total wanted time (s), [2] Desired current (mA), [3] Actual current (mA), [4] Load voltage (mV), [5] Output enabled flag, [6] Resistance of the load (ohm))
    int functionCount, //number of functions in menuFunctions
    bool *encoderButtonPressed,  //gets high when encoder button is pressed
    int *menuCursor, //current position of the cursor in the menu
    bool *isEditing, //it is high if the user is editing a value
    int *encoderValue, //value from the encoder, it will change if the user turns the encoder
    int *menuOrder, //gives the order in which menuFunctions should be displayed on the screen
    bool *smallButtonPressed, //gets high when small button is pressed
    bool *reset, //gets send when reseting from finished screen
    bool *toggleStartWifi //goes high if start is pressed by wifi
) {
    showEndingScreen = false; // reset ending screen flag
    bool encoderButtonHandled = false; // To prevent multiple toggles from a single press
    

    //handle reset
    if(*reset){ //it returns from finish screen so everything should get reset
        *reset = false; // Reset the flag after processing
        *menuCursor = 0; // Reset cursor position
        *encoderValue = 0; // Reset encoder value
        *isEditing = false; // Reset editing state
        deviceStats[5] = false; // Reset run/pause state
        deviceStats[0] = millis(); // Reset start time
        elapsedTime = 0; // Reset elapsed time
        *toggleStartWifi = false; // Reset toggle wifi state
    }

    if (overVoltage){
        deviceStats[5] = 0; // Toggle run/pause state
        *isEditing = false; // Exit editing mode if toggling pause/resume
        elapsedTime = millis() - deviceStats[0];
        overVoltage = false;   
    }
    // Handle small button toggle (pause/resume)
    if (*smallButtonPressed or *toggleStartWifi) {
        *toggleStartWifi = false; // Reset the flag after processing
        *smallButtonPressed = false;
        deviceStats[5] = !deviceStats[5]; // Toggle run/pause state
        *isEditing = false; // Exit editing mode if toggling pause/resume
        if (deviceStats[5]) { // Resuming, this will make the current time accurate
            deviceStats[0] = millis() - elapsedTime; // Resume from paused state
            elapsedTime = 0;
        } else { // Keep the time when paused
            elapsedTime = millis() - deviceStats[0];
        }
    }

    // Encoder button and cursor handling
    if (!deviceStats[5]) { // Only allow menu interaction when paused
        if (*encoderButtonPressed) { //if button pressed, reset flags and make encodervalue, menucursor
            *encoderButtonPressed = false; // Reset encoder button state after processing
            encoderButtonHandled = true; // Mark that the button press has been handled

            *isEditing = !(*isEditing); 
            if (!*isEditing) {
                *encoderValue = *menuCursor;
            }
        }

        if (!*isEditing) { //change menucursor depending on encodervalue, but make sure it is in bounds
            if (*encoderValue < 0) {
                *encoderValue = 0;
            } else if (*encoderValue > 3) {
                *encoderValue = 3;
            }
            *menuCursor = *encoderValue;
        }
    } else { //if running, menu shouldn't be changeable
        if(*encoderButtonPressed){
            *encoderButtonPressed = false; // Reset encoder button but shouldn't do anything
        }
        *menuCursor = -1; // Disable menu cursor when running
        *encoderValue = 0;
        *isEditing = false;
    }



    // Menu display
    display.clearDisplay();
    display.fillScreen(SSD1306_BLACK);
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);

    for (int i = 0; i < functionCount; i++) {
        int menuIndex = menuOrder[i];

        if (*menuCursor == menuIndex && *isEditing) { //if changing a value, it should be white
            display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
        } else {
            display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
        }

        display.write(menuIndex == *menuCursor ? '*' : ' '); //put a * if selected
        display.println(menuFunctions[menuIndex](deviceStats, isEditing, &encoderButtonHandled, encoderValue, *menuCursor == menuIndex)); //outputs what should be displayed
    }

    deviceStats[5] == false ? drawStopButton() : drawPlayButton(); //display play button
    drawWifiIcon(115, 10);   // adjust offsets if cut off
    display.display();

    return !showEndingScreen;

}
// will display the advanced menu, does not return anything
void renderAdvancedMenu(AdvancedItemFn menuFunctionsAdvanced[], //functions for each menu item
                        float correctionVariables[][2], //array holding correction variables
                        int functionCountAdvanced, //number of functions in menuFunctions
                        bool* encoderButtonPressed, //gets high when encoder button is pressed
                        int* menuCursor, //current position of the cursor in the menu
                        bool* isEditing, //it is high if the user is editing a value
                        int* encoderValue, //value from the encoder, it will change if the user turns the encoder
                        int* menuOrderAdvanced, //gives the order in which menuFunctions should be displayed on the screen
                        bool* smallButtonPressed, //gets high when small button is pressed
                        bool* reset)  //gets send when reseting stats menu from finished screen
{
    bool encoderButtonHandled = false; // To prevent multiple toggles from a single press

    if (*smallButtonPressed) { //it returns from advanced screen so everything should get reset
        showAdvancedMenu = false;
        *reset = true;
        *smallButtonPressed = false; // Reset the flag after processing
        
    }
    if (*encoderButtonPressed) { //if button pressed, reset flags and make encodervalue, menucursor
            *encoderButtonPressed = false; // Reset encoder button state after processing
            encoderButtonHandled = true; // Mark that the button press has been handled

            *isEditing = !(*isEditing); 
            if (!*isEditing) {
                *encoderValue = *menuCursor;
            }
        }

        if (!*isEditing) { //change menucursor depending on encodervalue, but make sure it is in bounds
            if (*encoderValue < 0) {
                *encoderValue = 0;
            } else if (*encoderValue > 4) {
                *encoderValue = 4;
            }
            *menuCursor = *encoderValue;
        }
    display.clearDisplay();
    display.fillScreen(SSD1306_BLACK);
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
  for (int i = 0; i < functionCountAdvanced; i++) {
    int menuIndex = menuOrderAdvanced[i];
    if (*menuCursor == menuIndex && *isEditing) { //if changing a value, it should be white
        display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    } else {
        display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    }
    display.write(menuIndex == *menuCursor ? '*' : ' '); //put a * if selected

    display.println(
      menuFunctionsAdvanced[menuIndex](
        correctionVariables,
        isEditing,
        &encoderButtonHandled,   // ✅ no extra &
        encoderValue,
        *menuCursor == menuIndex
      )
    );

  }
      display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
      display.println("");
    display.println("Small button -> back");
    display.display();
}

int renderEnding(int deviceStats[], bool *smallButtonPressed, bool *reset_ending){

    display.clearDisplay();
    display.fillScreen(SSD1306_BLACK);
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("It finished!");
    display.println("Total time: " + String(timeFinished) + " s");
    display.println("Current: " + String(deviceStats[3]) + " mA");
    display.println("");
    display.println("Press small button to return");
    

    display.display();
    if(*smallButtonPressed or *reset_ending) {
        *smallButtonPressed = false;
        *reset_ending = false;
        return 1; // Indicate that the small button was pressed
    }
    else {
        return 0; // Indicate that the small button was not pressed
    }

}


// ======================= MENU ITEM FUNCTIONS =======================

const char* timeMenuItem(int deviceStats[], bool *isEditing, bool *encoderButtonPressed, int *encoderValue, bool isSelected) {
    static char buffer[32];
    int currentTimeSeconds;

    if (deviceStats[5]) { // If running, update elapsed time
        currentTimeSeconds = (millis() - deviceStats[0]) / 1000;
    } else { //time should stay the same
        pauseTimestamp = millis();
        currentTimeSeconds = elapsedTime / 1000;
    }

    int totalTime = deviceStats[1]; //until it should stop
    snprintf(buffer, sizeof(buffer), " Time: %d s/%d s", currentTimeSeconds, totalTime);

    if (*encoderButtonPressed && isSelected && *isEditing) { //to make encoder value equal to wanted time when starting to edit
        *encoderValue = totalTime;
    }
    if (*isEditing && isSelected) {
        if(*encoderValue < 0) {
            *encoderValue = 0; // Ensure encoder value does not go below 0
        }
        deviceStats[1] = *encoderValue;
    }
    return buffer;
}

const char* desiredCurrentMenuItem(int deviceStats[], bool *isEditing, bool *encoderButtonPressed, int *encoderValue, bool isSelected) {
    static char buffer[32];
    int desiredCurrent = deviceStats[2];
    snprintf(buffer, sizeof(buffer), " I want: %d mA", desiredCurrent);

    if (*encoderButtonPressed && isSelected && *isEditing) {
        *encoderValue = desiredCurrent;
    }
    if (*isEditing && isSelected) {
        if(*encoderValue < 0) {
            *encoderValue = 0; // Ensure encoder value does not go below 0
        }
        // else if(*encoderValue > 250){
        //     *encoderValue = 250;
        // }
        deviceStats[2] = *encoderValue;
    }
    return buffer;
}
const char* resistanceMenuItem(int deviceStats[], bool*, bool*, int*, bool) {
    static char buffer[32];
    int resistanceValue = deviceStats[6];
    snprintf(buffer, sizeof(buffer), " R load: %d ohm", resistanceValue);
    return buffer;
}
const char* actualCurrentMenuItem(int deviceStats[], bool*, bool*, int*, bool) {
    static char buffer[32];
    int actualCurrent = deviceStats[3];
    snprintf(buffer, sizeof(buffer), " I real: %d mA", actualCurrent);
    return buffer;
}

const char* voltageMenuItem(int deviceStats[], bool*, bool*, int*, bool) {
    static char buffer[32];
    int voltageValue = deviceStats[4];
    snprintf(buffer, sizeof(buffer), " V load: %d mV", voltageValue);
    return buffer;
}

const char* resetTimeMenuItem(int deviceStats[], bool *isEditing, bool*, int*, bool isSelected) {
    static char buffer[32];
    snprintf(buffer, sizeof(buffer), " Reset Time");

    if (*isEditing && isSelected) {
        resetTime(deviceStats);
        *isEditing = false;
    }
    return buffer;
}

const char* advancedScreenItem(int deviceStats[], bool *isEditing, bool*, int*, bool isSelected) {
    static char buffer[32];
    snprintf(buffer, sizeof(buffer), " Adv. Settings");

    if (*isEditing && isSelected) {
        showAdvancedMenu = true;
        *isEditing = false;

    }
    return buffer;
}

const char* ipMenuItem(int deviceStats[], bool *isEditing, bool*, int*, bool isSelected) {
    IPAddress ip = WiFi.localIP();
    static char buffer[32];
    snprintf(buffer, sizeof(buffer), " IP: %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    return buffer;
}

// ======================= ADVANCED MENU ITEM FUNCTIONS =======================

const char* PCB_numberMenuItem(float correctionVariables[][2],
                               bool* isEditing,
                               bool* encoderButtonPressed,
                               int* encoderValue,
                               bool isSelected) {
  static char buffer[32];
    if (*encoderButtonPressed && isSelected && *isEditing) {
        *encoderValue = numbPCB;
    }
    if (*isEditing && isSelected) {
        if(*encoderValue < 0) {
            *encoderValue = 0; // Ensure encoder value does not go below 0
        }
        else if(*encoderValue > 1){
            *encoderValue = 1;
        }
        numbPCB = *encoderValue;
    }
  snprintf(buffer, sizeof(buffer), " PCB number: %d", numbPCB);
  return buffer;
}

const char* A0CorrectionMenuItem(float correctionVariables[][2],
                                 bool* isEditing,
                                 bool* encoderButtonPressed,
                                 int* encoderValue,
                                 bool isSelected) {
  static char buffer[32];
if (*encoderButtonPressed && isSelected && *isEditing) {
    *encoderValue = correctionVariables[0][numbPCB]*100;
}
if (*isEditing && isSelected) {
    if(*encoderValue < 0) {
        *encoderValue = 0; // Ensure encoder value does not go below 0
    }

    correctionVariables[0][numbPCB] = *encoderValue/100.0;
}

  snprintf(buffer, sizeof(buffer), " A0 Corr: %.2f", correctionVariables[0][numbPCB]);
  return buffer;
}

const char* A1CorrectionMenuItem(float correctionVariables[][2],
                                 bool* isEditing,
                                 bool* encoderButtonPressed,
                                 int* encoderValue,
                                 bool isSelected) {
  static char buffer[32];
if (*encoderButtonPressed && isSelected && *isEditing) {
    *encoderValue = correctionVariables[1][numbPCB]*100;
}
if (*isEditing && isSelected) {
    if(*encoderValue < 0) {
        *encoderValue = 0; // Ensure encoder value does not go below 0
    }

    correctionVariables[1][numbPCB] = *encoderValue/100.0;
}

  snprintf(buffer, sizeof(buffer), " A1 Corr: %.2f", correctionVariables[1][numbPCB]);
  return buffer;
}

const char* loadCorrectionMenuItem(float correctionVariables[][2],
                                 bool* isEditing,
                                 bool* encoderButtonPressed,
                                 int* encoderValue,
                                 bool isSelected) {
  static char buffer[32];
if (*encoderButtonPressed && isSelected && *isEditing) {
    *encoderValue = correctionVariables[3][numbPCB]*100;
}
if (*isEditing && isSelected) {
    if(*encoderValue < 0) {
        *encoderValue = 0; // Ensure encoder value does not go below 0
    }

    correctionVariables[3][numbPCB] = *encoderValue/100.0;
}

  snprintf(buffer, sizeof(buffer), " load Corr: %.2f", correctionVariables[3][numbPCB]);
  return buffer;
}

const char* currentResistorMenuItem(float correctionVariables[][2],
                                 bool* isEditing,
                                 bool* encoderButtonPressed,
                                 int* encoderValue,
                                 bool isSelected) {
  static char buffer[32];
if (*encoderButtonPressed && isSelected && *isEditing) {
    *encoderValue = correctionVariables[2][numbPCB]*100;
}
if (*isEditing && isSelected) {
    if(*encoderValue < 0) {
        *encoderValue = 0; // Ensure encoder value does not go below 0
    }

    correctionVariables[2][numbPCB] = *encoderValue/100.0;
}

  snprintf(buffer, sizeof(buffer), " cur. res.: %.2f ohm", correctionVariables[2][numbPCB]);
  return buffer;
}



// ======================= UTILITY FUNCTIONS =======================

void resetTime(int deviceStats[]) {
    deviceStats[0] = millis();
    pauseTimestamp = 0;
    elapsedTime = 0;
}

void drawPlayButton() {
  display.fillTriangle(113, 47, 113, 63, 127, 55, SSD1306_WHITE);
}

void drawStopButton() {
  display.fillRect(113, 47, 4, 34, SSD1306_WHITE);
  display.fillRect(123, 47, 4, 34, SSD1306_WHITE);
}

void drawWifiIcon(int x, int y) {
  // dot
  display.fillCircle(x, y, 1, SSD1306_WHITE);

  // arcs (quarter circles)
  display.drawCircle(x, y, 4, SSD1306_WHITE);
  display.drawCircle(x, y, 7, SSD1306_WHITE);
  display.drawCircle(x, y, 10, SSD1306_WHITE);
//   display.fillRect(x-11, y, 24, 13  , SSD1306_BLACK);
  display.fillTriangle(x-10, y-10, x+10, y+10, x-10, y+10, SSD1306_BLACK);
  display.fillTriangle(x+10, y-10, x-10, y+10, x+10, y+10, SSD1306_BLACK);
}





