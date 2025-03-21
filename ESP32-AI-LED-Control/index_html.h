#ifndef INDEX_HTML_H
#define INDEX_HTML_H

const char htmlPage[] PROGMEM = R"HTML(

<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>ESP32 Gemini AI LED Controller</title>
  <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0/css/all.min.css" />
  <link href="https://fonts.googleapis.com/css2?family=Roboto+Mono:wght@400;700&display=swap" rel="stylesheet">
  <style>
    :root {
      /* Enhanced Cyberpunk Theme */
      --primary: #00ffea;
      --secondary: #9d00ff;
      --accent: #ff2a6d;
      --bg-dark: #0a0a0f;
      --card-bg: rgba(20, 20, 30, 0.95);
      --text: #e0e0e0;
      --text-dim: #a0a0a0;
      --transition: all 0.3s ease;
      --glow: 0 0 15px;
    }
    * {
      margin: 0;
      padding: 0;
      box-sizing: border-box;
      font-family: 'Roboto Mono', monospace;
    }
    body {
      background: linear-gradient(135deg, #0a0a0f, #1a1a2e);
      min-height: 100vh;
      color: var(--text);
      display: flex;
      justify-content: center;
      align-items: center;
      padding: 0;
    }
    .container {
      width: 100%;
      max-width: 900px;
      background: var(--card-bg);
      border-radius: 12px;
      overflow: hidden;
      box-shadow: 0 0 30px rgba(0, 0, 0, 0.9);
      border: 1px solid rgba(0, 255, 234, 0.2);
      height: 100vh;
      display: flex;
      flex-direction: column;
    }
    header, footer {
      padding: 15px;
      text-align: center;
      background: rgba(0, 0, 0, 0.4);
      backdrop-filter: blur(5px);
      border-bottom: 1px solid rgba(0, 255, 234, 0.2);
    }
    header {
      display: flex;
      justify-content: space-between;
      align-items: center;
    }
    .logo {
      display: flex;
      align-items: center;
      gap: 10px;
    }
    .logo-icon {
      width: 44px;
      height: 44px;
      background: linear-gradient(45deg, var(--primary), var(--secondary));
      border-radius: 50%;
      display: flex;
      justify-content: center;
      align-items: center;
      color: #000;
      font-size: 1.4rem;
      box-shadow: var(--glow) var(--primary);
    }
    h1 {
      font-size: 1.8rem;
      background: linear-gradient(45deg, var(--primary), var(--secondary));
      -webkit-background-clip: text;
      -webkit-text-fill-color: transparent;
      text-shadow: 0 0 5px rgba(0, 255, 234, 0.5);
    }
    .status {
      display: flex;
      align-items: center;
      gap: 8px;
      font-size: 0.85rem;
    }
    .status-dot {
      width: 10px;
      height: 10px;
      border-radius: 50%;
      background: var(--primary);
      box-shadow: var(--glow) var(--primary);
      animation: pulse 2s infinite;
    }
    @keyframes pulse {
      0%, 100% { opacity: 1; }
      50% { opacity: 0.5; }
    }
    main { 
      padding: 20px; 
      flex: 1;
      display: flex;
      flex-direction: column;
    }
    .tabs { 
      display: flex; 
      justify-content: center; 
      gap: 10px; 
      margin-bottom: 20px; 
    }
    .tab-btn {
      flex: 1;
      padding: 10px;
      background: transparent;
      border: 2px solid var(--primary);
      color: var(--text);
      border-radius: 30px;
      cursor: pointer;
      transition: var(--transition);
      text-transform: uppercase;
      font-size: 0.9rem;
      position: relative;
      overflow: hidden;
    }
    .tab-btn::after {
      content: '';
      position: absolute;
      top: 0;
      left: -100%;
      width: 100%;
      height: 100%;
      background: linear-gradient(90deg, transparent, rgba(0, 255, 234, 0.2), transparent);
      transition: 0.5s;
    }
    .tab-btn:hover::after {
      left: 100%;
    }
    .tab-btn:hover {
      color: var(--primary);
      box-shadow: 0 0 10px rgba(0, 255, 234, 0.5);
    }
    .tab-btn.active {
      background: linear-gradient(45deg, var(--primary), var(--secondary));
      color: #000;
      border-color: transparent;
      box-shadow: 0 0 15px rgba(0, 255, 234, 0.7);
    }
    .tab-content { 
      display: none; 
      animation: fadeIn 0.4s ease; 
      flex: 1;
    }
    .tab-content.active { display: flex; flex-direction: column; }
    @keyframes fadeIn {
      from { opacity: 0; transform: translateY(10px); }
      to { opacity: 1; transform: translateY(0); }
    }
    /* Control Panel with Big Circle Toggle */
    .control-panel {
      padding: 20px;
      text-align: center;
      border: 1px solid rgba(0, 255, 234, 0.2);
      border-radius: 12px;
      background: rgba(0, 0, 0, 0.45);
      margin-bottom: 20px;
      position: relative;
      overflow: hidden;
      flex: 1;
      display: flex;
      flex-direction: column;
      justify-content: center;
    }
    .control-panel::before {
      content: '';
      position: absolute;
      top: 0;
      left: 0;
      right: 0;
      bottom: 0;
      background: radial-gradient(circle at center, rgba(0, 255, 234, 0.1), transparent 60%);
      pointer-events: none;
    }
    .toggle-circle {
      width: 180px;
      height: 180px;
      margin: 20px auto;
      border-radius: 50%;
      background: var(--bg-dark);
      border: 4px solid var(--primary);
      display: flex;
      justify-content: center;
      align-items: center;
      cursor: pointer;
      transition: var(--transition);
      position: relative;
    }
    .toggle-circle:hover {
      box-shadow: 0 0 30px var(--primary);
      transform: scale(1.05);
    }
    .toggle-circle i {
      font-size: 4rem;
      transition: var(--transition);
      color: var(--primary);
    }
    .toggle-circle.active {
      background: var(--primary);
      box-shadow: 0 0 40px var(--primary);
    }
    .toggle-circle.active i { color: var(--bg-dark); }
    .status-display {
      margin-top: 20px;
      font-size: 1.1rem;
      font-weight: 700;
      color: var(--primary);
      text-shadow: 0 0 5px rgba(0, 255, 234, 0.5);
    }
    /* Chat Panel */
    .chat-panel {
  display: flex;
  flex-direction: column;
  height: 75vh; /* Fixed height added */
  border: 1px solid rgba(0, 255, 234, 0.2);
  border-radius: 12px;
  background: rgba(0, 0, 0, 0.35);
  position: relative;
  overflow: hidden;
  box-shadow: 0 0 20px rgba(0, 255, 234, 0.1) inset;
}

    /* Ensure chat panel properly shrinks/expands */
    .chat-panel, .chat-panel * {
      min-height: 0;
    }
    .messages {
      flex: 1;
      overflow-y: auto;
      padding: 15px;
      position: relative;
      z-index: 1;
      scrollbar-width: thin;
      scrollbar-color: var(--primary) var(--bg-dark);
    }
    .messages::-webkit-scrollbar {
      width: 6px;
    }
    .messages::-webkit-scrollbar-track {
      background: var(--bg-dark);
    }
    .messages::-webkit-scrollbar-thumb {
      background-color: var(--primary);
      border-radius: 6px;
    }
    .message {
      margin-bottom: 15px;
      padding: 12px 16px;
      border-radius: 8px;
      background: rgba(20, 20, 30, 0.8);
      font-size: 0.9rem;
      box-shadow: 0 0 10px rgba(0, 255, 234, 0.1);
      border-left: 3px solid var(--primary);
      position: relative;
      overflow: hidden;
    }
    .message.user-message {
      background: rgba(0, 255, 234, 0.1);
      border-left: 3px solid var(--accent);
      margin-left: 20px;
    }
    .message.ai-message {
      background: rgba(157, 0, 255, 0.1);
      border-left: 3px solid var(--secondary);
      margin-right: 20px;
    }
    .message::before {
      content: '';
      position: absolute;
      top: 0;
      left: 0;
      width: 100%;
      height: 100%;
      background: linear-gradient(45deg, transparent, rgba(0, 255, 234, 0.05), transparent);
      pointer-events: none;
    }
    .input-area {
      display: flex;
      border-top: 1px solid rgba(0, 255, 234, 0.2);
      position: relative;
      z-index: 1;
      background: rgba(10, 10, 15, 0.8);
      flex-shrink: 0;
    }
    .input-area input {
      flex: 1;
      padding: 12px 15px;
      border: none;
      background: transparent;
      color: var(--text);
      font-size: 0.9rem;
      outline: none;
    }
    .input-area input::placeholder {
      color: var(--text-dim);
    }
    .input-area button {
      padding: 0 20px;
      border: none;
      background: linear-gradient(45deg, var(--primary), var(--secondary));
      color: var(--bg-dark);
      cursor: pointer;
      font-size: 1rem;
      transition: var(--transition);
      font-weight: bold;
      flex-shrink: 0;
    }
    .input-area button:hover { 
      opacity: 0.9;
      box-shadow: 0 0 15px rgba(0, 255, 234, 0.5);
    }
    footer { 
      font-size: 0.8rem; 
      letter-spacing: 1px; 
      color: var(--text-dim);
      border-top: 1px solid rgba(0, 255, 234, 0.2);
    }
    
    /* Responsive adjustments */
    @media (max-width: 768px) {
      .container {
        max-width: 100%;
        height: 100vh;
        border-radius: 0;
        border: none;
        box-shadow: none;
      }
      body {
        padding: 0;
        height: 100vh;
        overflow: hidden;
      }
    }
    
    @media (max-width: 480px) {
      h1 { font-size: 1.4rem; }
      .tab-btn { font-size: 0.75rem; padding: 8px; }
      .toggle-circle { width: 140px; height: 140px; }
      .toggle-circle i { font-size: 3.2rem; }
      .input-area input { padding: 10px; font-size: 0.8rem; }
      .input-area button { padding: 0 16px; font-size: 0.9rem; }
      .logo-icon {
        width: 36px;
        height: 36px;
        font-size: 1.2rem;
      }
      .status {
        font-size: 0.75rem;
      }
      main {
        padding: 10px;
      }
    }
  </style>
</head>
<body>
  <div class="container">
    <header>
      <div class="logo">
        <div class="logo-icon"><i class="fas fa-lightbulb"></i></div>
        <h1>ESP32 GEMINI AI</h1>
      </div>
      <div class="status">
        <div class="status-dot"></div>
        <span>SYSTEM ONLINE</span>
      </div>
    </header>
    <main>
      <div class="tabs">
        <button class="tab-btn active" data-tab="control">Manual Control</button>
        <button class="tab-btn" data-tab="chat">AI Interface</button>
      </div>
      <div id="control" class="tab-content active">
        <div class="control-panel">
          <h2>Power Matrix</h2>
          <div class="toggle-circle" id="toggleCircle">
            <i class="fas fa-lightbulb"></i>
          </div>
          <div class="status-display">
            SYSTEM STATUS: <span id="ledStateDisplay">STANDBY</span>
          </div>
        </div>
      </div>
      <div id="chat" class="tab-content">
        <div class="chat-panel">
          <div class="messages" id="messageContainer">
            <div class="message ai-message"><em>GEMINI AI SYSTEM ONLINE. AWAITING INPUT.</em></div>
          </div>
          <div class="input-area">
            <input type="text" id="messageInput" placeholder="Enter command..." />
            <button id="sendBtn">OK</button>
          </div>
        </div>
      </div>
    </main>
    <footer>
      <p>ESP32 GEMINI AI NEURAL INTERFACE | Quantum Build 1.0.1</p>
    </footer>
  </div>

  <script>
    // Tab switching functionality
    const tabs = document.querySelectorAll('.tab-btn');
    const tabContents = document.querySelectorAll('.tab-content');
    tabs.forEach(tab => {
      tab.addEventListener('click', () => {
        const target = tab.getAttribute('data-tab');
        tabs.forEach(btn => btn.classList.remove('active'));
        tab.classList.add('active');
        tabContents.forEach(content => {
          content.classList.toggle('active', content.id === target);
        });
      });
    });
    
    // Toggle circle manual control using fetch API
    const toggleCircle = document.getElementById('toggleCircle');
    const ledStateDisplay = document.getElementById('ledStateDisplay');
    let isActive = false;
    
    toggleCircle.addEventListener('click', () => {
      isActive = !isActive;
      let endpoint = isActive ? "/manual/on" : "/manual/off";
      fetch(endpoint)
        .then(response => response.text())
        .then(data => {
          console.log(data);
          toggleCircle.classList.toggle('active', isActive);
          ledStateDisplay.textContent = isActive ? 'ACTIVE' : 'STANDBY';
        })
        .catch(err => console.error("Error:", err));
    });
    
    // AI Chat message handling with API integration
    const sendBtn = document.getElementById('sendBtn');
    const messageInput = document.getElementById('messageInput');
    const messageContainer = document.getElementById('messageContainer');
    
    // Function to send message
    function sendMessage() {
      const userMessage = messageInput.value.trim();
      if (!userMessage) return;
      
      // Display user message in chat
      appendMessage(userMessage, 'user-message');
      messageInput.value = '';
      
      // Send the query to the API endpoint
      fetch("/api/ask?q=" + encodeURIComponent(userMessage))
        .then(response => response.json())
        .then(data => {
          appendMessage(data.answer, 'ai-message');
        })
        .catch(err => {
          console.error("Error:", err);
          appendMessage("Error: Unable to get response", 'ai-message');
        });
    }
    
    // Button click event
    sendBtn.addEventListener('click', sendMessage);
    
    // Enter key press event
    messageInput.addEventListener('keypress', function(e) {
      if (e.key === 'Enter') {
        sendMessage();
      }
    });
    
    // Helper function to append messages to the chat container
    function appendMessage(message, className) {
      const msgElem = document.createElement('div');
      msgElem.className = 'message ' + className;
      msgElem.textContent = message;
      messageContainer.appendChild(msgElem);
      messageContainer.scrollTop = messageContainer.scrollHeight;
    }
  </script>
</body>
</html>
)HTML";

#endif