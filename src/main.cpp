#include <Arduino.h>
#include "WiFi.h"
#include <HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>

// Correct pin definitions matching your wiring
#define RST_PIN 2     // GPIO2 for reset (confirmed in your hardware)
#define SS_PIN 5      // GPIO5 for slave select

const char* ssid = "VC-1012-9086";
const char* password = "41a4843464";
const char* serverUrl = "https://spring-api.publicvm.com/api/v1/health/"; 

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
byte nuidPICC[4];

bool connectToWiFi();
void makeHttpGetRequest();

void setup() {
  Serial.begin(115200);
  while (!Serial); // Wait for serial port to connect - needed for native USB
  Serial.println("Serial Monitor Started!!!");
  
  // Initialize SPI bus
  SPI.begin();
  // SPI.begin(18, 19, 23, 5);  // SCK, MISO, MOSI, SS

  
  // Initialize MFRC522
  mfrc522.PCD_Init();
  delay(1000); // Short delay needed after init
  
  // Dump version information
  Serial.println(F("Scan PICC to see UID, type, and data blocks..."));
  // mfrc522.PCD_DumpVersionToSerial();
  
  // Prepare key - all keys are set to FFFFFFFFFFFFh at chip delivery
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  // WiFi connection (commented out for now to focus on RFID)
  // if (!connectToWiFi()) {
  //   Serial.println("Check credentials or hardware.");
  //   while (1);
  // }
  mfrc522.PCD_DumpVersionToSerial();
}

void loop() {
  // Reset the loop if no new card present on the sensor/reader
  Serial.println(mfrc522.PICC_IsNewCardPresent());
  Serial.println(mfrc522.PICC_ReadCardSerial());
  Serial.println("===============================");
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Verify if the NUID has been read
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  Serial.println(mfrc522.PICC_GetTypeName(piccType));

  // Check is the PICC of Classic MIFARE type
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&
      piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
      piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return;
  }

  // Print UID
  Serial.print(F("UID:"));
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
  }
  Serial.println();

  // Halt PICC
  mfrc522.PICC_HaltA();
  // Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();
}

// Your existing WiFi functions remain the same...
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
  } else {
    Serial.println("WiFi not connected!");
  }
}
