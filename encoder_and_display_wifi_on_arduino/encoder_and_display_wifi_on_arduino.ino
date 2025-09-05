//===================== CALIBRATION =======================
#define CURRENT_SENSE_RESISTOR_1 0.56
#define CURRENT_SENSE_RESISTOR_2 1
float current_sense_resistor[2] =  {CURRENT_SENSE_RESISTOR_1, CURRENT_SENSE_RESISTOR_2};
float A0Correction[2] = {1.02, 1.07};
float A1Correction[2] = {1, 0.91};
float loadVoltageCorrection[2] = {1, 1};
float correctionVariables[4][2] = {
  { A0Correction[0], A0Correction[1] },        // A0
  { A1Correction[0], A1Correction[1] },        // A1
  { current_sense_resistor[0], current_sense_resistor[1] }, // current sense
  { loadVoltageCorrection[0], loadVoltageCorrection[1] }    // load voltage
};
float oldCorrectionVariables[4][2];
int numbPCB = 0;
#define Vdd 4.56
#define Vext 4.56


//====================== WIFI SETTINGS =======================
#include <WiFiS3.h>
#include "webpage.h"
    
WiFiServer server(80);  // Create a WebServer object on port 80
char ssid[] = "ArduinoAP";       // Network SSID
char pass[] = "12345678";        // Password (at least 8 chars)


//====================== LIBRARIES =======================

#include <EEPROM.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "utils.h"

//=============Function prototypes===================
void redOn();
void greenOn();
void yellowOn();
void blueOn();
void pinkOn();
void readEncoder();
void onEncoderButtonPress();
void onSmallButtonPress();


//======================Global Variables======================
int timeFinished = 0; //Time when it finished, to display in ending screen
bool overVoltage = false; //if goes high, it shows overvoltage warning in stats screen

//====================== PIN DEFINITIONS =======================
#define SDA_PIN A4   // adjust if not on Arduino Uno/Nano
#define SCL_PIN A5
#define ENCODER_PIN_A       3
#define ENCODER_PIN_B       2
#define ENCODER_BUTTON_PIN  6
#define SMALL_BUTTON_PIN    8
#define OLED_RESET         4
#define ledR 9
#define ledB 11
#define ledG 10
/*****************
 Pin Connections:
   A0 -> Output Voltage
   A1 -> Current Sense Resistor
   A2 -> Load Voltage Negative
   A3 -> Load Voltage Positive
   A4 -> SDA (I2C)
   A5 -> SCL (I2C)
   2  -> Encoder A
   3  -> Encoder B
   6  -> Encoder Button
   8  -> Small Button
******************/


//====================== OLED Display =======================
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);



/******************** Encoder Variables *******************/
volatile int encoderCount = 0; 
volatile bool encoderButtonPressedFlag = false;
volatile bool smallButtonPressedFlag = false;

volatile unsigned long lastEncoderButtonInterrupt = 0; 
volatile unsigned long lastSmallButtonInterrupt = 0;

unsigned long lastIncrementTime = micros();
unsigned long lastDecrementTime = micros();

const int encoderFastStep = 3; //if quickly turning, it will change by this value
const int encoderDebounceMicros = 25000; //reduce against bouncing


/******************** Menu System *************************/
bool isEditing = false;
int menuCursor = 0;
bool showStatsScreen = 1;
bool showAdvancedMenu = 0;
bool reset = 1; //reset the values of stats screen
bool reset_ending = 0; //lets the ending screen know it should reset
bool toggleStartWifi = 0; //if start is pressed by web server
int elapsedTime = 0;
int timePassed = 0;



/******************** Measurements ************************/
int deviceStats[7] = {0, 60, 3, 44, 0, false, 0}; 
// [0] Last reset time
// [1] Total wanted time (s)
// [2] Desired current (mA)
// [3] Actual current (mA)
// [4] Load voltage (mV)
// [5] Output enabled flag
// [6] Resistance of the load (ohm)

int oldWantedTime = deviceStats[1]; //to check if it changed to updated EEPROM
int oldDesiredCurrent = deviceStats[2]; //idem
int oldnumbPCB = numbPCB; //idem
int voltageSenseRaw = 0; //raw value from ADC over current sense resistor
float voltageSense = 0.0;  //voltage over current sense resistor
int outputVoltageCommand = 0; //digital value to set output voltage over DAC


/******************** Setup *******************************/
void setup() {
  // ====== Serial Setup ======
  Serial.begin(500000);
  pinMode(ledR, OUTPUT);
  pinMode(ledB, OUTPUT);
  pinMode(ledG, OUTPUT);
  blueOn();
  while(!Serial);  // wait for Serial

  // ====== Encoder Setup ======
  // analogReference(AR_EXTERNAL);
  pinMode(ENCODER_PIN_A, INPUT_PULLUP);
  pinMode(ENCODER_PIN_B, INPUT_PULLUP);
  pinMode(ENCODER_BUTTON_PIN, INPUT_PULLUP);
  pinMode(SMALL_BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), readEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_B), readEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_BUTTON_PIN), onEncoderButtonPress, FALLING);
  attachInterrupt(digitalPinToInterrupt(SMALL_BUTTON_PIN), onSmallButtonPress, FALLING);

  // ====== Analog Setup ======
  analogReadResolution(12);
  analogWriteResolution(12);

  // ====== EEPROM Setup ======
  EEPROM.get(0, deviceStats[1]);
  EEPROM.get(4, deviceStats[2]);
  EEPROM.get(8, numbPCB);
  EEPROM.get(16, correctionVariables);


  oldDesiredCurrent = deviceStats[2]; //save values from EEPROM
  oldWantedTime = deviceStats[1];
  oldnumbPCB = numbPCB;
  for (int i=0; i<4; i++){
    for (int j=0; j<2; j++){
      oldCorrectionVariables[i][j] = correctionVariables[i][j];
    }
  }


  // ====== OLED Setup ======
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (true);
  }
  display.clearDisplay();
  display.fillScreen(SSD1306_BLACK);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("Connecting");
  display.println("to the");
  display.println("wifi!");
  display.setTextSize(1);
  display.println("Small button -> no wifi");
  display.display();

  // ====== WiFi Setup ======
  Serial.println("Starting Access Point…");
  int status = WiFi.beginAP(ssid, pass);
  if (status != WL_AP_LISTENING) {
    Serial.println("Failed to start AP");
    while (true);
  }
  // Print the IP address of the AP (usually 192.168.4.1)
  Serial.print("AP started. IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();

}


/******************** Main Loop ***************************/
void loop() {
  //===================== Read Sensors =======================
    int voltageLoadRawPos = analogRead(A3);//voltage over the load
    int voltageLoadRawNeg = analogRead(A2);
    int voltageLoadDiffRaw = voltageLoadRawPos - voltageLoadRawNeg;
    deviceStats[4] = correctionVariables[3][numbPCB]*Vext*1000.0 * voltageLoadDiffRaw / 4096;
    voltageSenseRaw = analogRead(A1); //current sense resistor
    voltageSense = correctionVariables[1][numbPCB]*Vext*1000.0 * voltageSenseRaw / (4096.0);
    deviceStats[3] = voltageSense / correctionVariables[2][numbPCB];
    // deviceStats[3] = voltageSense;
    deviceStats[6] = deviceStats[4]/deviceStats[3]; //resistance of the load
    int vdsRaw = voltageLoadRawNeg - voltageSenseRaw; //voltage over the mosfet
    float vds = correctionVariables[0][numbPCB]*Vext * vdsRaw / 4096.0;
    float powerDissipated = vds * deviceStats[3]; //in mW
    if(powerDissipated > 400){ //if too much power dissipated, turn off output
      overVoltage = true;
    }
    // Calculate output voltage command
    outputVoltageCommand = correctionVariables[0][numbPCB]*deviceStats[2] * correctionVariables[2][numbPCB] * 4096 / (Vdd*1000);
    // Write to output
    if (!deviceStats[5]) {
      analogWrite(A0, 0);
    } else {
      analogWrite(A0, outputVoltageCommand);
    }
    //===================== LIGHTS =======================

    if(!showStatsScreen){
      blueOn();
    }
    else if(deviceStats[5]){
        greenOn();
      }
    else if(voltageLoadDiffRaw > 4096*0.93){
      redOn();
    }
    else{
      yellowOn();
    }
  //===================== MENU =======================
  const char* (*menuFunctions[])(int*, bool*, bool*, int*, bool) = { //functions for each menu item
    timeMenuItem,
    desiredCurrentMenuItem,
    resetTimeMenuItem,
    advancedScreenItem,
    actualCurrentMenuItem,
    voltageMenuItem,
    resistanceMenuItem,
    ipMenuItem
  };  
  AdvancedItemFn menuFunctionsAdvanced[] = {
    PCB_numberMenuItem,
    currentResistorMenuItem,
    A0CorrectionMenuItem,
    A1CorrectionMenuItem,
    loadCorrectionMenuItem
    // add the others here
  };

  int menuOrder[] = {0, 1, 4, 5, 6, 2, 3, 7};
  int menuOrderAdvanced[] = {0, 1, 2, 3, 4};
  //check if time is finished
  if(((millis()-deviceStats[0])/1000 >= deviceStats[1]) && deviceStats[5]){
    showStatsScreen = 0;
    timeFinished = (millis() - deviceStats[0])/1000;
  }
  if(showAdvancedMenu){
    renderAdvancedMenu(
        menuFunctionsAdvanced,
        correctionVariables,
        sizeof(menuFunctionsAdvanced) / sizeof(menuFunctionsAdvanced[0]),
        (bool*)&encoderButtonPressedFlag,
        &menuCursor,
        &isEditing,
        (int*)&encoderCount,
        menuOrderAdvanced,
        (bool*)&smallButtonPressedFlag,
        &reset
      );
  }
  else if(showStatsScreen){
    showStatsScreen = renderStatsMenu(
        menuFunctions,
        deviceStats,
        sizeof(menuFunctions) / sizeof(menuFunctions[0]),
        (bool*)&encoderButtonPressedFlag,
        &menuCursor,
        &isEditing,
        (int*)&encoderCount,
        menuOrder,
        (bool*)&smallButtonPressedFlag, 
        &reset,
        &toggleStartWifi
      );
  }
  else{
    showStatsScreen = renderEnding(deviceStats, (bool*)&smallButtonPressedFlag, &reset_ending);
    deviceStats[5] = 0;
    reset = 1;
  }


  //===================== SAVE EEPROM =======================
  if(oldDesiredCurrent != deviceStats[2]){
    EEPROM.put(4, deviceStats[2]);
    oldDesiredCurrent = deviceStats[2];
  }
  if(oldWantedTime != deviceStats[1]){
    EEPROM.put(0, deviceStats[1]);
    oldDesiredCurrent = deviceStats[1];
  }  
  if(oldnumbPCB != numbPCB){
    EEPROM.put(8, numbPCB);
    oldnumbPCB = numbPCB;
  }

  if(!arraysEqual2D(oldCorrectionVariables, correctionVariables, 4, 2)){
    EEPROM.put(16, correctionVariables);
      for (int i=0; i<4; i++){
        for (int j=0; j<2; j++){
          oldCorrectionVariables[i][j] = correctionVariables[i][j];
        }
      }
  }
  //===================== TIME PASSED FOR DISPLAYING WIFI =======================
  if (!showStatsScreen){ //
    timePassed = deviceStats[1];
  }
  else if(!deviceStats[5]){
    timePassed = elapsedTime/1000;
  }
  else{
    timePassed = (millis() - deviceStats[0])/1000;
  }

//=====================WIFI======================
  WiFiClient client = server.available();   // check for incoming client
  if (client) {
    Serial.println("New client connected");
    String currentLine = "";
    String request = "";
    unsigned long startTime = millis(); // Add a timeout to prevent an infinite loop

  // Read the request line by line, up to the end of the headers
  while (client.connected() && (millis() - startTime < 50)) {
    if (client.available()) {
      //read request
      char c = client.read();
      request += c;
      if (c != '\r') {
        currentLine += c;
      }
      if (c == '\n') {
        // We've received a complete line
        if (currentLine.length() == 0) {
          // This marks the end of the headers, so we can stop
          break;
        }

        // Handle GET requests first, as they are a single line
        if (currentLine.startsWith("GET /updateCurrent")) {
          int value = currentLine.substring(currentLine.indexOf("value=") + 6).toInt();
          if (value > 0) {
            deviceStats[2] = value;
          }
          client.println("HTTP/1.1 200 OK");
          client.println();
          client.stop(); // Stop the client after a successful response
          return;
        }

        if (currentLine.startsWith("GET /updateTime")) {
          int value = currentLine.substring(currentLine.indexOf("value=") + 6).toInt();
          if (value > 0) {
            deviceStats[1] = value;
          }
          client.println("HTTP/1.1 200 OK");
          client.println();
          client.stop();
          return;
        }

        if (currentLine.startsWith("GET /toggleRun")) {
          toggleStartWifi = 1;
          client.println("HTTP/1.1 200 OK");
          client.println();
          client.stop();
          return;
        }

        if (currentLine.startsWith("GET /restart")) {
          reset_ending = 1;
          reset = 1;
          client.println("HTTP/1.1 200 OK");
          client.println();
          client.stop();
          return;
        }

        if (currentLine.startsWith("GET /stats")) { // Return stats as JSON to send to webpage to update
          String json = "{";
          json += "\"desiredCurrent\":" + String(deviceStats[2]) + ",";
          json += "\"measuredCurrent\":" + String(deviceStats[3]) + ",";
          json += "\"loadVoltage\":" + String(deviceStats[4]) + ",";
          json += "\"time\":\"" + String(timePassed) + "/" + String(deviceStats[1]) + "\",";
          json += "\"deviceOn\":" + String(deviceStats[5]) + ",";
          json += "\"showStatsScreen\":" + String(showStatsScreen) + ",";
          json += "\"loadResistance\":" + String(deviceStats[6]);
          json += "}";
          
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: application/json");
          client.println("Connection: close");
          client.println();
          client.println(json);
          client.stop();
          return;
        }
        
        currentLine = ""; // Reset the line for the next one
      }
    }
  }

  Serial.println("Headers: " + request);

  // If we reach here, no GET request was handled, so check for POST data
  int contentLength = 0;
  int idxLen = request.indexOf("Content-Length: ");
  if (idxLen >= 0) {
    int idxEnd = request.indexOf("\r\n", idxLen);
    contentLength = request.substring(idxLen + 16, idxEnd).toInt();
  }

  // --- Read body (POST data) ---
  String body = "";
  while (client.connected() && body.length() < contentLength) {
    if (client.available()) {
      body += (char)client.read();
    }
  }

  Serial.println("Body: " + body);

  bool handledPost = false;

  // ---- Parse POST form data ----
  if (body.indexOf("current=") >= 0) {
    int idx = body.indexOf("current=") + 8;
    int val = body.substring(idx).toInt();
    if (val > 0) {
      deviceStats[2] = val;
    }
    Serial.print("Updated current: ");
    Serial.println(deviceStats[2]);
    handledPost = true;
  }

  if (body.indexOf("time=") >= 0) {
    int idx = body.indexOf("time=") + 5;
    int val = body.substring(idx).toInt();
    if (val > 0) {
      deviceStats[1] = val;
    }
    Serial.print("Updated time: ");
    Serial.println(deviceStats[1]);
    handledPost = true;
  }

  if (body.indexOf("run=0") >= 0) {
    toggleStartWifi = 1;
    Serial.println("Device turned on/off");
    handledPost = true;
  }

  if (body.indexOf("run=1") >= 0) {
    reset_ending = 1;
    reset = 1;
    Serial.println("Device reset");
    handledPost = true;
  }

  // ---- Send response ----
  if (handledPost) {
    client.println("HTTP/1.1 303 See Other");
    client.println("Location: /");
    client.println("Connection: close");
    client.println();
  } else {
    // Normal GET → return the page
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:text/html");
    client.println("Connection: close");
    client.println();
    sendHTML(client); // generate with updated state
  }

  client.stop();
  Serial.println("Client disconnected");
  }

}



//==================UTILS=======================

bool arraysEqual2D(float arr1[][2], float arr2[][2], int rows, int cols) {
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {
      if (arr1[i][j] != arr2[i][j]) {
        return false;
      }
    }
  }
  return true;
}
/******************** Encoder ISR *************************/
void readEncoder() {
  static uint8_t previousState = 3;
  static int8_t encoderDelta = 0;

  const int8_t encoderStateTable[] = {
     0, -1,  1, 0,
     1,  0,  0, -1,
    -1,  0,  0, 1,
     0,  1, -1, 0
  };

  previousState <<= 2;
  if (digitalRead(ENCODER_PIN_A)) previousState |= 0x02;
  if (digitalRead(ENCODER_PIN_B)) previousState |= 0x01;
  encoderDelta += encoderStateTable[previousState & 0x0F];

  if (encoderDelta > 1) {
    encoderCount += (micros() - lastIncrementTime < encoderDebounceMicros) ? encoderFastStep : 1;
    lastIncrementTime = micros();
    encoderDelta = 0;
  } else if (encoderDelta < -1) {
    encoderCount -= (micros() - lastDecrementTime < encoderDebounceMicros) ? encoderFastStep : 1;
    lastDecrementTime = micros();
    encoderDelta = 0;
  }

}

/******************** Button ISRs *************************/
void onEncoderButtonPress() {
  unsigned long now = micros();
  if (now - lastEncoderButtonInterrupt > 200000) {
    encoderButtonPressedFlag = true;
    lastEncoderButtonInterrupt = now;
  }
}

void onSmallButtonPress() {
  unsigned long now = micros();
  if (now - lastSmallButtonInterrupt > 500000) {
    smallButtonPressedFlag = true;
    lastSmallButtonInterrupt = now;
  }
}



/******************** LED Control *************************/

void redOn(){
  analogWrite(ledR, 3000);  
  analogWrite(ledG, 4095);
  analogWrite(ledB, 4095);
}
void greenOn(){
  analogWrite(ledR, 4095);  
  analogWrite(ledG, 3000);
  analogWrite(ledB, 4095);
}
void yellowOn(){
  analogWrite(ledR, 3000);  
  analogWrite(ledG, 3500);
  analogWrite(ledB, 4095);
}

void blueOn(){
  analogWrite(ledR, 4095);
  analogWrite(ledG, 4095);
  analogWrite(ledB, 3000);
}

void pinkOn(){
  analogWrite(ledR, 0);
  analogWrite(ledG, 3000);
  analogWrite(ledB, 2000);

}
