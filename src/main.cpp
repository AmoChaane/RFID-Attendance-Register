#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
//#include <MFRC522DriverI2C.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>
#include "WiFi.h"
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define LED_BUILTIN 2
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const char* ssid = "VC-1012-9086";
const char* password = "41a4843464";
const char* serverUrl = "https://spring-api.publicvm.com/api/v1/health/"; 

// Learn more about using SPI/I2C or check the pin assigment for your board: https://github.com/OSSLibraries/Arduino_MFRC522v2#pin-layout
MFRC522DriverPinSimple ss_pin(5);

MFRC522DriverSPI driver{ss_pin}; // Create SPI driver
//MFRC522DriverI2C driver{};     // Create I2C driver
MFRC522 mfrc522{driver};         // Create MFRC522 instance

MFRC522::MIFARE_Key key;

byte studentNumberBlockAddress = 1;
byte studentNameBlockAddress = 2;
// byte newBlockData[17] = {"Botshelo Bokaba "};
//byte newBlockData[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};   // CLEAR DATA
byte bufferblocksize = 18;
byte studentNumberBlockDataRead[18];
byte studentNameBlockDataRead[18];

// #############Function Declarations##################

bool connectToWiFi();
void makeHttpGetRequest();
void listenForTags();
void readFromBlock(byte blockAddress, byte* blockDataRead, byte bufferBlockSize);
void writeToBlock(byte blockAddress, byte* newBlockData);
void blinkBuiltInLED();
void showTextOnDisplayReplace(String text, int textSize, bool clearDisplay);
String turnByteToString(byte* byte);
String shortenStringToFitScreen(String text);

// ############################################

void setup() {
  Serial.begin(115200);  // Initialize serial communication
  while (!Serial);       // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4).
  
  pinMode(LED_BUILTIN, OUTPUT);
  
  mfrc522.PCD_Init();    // Init MFRC522 board.
  // Serial.println(F("Warning: this example overwrites a block in your card, use with care!"));
  Serial.println("Scan a card/tag...");
 
  // Prepare key - all keys are set to FFFFFFFFFFFF at chip delivery from the factory.
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  // WiFi connection
  // if (!connectToWiFi()) {
  //   Serial.println("Check credentials or hardware.");
  //   while (1);
  // }

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  delay(2000);
  showTextOnDisplayReplace("Scan card/tag...", 2, true);
}

void loop() {
  listenForTags();
}

void listenForTags() {
  // Check if a new card is present
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    delay(500);
    return;
  }

  // Display card UID
  Serial.print("----------------\nCard UID: ");
  // MFRC522Debug::PrintUID(Serial, (mfrc522.uid)); // use in arduino IDE
  MFRC522Debug::PICC_DumpDetailsToSerial(mfrc522, Serial, &(mfrc522.uid)); // use in PlatformIO
  Serial.println();

  readFromBlock(studentNumberBlockAddress, studentNumberBlockDataRead, bufferblocksize);
  readFromBlock(studentNameBlockAddress, studentNameBlockDataRead, bufferblocksize);

  String studentNumber = shortenStringToFitScreen(turnByteToString(studentNumberBlockDataRead));
  String studentName = shortenStringToFitScreen(turnByteToString(studentNameBlockDataRead));

  String displayText = studentNumber + "\n" + studentName;
  showTextOnDisplayReplace(displayText, 2, true);
  
  // Halt communication with the card
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

  delay(2000);  // Delay for readability
}

bool connectToWiFi() {
  //Configures the ESP32 to operate in Station (STA) mode (i.e., as a client that connects to a router).
  // Disables unused modes (like Access Point mode) to save resources.
  WiFi.mode(WIFI_STA); 
  WiFi.begin(ssid, password);
  
  Serial.println("Connecting to WiFi...");
  // millis() Gets the current time (in milliseconds) since the ESP32 started.
  // Purpose: Stores the start time to enforce a 20-second timeout (prevents infinite hanging).
  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 20000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nFailed to connect!");
    return false;
  }

  Serial.println("\nConnected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  return true;
}

void makeHttpGetRequest() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrl);

    int httpCode = http.GET();  // Send the GET request

    if (httpCode > 0) {  // Check for a response
      String payload = http.getString();  // Get the response payload
      Serial.print("HTTP Response Code: ");
      Serial.println(httpCode);
      Serial.print("Response: ");
      Serial.println(payload);
    } else {
      Serial.print("GET request failed. Error: ");
      Serial.println(http.errorToString(httpCode).c_str());
    }
    http.end();  // Free resources
  }
}



void readFromBlock(byte blockAddress, byte* blockDataRead, byte bufferBlockSize = 18) {
  //##############################
  //In RFID communication with MIFARE Classic cards, authentication must be performed before reading or writing data blocks. There are two keys per sector on a MIFARE Classic card:
  // 1. Key
  // 2. Key B
  // These keys control access to the data in that sector.
  // 0x60 -> authenticate with Key A
  // 0x61 -> authenticate with Key B
  //#############################
  // Authenticate the specified block using KEY_A = 0x60
  if (mfrc522.PCD_Authenticate(0x60, blockAddress, &key, &(mfrc522.uid)) != 0) { 
    Serial.println("Authentication failed.");
    return;
  }

  // Read data from the specified block
  if (mfrc522.MIFARE_Read(blockAddress, blockDataRead, &bufferBlockSize) != 0) {
    Serial.println("Read failed.");
  } else {
    Serial.println("Read successfully!");
    Serial.print("Data in block ");
    Serial.print(blockAddress);
    Serial.print(": ");
    for (byte i = 0; i < 16; i++) {
      Serial.print((char)blockDataRead[i]);  // Print as character
    }
    Serial.println();

    blinkBuiltInLED();
  }
  
}

void writeToBlock(byte blockAddress, byte* newBlockData) {
  // Authenticate the specified block using KEY_A = 0x60
  if (mfrc522.PCD_Authenticate(0x60, blockAddress, &key, &(mfrc522.uid)) != 0) {
    Serial.println("Authentication failed.");
    return;
  }
  
  // Write data to the specified block
  if (mfrc522.MIFARE_Write(blockAddress, newBlockData, 16) != 0) {
    Serial.println("Write failed.");
  } else {
    Serial.print("Data written successfully in block: ");
    Serial.println(blockAddress);
  }

}

void blinkBuiltInLED() {
  digitalWrite(LED_BUILTIN, HIGH); // LED ON
  delay(300);                     // Wait 1 second
  digitalWrite(LED_BUILTIN, LOW);  // LED OFF
}

void showTextOnDisplayReplace(String text, int textSize, bool clearDisplay) {
  if(clearDisplay) {
    display.clearDisplay();
  }
  display.setTextSize(textSize);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  // Display static text
  display.println(text);
  display.display(); 
  delay(1000);
}

String turnByteToString(byte* byte) {
  String text = "";
  for (int i = 0; i < 16; i++) {
    if (byte[i] == 0) break; // Stop at null terminator
    text += (char)byte[i];
  }
  return text;
}

String shortenStringToFitScreen(String text) {
  if (text.length() > 10) {
    return text.substring(0, 10);
  }
  return text;
}