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


// If you need custom behavior, consider renaming your version or modifying the library.

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
    // Warning: StaticJsonDocument is deprecated; consider using JsonDocument if you want to update
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
void handleManualOn(Request &req, Response &res) {
  digitalWrite(ledPin, HIGH);
  Serial.println("Manual command: LED turned ON");
  res.send("LED turned ON");
}

// GET /manual/off – Manually turn the LED off.
void handleManualOff(Request &req, Response &res) {
  digitalWrite(ledPin, LOW);
  Serial.println("Manual command: LED turned OFF");
  res.send("LED turned OFF");
}

// GET /api/ask?q=... – Process a Gemini API question.
void handleApiAsk(Request &req, Response &res) {
  // Use the library's urlDecode function (from ESPExpress)
  
  String query = req.getQuery("q");
  Serial.println("API query: " + query);
  String question = urlDecode(query);
  Serial.println("API question: " + question);
  
  String command = sendGeminiRequest(question);
  processCommand(command);
  
  String jsonResponse = "{\"answer\":\"" + command + "\"}";
  res.sendJson(jsonResponse);
}

// GET / – Serve the main HTML page.
void handleDefault(Request &req, Response &res) {
  String htmlContent;
  size_t len = strlen_P(htmlPage);
  for (size_t i = 0; i < len; i++) {
    htmlContent += (char)pgm_read_byte_near(htmlPage + i);
  }
  res.send(htmlContent);
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
  
  // Start listening. This call blocks inside the listen() loop.
  app.listen();
}

void loop() {
  // Not used when app.listen() blocks in setup.
}
