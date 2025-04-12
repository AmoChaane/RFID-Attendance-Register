#include <Arduino.h>
#include "WiFi.h"
#include <HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>

// Correct pin definitions matching your wiring
#define SS_PIN  5  // ESP32 pin GPIO5 
#define RST_PIN 27 // ESP32 pin GPIO27 

const char* ssid = "VC-1012-9086";
const char* password = "41a4843464";
const char* serverUrl = "https://spring-api.publicvm.com/api/v1/health/"; 

MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
byte nuidPICC[4];

bool connectToWiFi();
void makeHttpGetRequest();

void setup() {
  Serial.begin(115200);
  while (!Serial); // Wait for serial port to connect - needed for native USB
  Serial.println("Serial Monitor Started!!!");
  
  SPI.begin(); // init SPI bus
  rfid.PCD_Init(); // init MFRC522

  Serial.println("Tap an RFID/NFC tag on the RFID-RC522 reader");
  
  // Prepare key - all keys are set to FFFFFFFFFFFFh at chip delivery
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  // WiFi connection (commented out for now to focus on RFID)
  // if (!connectToWiFi()) {
  //   Serial.println("Check credentials or hardware.");
  //   while (1);
  // }
}

void loop() {
  if (rfid.PICC_IsNewCardPresent()) { // new tag is available
    if (rfid.PICC_ReadCardSerial()) { // NUID has been readed
      MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
      Serial.print("RFID/NFC Tag Type: ");
      Serial.println(rfid.PICC_GetTypeName(piccType));

      // print UID in Serial Monitor in the hex format
      Serial.print("UID:");
      for (int i = 0; i < rfid.uid.size; i++) {
        Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
        Serial.print(rfid.uid.uidByte[i], HEX);
      }
      Serial.println();

      rfid.PICC_HaltA(); // halt PICC
      rfid.PCD_StopCrypto1(); // stop encryption on PCD
    }
  }
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




