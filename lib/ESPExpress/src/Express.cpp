#include "../include/ESPExpress.h"

ESPExpress::ESPExpress(uint16_t port)
  : _server(port), _routeCount(0) {}

void ESPExpress::get(const String &path, RouteHandler handler) {
  if (_routeCount < 10) {
    _routes[_routeCount++] = { GET, path, handler };
  }
}

void ESPExpress::listen() {
  _server.begin();
  while (true) {
    WiFiClient client = _server.available();
    if (client) {
      processClient(client);
    }
    delay(1);
  }
}

void ESPExpress::processClient(WiFiClient &client) {
  // Read the first line of the HTTP request (e.g., "GET /path HTTP/1.1")
  String reqLine = client.readStringUntil('\r');
  client.readStringUntil('\n'); // Skip LF

  int firstSpace = reqLine.indexOf(' ');
  int secondSpace = reqLine.indexOf(' ', firstSpace + 1);
  if (firstSpace == -1 || secondSpace == -1) {
    client.stop();
    return;
  }
  
  String methodStr = reqLine.substring(0, firstSpace);
  String path = reqLine.substring(firstSpace + 1, secondSpace);
  
  // For simplicity, we only support GET.
  if (methodStr != "GET") {
    client.println("HTTP/1.1 405 Method Not Allowed");
    client.println("Connection: close");
    client.println();
    client.stop();
    return;
  }
  
  // Find a matching route
  bool routeFound = false;
  for (uint8_t i = 0; i < _routeCount; i++) {
    if (_routes[i].path == path) {
      _routes[i].handler(client, reqLine);
      routeFound = true;
      break;
    }
  }
  
  // If no route matches, send a 404
  if (!routeFound) {
    client.println("HTTP/1.1 404 Not Found");
    client.println("Connection: close");
    client.println();
  }
  
  delay(1);
  client.stop();
}
