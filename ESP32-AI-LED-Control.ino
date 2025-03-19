#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid     = "rami";
const char* password = "ramirami";

// Gemini API configuration
const char* Gemini_Token = "AIzaSyAs44KUuNewiuVQynu3ywdByeJCepX0TzE";
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

// ----- Embedded HTML UI -----
// Modern, two‑tab interface (Manual Control and AI Chat)
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 Gemini AI LED Controller</title>
  <style>
    :root {
      --primary-color: #3498db;
      --secondary-color: #2980b9;
      --background-color: #f5f5f5;
      --card-background: #ffffff;
      --text-color: #333333;
      --border-color: #e0e0e0;
      --success-color: #2ecc71;
      --error-color: #e74c3c;
      --shadow: 0 2px 10px rgba(0, 0, 0, 0.1);
      --transition: all 0.3s ease;
    }
    * { margin: 0; padding: 0; box-sizing: border-box; font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; }
    body { background-color: var(--background-color); color: var(--text-color); line-height: 1.6; }
    .app-container { display: flex; flex-direction: column; min-height: 100vh; max-width: 800px; margin: 0 auto; padding: 0 20px; }
    header { display: flex; justify-content: space-between; align-items: center; padding: 20px 0; border-bottom: 1px solid var(--border-color); }
    .logo { display: flex; align-items: center; gap: 10px; }
    .logo i { font-size: 24px; color: var(--primary-color); }
    .logo h1 { font-size: 1.5rem; font-weight: 600; color: var(--primary-color); }
    .connection-status { display: flex; align-items: center; gap: 8px; font-size: 0.9rem; }
    .status-dot { width: 10px; height: 10px; border-radius: 50%; background-color: var(--error-color); transition: var(--transition); }
    .status-dot.connected { background-color: var(--success-color); }
    main { flex: 1; padding: 20px 0; }
    .tabs { display: flex; border-bottom: 1px solid var(--border-color); margin-bottom: 20px; }
    .tab-btn { padding: 10px 20px; background: none; border: none; cursor: pointer; font-size: 1rem; color: var(--text-color); transition: var(--transition); }
    .tab-btn.active { color: var(--primary-color); font-weight: 600; border-bottom: 3px solid var(--primary-color); }
    .tab-content { display: none; }
    .tab-content.active { display: block; }
    .control-panel { background-color: var(--card-background); padding: 20px; border-radius: 8px; box-shadow: var(--shadow); text-align: center; }
    .control-panel h3 { margin-bottom: 15px; font-size: 1.2rem; color: var(--primary-color); }
    .button-group { display: flex; justify-content: center; gap: 20px; }
    .control-btn { padding: 10px 20px; background-color: var(--primary-color); color: white; border: none; border-radius: 4px; cursor: pointer; transition: var(--transition); }
    .control-btn:hover { background-color: var(--secondary-color); }
    .chat-container { background-color: var(--card-background); border-radius: 8px; box-shadow: var(--shadow); padding: 20px; height: 60vh; display: flex; flex-direction: column; }
    .messages { flex: 1; overflow-y: auto; margin-bottom: 10px; }
    .message { margin-bottom: 10px; }
    .message.user { text-align: right; color: var(--primary-color); }
    .message.ai { text-align: left; color: var(--secondary-color); }
    .input-area { display: flex; gap: 10px; }
    .input-area input { flex: 1; padding: 10px; border: 1px solid var(--border-color); border-radius: 4px; outline: none; }
    .input-area button { padding: 10px 20px; background-color: var(--primary-color); color: white; border: none; border-radius: 4px; cursor: pointer; }
    footer { padding: 20px 0; border-top: 1px solid var(--border-color); text-align: center; font-size: 0.9rem; color: #777; }
    @media (max-width: 600px) { .logo h1 { font-size: 1.2rem; } }
  </style>
  <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0/css/all.min.css">
</head>
<body>
  <div class="app-container">
    <header>
      <div class="logo">
        <i class="fas fa-lightbulb"></i>
        <h1>ESP32 Gemini AI LED Controller</h1>
      </div>
      <div class="connection-status" id="connectionStatus">
        <span class="status-dot"></span>
        <span class="status-text">Connected</span>
      </div>
    </header>
    <main>
      <div class="tabs">
        <button class="tab-btn active" data-tab="control">Manual Control</button>
        <button class="tab-btn" data-tab="chat">AI Chat</button>
      </div>
      <div class="tab-content active" id="control">
        <div class="control-panel">
          <h3>Power Control</h3>
          <div class="button-group">
            <button class="control-btn" id="turnOnBtn"><i class="fas fa-power-off"></i> Turn On</button>
            <button class="control-btn" id="turnOffBtn"><i class="fas fa-power-off"></i> Turn Off</button>
          </div>
          <div style="margin-top:20px;">
            <p>LED Status: <span id="ledStateDisplay">Off</span></p>
          </div>
        </div>
      </div>
      <div class="tab-content" id="chat">
        <div class="chat-container">
          <div class="messages" id="messageContainer">
            <div class="message system">
              <p>Welcome! Ask your question or command.</p>
            </div>
          </div>
          <div class="input-area">
            <input type="text" id="messageInput" placeholder="Type your message...">
            <button id="sendBtn"><i class="fas fa-paper-plane"></i></button>
          </div>
        </div>
      </div>
    </main>
    <footer>
      <p>ESP32 Gemini AI LED Controller</p>
      <p id="versionInfo">Version 1.0.0</p>
    </footer>
  </div>
  <script>
    // Tab switching
    document.querySelectorAll('.tab-btn').forEach(btn => {
      btn.addEventListener('click', function() {
        document.querySelectorAll('.tab-btn').forEach(b => b.classList.remove('active'));
        document.querySelectorAll('.tab-content').forEach(tc => tc.classList.remove('active'));
        btn.classList.add('active');
        document.getElementById(btn.getAttribute('data-tab')).classList.add('active');
      });
    });
    
    // Manual control buttons using dedicated HTTP endpoints for fast response
    document.getElementById('turnOnBtn').addEventListener('click', () => {
      fetch('/manual/on')
        .then(response => response.text())
        .then(data => {
          setTimeout(() => location.reload(), 100);
        });
    });
    
    document.getElementById('turnOffBtn').addEventListener('click', () => {
      fetch('/manual/off')
        .then(response => response.text())
        .then(data => {
          setTimeout(() => location.reload(), 100);
        });
    });
    
    // Chat form submission using HTTP fetch for AI commands
    document.getElementById('sendBtn').addEventListener('click', () => {
      const input = document.getElementById('messageInput');
      const text = input.value;
      if(text.trim() !== ''){
        addMessage('user', text);
        fetch('/api/ask?q=' + encodeURIComponent(text))
          .then(response => response.json())
          .then(data => {
            addMessage('ai', data.answer);
          });
        input.value = '';
      }
    });
    
    function addMessage(sender, message) {
      const container = document.getElementById('messageContainer');
      const msgDiv = document.createElement('div');
      msgDiv.classList.add('message', sender);
      msgDiv.innerHTML = `<p>${message}</p>`;
      container.appendChild(msgDiv);
      container.scrollTop = container.scrollHeight;
    }
  </script>
  <script src="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0/js/all.min.js"></script>
</body>
</html>
)rawliteral";

// ----- HTTP Request Handling -----
// Handles HTTP requests on port 80.
void handleClient(WiFiClient client) {
  String request = client.readStringUntil('\r');
  client.flush();
  
  Serial.println("Request: " + request);
  
  String question;
  
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
    question = urlDecode(query);
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
  
  // Otherwise, serve the main HTML page.
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  client.print(htmlPage);
  
  delay(1);
  client.stop();
}

// ----- Arduino Setup and Loop -----
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
