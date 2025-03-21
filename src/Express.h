#ifndef EXPRESS_H
#define EXPRESS_H

#include <Arduino.h>
#include <WiFi.h>
#include <vector>
#include <map>
#include <functional>
#include <regex>

// Forward declarations
class Request;
class Response;
class Express;

// Type definition for middleware and route handlers
using RouteHandler = std::function<void(const Request&, Response&)>;
using Middleware = std::function<bool(Request&, Response&)>;

// HTTP Methods enum for type safety
enum class HttpMethod {
  GET,
  POST,
  PUT,
  DELETE,
  PATCH,
  OPTIONS,
  HEAD,
  ANY
};

// Convert string to HttpMethod
HttpMethod stringToMethod(const String& method);
// Convert HttpMethod to string
String methodToString(HttpMethod method);

// --- Request Abstraction ---
class Request {
public:
  HttpMethod method;
  String path;
  String query;
  std::map<String, String> params;     // URL parameters (e.g., /user/:id)
  mutable std::map<String, String> queryParams; // Parsed query parameters (mutable if lazily initialized)
  std::map<String, String> headers;      // Request headers
  String body;                           // Request body
  
  // Parse query string into queryParams map
  void parseQueryParams();
  
  // Get a specific query parameter
  String getQuery(const String& name, const String& defaultValue = "") const;
  
  // Get a specific header
  String getHeader(const String& name, const String& defaultValue = "") const;
  
  // Check if a specific header exists
  bool hasHeader(const String& name) const;
  
  // Get a specific URL parameter
  String getParam(const String& name, const String& defaultValue = "") const;
  
  // Check if content type matches
  bool is(const String& type) const;
  
  // Check if request accepts a specific content type
  bool accepts(const String& type) const;
  
private:
  friend class Express;
};

// --- Response Abstraction ---
class Response {
public:
  Response(WiFiClient& client);
  
  // Send a response with content and an optional content-type
  void send(const String& content, const String& contentType = "text/html");
  
  // Send JSON response
  void json(const String& jsonContent);
  
  // Send binary data (e.g., for images)
  void sendData(const uint8_t* data, size_t len, const String& contentType);
  
  // Send a generic status (e.g., 404 Not Found)
  void status(int code, const String& message = "");
  
  // Set a response header
  Response& header(const String& name, const String& value);
  
  // Set the response status code
  Response& code(int statusCode);
  
  // Redirect to another URL
  void redirect(const String& url, int statusCode = 302);
  
  // End the response
  void end();
  
  // Check if response has been sent
  bool isSent() const;
  
private:
  WiFiClient& _client;
  bool _headersSent;
  bool _responseSent;
  int _statusCode;
  std::map<String, String> _headers;
  
  // Send response headers
  void sendHeaders();
};

// --- Internal Route Structure ---
struct Route {
  HttpMethod method;
  String path;
  std::regex pathRegex;
  std::vector<String> paramNames;
  RouteHandler handler;
  
  Route(HttpMethod m, const String& p, RouteHandler h);
  bool matches(HttpMethod m, const String& p) const;
  void extractParams(const String& path, std::map<String, String>& params) const;
};

// --- Memory-Efficient Content Provider ---
class ContentProvider {
public:
  struct Content {
    String path;
    String contentType;
    String data;
  };
  
  void addContent(const String& path, const String& data, const String& contentType);
  const Content* getContent(const String& path) const;
  void removeContent(const String& path);
  
private:
  std::vector<Content> _contents;
};

// --- Express-like Class ---
class Express {
public:
  Express(int port = 80);
  
  // HTTP method handlers
  void get(const String& path, RouteHandler handler);
  void post(const String& path, RouteHandler handler);
  void put(const String& path, RouteHandler handler);
  void del(const String& path, RouteHandler handler);
  void patch(const String& path, RouteHandler handler);
  void options(const String& path, RouteHandler handler);
  void all(const String& path, RouteHandler handler);
  
  // Register a route with custom method
  void on(HttpMethod method, const String& path, RouteHandler handler);
  
  // Add middleware
  void use(Middleware middleware);
  void use(const String& path, Middleware middleware);
  
  // Serve in-memory content
  void serveContent(const String& virtualPath, const String& content, const String& contentType);

  // Register a callback for unhandled routes
  void onNotFound(RouteHandler handler);
  
  // Handle incoming requests (non-blocking)
  void handle();
  
  // Start server and block (like traditional Express.listen())
  void listen(const std::function<void()>& onStart = nullptr);
  
  // Stop the server
  void close();
  
  // Get server status
  bool isRunning() const;
  
  // Access the content provider
  ContentProvider& content() { return _contentProvider; }
  
private:
  WiFiServer _server;
  std::vector<Route> _routes;
  std::vector<std::pair<String, Middleware>> _middlewares;
  RouteHandler _notFoundHandler;
  bool _running;
  ContentProvider _contentProvider;
  
  // Process an incoming client
  void processClient(WiFiClient& client);
  
  // Parse an HTTP request
  bool parseRequest(WiFiClient& client, Request& req);
  
  // Convert URL path parameters (e.g., /user/:id) to regex
  std::pair<std::regex, std::vector<String>> pathToRegex(const String& path);
  
  // Apply middleware chain
  bool applyMiddleware(Request& req, Response& res);
  
  // Handle binary data efficiently
  void sendBinaryData(WiFiClient& client, const uint8_t* data, size_t length);
};

#endif // EXPRESS_H
