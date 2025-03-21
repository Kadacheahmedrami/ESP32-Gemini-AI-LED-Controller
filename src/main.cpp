#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "index_html.h"      // External HTML file stored in PROGMEM
#include "ESPExpress.h"      // Minimal Express‑like library header

// WiFi credentials
const char* ssid     = "Tenda1200";
const char* password = "78787878";

// Gemini API configuration
const char* Gemini_Token = "AIzaSyAs44KUuNewiuVQynu3ywdByeJCepX0TzE";
const int maxTokens = 100;

// LED configuration
const int ledPin = 2;

// Create an instance of our minimal Express‑like server on port 80
ESPExpress app(80);

//
// ----- Helper Functions -----
//

// URL-decode a given string
String urlDecode(const String& input) {
  String decoded;
  char temp[] = "0x00";
  for (unsigned int i = 0; i < input.length(); i++) {
    if (input[i] == '+') {
      decoded += ' ';
    } else if (input[i] == '%') {
      if (i + 2 < input.length()) {
        temp[2] = input[i + 1];
        temp[3] = input[i + 2];
        decoded += char(strtol(temp, NULL, 16));
        i += 2;
      }
    } else {
      decoded += input[i];
    }
  }
  return decoded;
}

// Generate a strict prompt for Gemini that returns exactly one command (in lowercase)
String generatePrompt(const String& question) {
  return "You are a precise command interpreter for a digital LED. When given an input, respond with EXACTLY one of these commands: 'turn on', 'turn off', or 'no command'. Do not include any extra words, punctuation, or explanations. Input: " + question;
}

// Connect to WiFi with a timeout and serial feedback
void connectToWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000) {
    Serial.print(".");
    delay(500);
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to WiFi");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to WiFi");
  }
}

// Send a query to the Gemini API and return its command response
String sendGeminiRequest(const String& question) {
  String command = "no command";
  HTTPClient http;
  String url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash:generateContent?key=" + String(Gemini_Token);
  
  if (!http.begin(url)) {
    Serial.println("Failed to connect to Gemini API endpoint");
    return command;
  }

  http.addHeader("Content-Type", "application/json");
  String prompt = generatePrompt(question);
  String payload = "{\"contents\": [{\"parts\":[{\"text\":\"" + prompt + "\"}]}],"
                   "\"generationConfig\": {\"maxOutputTokens\": " + String(maxTokens) + "}}";
  
  int httpCode = http.POST(payload);
  if (httpCode == HTTP_CODE_OK) {
    String response = http.getString();
    
    // Using a static buffer size for simplicity; adjust as needed.
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, response);
    
    if (!error) {
      command = doc["candidates"][0]["content"]["parts"][0]["text"].as<String>();
      command.trim();
      command.toLowerCase();
      Serial.println("Gemini returned: " + command);
    } else {
      Serial.println("JSON parse error: " + String(error.c_str()));
    }
  } else {
    Serial.println("HTTP POST failed: " + http.errorToString(httpCode));
  }
  
  http.end();
  return command;
}

// Process the command from Gemini and control the LED accordingly
void processCommand(const String& command) {
  Serial.println("Processing command: " + command);
  if (command == "turn on") {
    digitalWrite(ledPin, HIGH);
    Serial.println("LED turned ON");
  } else if (command == "turn off") {
    digitalWrite(ledPin, LOW);
    Serial.println("LED turned OFF");
  } else {
    Serial.println("No valid command received.");
  }
}

//
// ----- Route Handlers -----
//

// GET /manual/on – Manually turn the LED on.
void handleManualOn(WiFiClient &client, const String &reqLine) {
  digitalWrite(ledPin, HIGH);
  Serial.println("Manual command: LED turned ON");
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/plain");
  client.println("Connection: close");
  client.println();
  client.println("LED turned ON");
}

// GET /manual/off – Manually turn the LED off.
void handleManualOff(WiFiClient &client, const String &reqLine) {
  digitalWrite(ledPin, LOW);
  Serial.println("Manual command: LED turned OFF");
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/plain");
  client.println("Connection: close");
  client.println();
  client.println("LED turned OFF");
}

// GET /api/ask?q=... – Process a Gemini API question.
void handleApiAsk(WiFiClient &client, const String &reqLine) {
  // Extract the query parameter. The request line will be like:
  // "GET /api/ask?q=somequestion HTTP/1.1"
  int qIndex = reqLine.indexOf("/api/ask?q=");
  String geminiAnswer = "";
  if (qIndex != -1) {
    int start = qIndex + strlen("/api/ask?q=");
    int endIndex = reqLine.indexOf(' ', start);
    String query = reqLine.substring(start, endIndex);
    String question = urlDecode(query);
    Serial.println("API question: " + question);
    
    String command = sendGeminiRequest(question);
    processCommand(command);
    geminiAnswer = command;
  }
  String jsonResponse = "{\"answer\":\"" + geminiAnswer + "\"}";
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();
  client.println(jsonResponse);
}

// GET / – Serve the main HTML page.
void handleDefault(WiFiClient &client, const String &reqLine) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  
  size_t len = strlen_P(htmlPage);
  for (size_t i = 0; i < len; i++) {
    client.write(pgm_read_byte_near(htmlPage + i));
  }
}

//
// ----- Setup & Main -----
//

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  
  connectToWiFi();
  
  // Register routes with our minimal Express‑like library.
  app.get("/manual/on", handleManualOn);
  app.get("/manual/off", handleManualOff);
  app.get("/api/ask", handleApiAsk);
  app.get("/", handleDefault);
  
  Serial.println("Web server started on port 80");
  
  // Start listening. (listen() is a blocking call in this minimal implementation.)
  app.listen();
}

void loop() {
  // Not used when app.listen() blocks in setup.
}
