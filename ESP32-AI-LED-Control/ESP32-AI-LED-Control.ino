#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <pgmspace.h>
#include "index_html.h"  // Use external HTML file

// WiFi credentials
const char* ssid     = "Tenda1200";
const char* password = "78787878";

// Gemini API configuration
const char* Gemini_Token = "";
const int maxTokens = 100;

// LED configuration
const int ledPin = 2;

// Create an HTTP server on port 80
WiFiServer server(80);

// ----- Helper Functions -----
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

// Generate a strict prompt for Gemini that returns only one command (in lowercase)
String generatePrompt(const String& question) {
  return "You are a precise command interpreter for a digital LED. When given an input, respond with EXACTLY one of these commands: 'turn on', 'turn off', or 'no command'. Do not include any extra words, punctuation, or explanations. Input: " + question;
}

// Connect to WiFi with timeout and serial feedback
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
    DynamicJsonDocument doc(2048);
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

// ----- HTTP Request Handling -----
void handleClient(WiFiClient client) {
  String request = client.readStringUntil('\r');
  client.flush();
  
  Serial.println("Request: " + request);
  
  // Check for manual control endpoints
  if (request.indexOf("GET /manual/on") >= 0) {
    digitalWrite(ledPin, HIGH);
    Serial.println("Manual command: LED turned ON");
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/plain");
    client.println("Connection: close");
    client.println();
    client.println("LED turned ON");
    delay(1);
    client.stop();
    return;
  }
  if (request.indexOf("GET /manual/off") >= 0) {
    digitalWrite(ledPin, LOW);
    Serial.println("Manual command: LED turned OFF");
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/plain");
    client.println("Connection: close");
    client.println();
    client.println("LED turned OFF");
    delay(1);
    client.stop();
    return;
  }
  
  // Check if the request is for the API endpoint (/api/ask?q=...)
  if (request.indexOf("GET /api/ask") >= 0) {
    int qIndex = request.indexOf("GET /api/ask?q=") + strlen("GET /api/ask?q=");
    int endIndex = request.indexOf(" ", qIndex);
    String query = request.substring(qIndex, endIndex);
    String question = urlDecode(query);
    Serial.println("API question: " + question);
    
    String command = sendGeminiRequest(question);
    processCommand(command);
    String jsonResponse = "{\"answer\":\"" + command + "\"}";
    
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.println();
    client.println(jsonResponse);
    
    delay(1);
    client.stop();
    return;
  }
  
  // Otherwise, serve the main HTML page from the external file.
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  
  // Print the HTML stored in PROGMEM from index_html.h
  size_t len = strlen_P(htmlPage);
  for (size_t i = 0; i < len; i++) {
    client.write(pgm_read_byte_near(htmlPage + i));
  }
  
  delay(1);
  client.stop();
}

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW); // Ensure LED starts off
  
  connectToWiFi();
  server.begin();
  Serial.println("Web server started on port 80");
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    Serial.println("New client connected");
    unsigned long timeout = millis();
    while (client.connected() && !client.available()) {
      if (millis() - timeout > 1000) break;
      delay(1);
    }
    if (client.available()) {
      handleClient(client);
    }
  }
}
