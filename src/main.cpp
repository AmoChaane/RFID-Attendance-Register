#include <Arduino.h>
#include "WiFi.h"
#include <HTTPClient.h>

const char* ssid = "VC-1012-9086";
const char* password = "41a4843464";
const char* serverUrl = "https://spring-api.publicvm.com/api/v1/health/"; 

// This is a forward declaration of a custom function named myFunction. 
// Allows the compiler to recognize myFunction before its actual definition (later in the code)
// This allows to call the function before it is declared
bool connectToWiFi();
void makeHttpGetRequest();

void setup() {
  Serial.begin(115200);
  Serial.println("Serial Monitor Started!!!");
  delay(1000);

  if (!connectToWiFi()) {
    Serial.println("Check credentials or hardware.");
    while (1); // Halt if connection fails
  }

}

void loop() {
  // makeHttpGetRequest();
  // delay(5000);
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
  } else {
    Serial.println("WiFi not connected!");
  }
}
