#ifndef ESP_EXPRESS_H
#define ESP_EXPRESS_H

#include <WiFi.h>

// Define a handler function type that receives the client and the request line.
typedef void (*RouteHandler)(WiFiClient &client, const String &reqLine);

enum HttpMethod {
  GET, // Currently, only GET is supported for simplicity
};

// A simple Route structure holding the HTTP method, path, and its handler.
struct Route {
  HttpMethod method;
  String path;
  RouteHandler handler;
};

class ESPExpress {
public:
  ESPExpress(uint16_t port);
  void get(const String &path, RouteHandler handler);
  void listen(); // Blocking loop to listen for incoming clients
  
private:
  WiFiServer _server;
  Route _routes[10]; // Supports up to 10 routes (adjust if needed)
  uint8_t _routeCount;
  
  void processClient(WiFiClient &client);
};

#endif
