#include "Express.h"

// Convert string to HttpMethod
HttpMethod stringToMethod(const String& method) {
  if (method.equalsIgnoreCase("GET")) return HttpMethod::GET;
  if (method.equalsIgnoreCase("POST")) return HttpMethod::POST;
  if (method.equalsIgnoreCase("PUT")) return HttpMethod::PUT;
  if (method.equalsIgnoreCase("DELETE")) return HttpMethod::DELETE;
  if (method.equalsIgnoreCase("PATCH")) return HttpMethod::PATCH;
  if (method.equalsIgnoreCase("OPTIONS")) return HttpMethod::OPTIONS;
  if (method.equalsIgnoreCase("HEAD")) return HttpMethod::HEAD;
  return HttpMethod::GET; // Default to GET
}

// Convert HttpMethod to string
String methodToString(HttpMethod method) {
  switch (method) {
    case HttpMethod::GET: return "GET";
    case HttpMethod::POST: return "POST";
    case HttpMethod::PUT: return "PUT";
    case HttpMethod::DELETE: return "DELETE";
    case HttpMethod::PATCH: return "PATCH";
    case HttpMethod::OPTIONS: return "OPTIONS";
    case HttpMethod::HEAD: return "HEAD";
    case HttpMethod::ANY: return "ANY";
    default: return "GET";
  }
}

// --- Request Implementation ---

void Request::parseQueryParams() {
  if (query.isEmpty()) return;
  
  int start = 0;
  int end = query.indexOf('&');
  
  while (start < query.length()) {
    if (end == -1) end = query.length();
    
    int equalPos = query.indexOf('=', start);
    if (equalPos > start && equalPos < end) {
      String key = query.substring(start, equalPos);
      String value = query.substring(equalPos + 1, end);
      
      // URL decode
      key.replace("+", " ");
      value.replace("+", " ");
      
      // Simple URL decoding for key
      for (int i = 0; i < key.length(); i++) {
        if (key[i] == '%' && i + 2 < key.length()) {
          char h1 = key[i + 1];
          char h2 = key[i + 2];
          if (isxdigit(h1) && isxdigit(h2)) {
            char c = (h1 >= 'A' ? (h1 & 0xDF) - 'A' + 10 : h1 - '0') * 16 +
                     (h2 >= 'A' ? (h2 & 0xDF) - 'A' + 10 : h2 - '0');
            key.setCharAt(i, c);
            key.remove(i + 1, 2);
          }
        }
      }
      
      // Simple URL decoding for value
      for (int i = 0; i < value.length(); i++) {
        if (value[i] == '%' && i + 2 < value.length()) {
          char h1 = value[i + 1];
          char h2 = value[i + 2];
          if (isxdigit(h1) && isxdigit(h2)) {
            char c = (h1 >= 'A' ? (h1 & 0xDF) - 'A' + 10 : h1 - '0') * 16 +
                     (h2 >= 'A' ? (h2 & 0xDF) - 'A' + 10 : h2 - '0');
            value.setCharAt(i, c);
            value.remove(i + 1, 2);
          }
        }
      }
      
      queryParams[key] = value;
    }
    
    start = end + 1;
    end = query.indexOf('&', start);
  }
}

String Request::getQuery(const String& name, const String& defaultValue) const {
  if (queryParams.empty()) {
    // Use const_cast to call non-const parseQueryParams() on this instance
    const_cast<Request*>(this)->parseQueryParams();
  }
  auto it = queryParams.find(name);
  return (it != queryParams.end()) ? it->second : defaultValue;
}

String Request::getHeader(const String& name, const String& defaultValue) const {
  auto it = headers.find(name);
  return (it != headers.end()) ? it->second : defaultValue;
}

bool Request::hasHeader(const String& name) const {
  return headers.find(name) != headers.end();
}

String Request::getParam(const String& name, const String& defaultValue) const {
  auto it = params.find(name);
  return (it != params.end()) ? it->second : defaultValue;
}

bool Request::is(const String& type) const {
  String contentType = getHeader("Content-Type", "");
  return contentType.startsWith(type);
}

bool Request::accepts(const String& type) const {
  String accept = getHeader("Accept", "*/*");
  return accept.indexOf(type) >= 0 || accept.indexOf("*/*") >= 0;
}

// --- Response Implementation ---

Response::Response(WiFiClient& client)
  : _client(client), _headersSent(false), _responseSent(false), _statusCode(200) {
  _headers["Connection"] = "close";
  _headers["Content-Type"] = "text/html";
}

Response& Response::header(const String& name, const String& value) {
  if (!_headersSent) {
    _headers[name] = value;
  }
  return *this;
}

Response& Response::code(int statusCode) {
  if (!_headersSent) {
    _statusCode = statusCode;
  }
  return *this;
}

void Response::sendHeaders() {
  if (_headersSent) return;
  
  // Determine status message based on code
  String statusMsg;
  switch (_statusCode) {
    case 200: statusMsg = "OK"; break;
    case 201: statusMsg = "Created"; break;
    case 204: statusMsg = "No Content"; break;
    case 301: statusMsg = "Moved Permanently"; break;
    case 302: statusMsg = "Found"; break;
    case 304: statusMsg = "Not Modified"; break;
    case 400: statusMsg = "Bad Request"; break;
    case 401: statusMsg = "Unauthorized"; break;
    case 403: statusMsg = "Forbidden"; break;
    case 404: statusMsg = "Not Found"; break;
    case 500: statusMsg = "Internal Server Error"; break;
    default: statusMsg = "Unknown";
  }
  
  _client.println("HTTP/1.1 " + String(_statusCode) + " " + statusMsg);
  
  // Send headers
  for (const auto& header : _headers) {
    _client.println(header.first + ": " + header.second);
  }
  
  _client.println(); // Empty line to signal end of headers
  _headersSent = true;
}

void Response::send(const String& content, const String& contentType) {
  if (_responseSent) return;
  
  header("Content-Type", contentType);
  header("Content-Length", String(content.length()));
  
  sendHeaders();
  _client.print(content);
  
  _responseSent = true;
}

void Response::json(const String& jsonContent) {
  send(jsonContent, "application/json");
}

void Response::sendData(const uint8_t* data, size_t len, const String& contentType) {
  if (_responseSent) return;
  
  header("Content-Type", contentType);
  header("Content-Length", String(len));
  
  sendHeaders();
  
  // Send data in chunks to avoid memory issues
  const size_t bufSize = 1024;
  size_t sentBytes = 0;
  
  while (sentBytes < len) {
    size_t bytesToSend = min(bufSize, len - sentBytes);
    _client.write(data + sentBytes, bytesToSend);
    sentBytes += bytesToSend;
    yield(); // Allow ESP32 to handle background tasks
  }
  
  _responseSent = true;
}

void Response::status(int code, const String& message) {
  _statusCode = code;
  
  if (!message.isEmpty()) {
    send(message);
  } else {
    String defaultMsg;
    switch (code) {
      case 404: defaultMsg = "Not Found"; break;
      case 400: defaultMsg = "Bad Request"; break;
      case 500: defaultMsg = "Internal Server Error"; break;
      default: defaultMsg = "Status " + String(code);
    }
    send(defaultMsg);
  }
}

void Response::redirect(const String& url, int statusCode) {
  code(statusCode);
  header("Location", url);
  send("<html><body>Redirecting to <a href=\"" + url + "\">" + url + "</a></body></html>");
}

void Response::end() {
  if (!_headersSent) {
    sendHeaders();
  }
  _responseSent = true;
}

bool Response::isSent() const {
  return _responseSent;
}

// --- Route Implementation ---

Route::Route(HttpMethod m, const String& p, RouteHandler h)
  : method(m), path(p), handler(h) {
  
  // Convert path to regex and extract parameter names
  String regexStr = "^";
  int start = 0;
  int pos = 0;
  
  while ((pos = p.indexOf(':', start)) != -1) {
    regexStr += p.substring(start, pos);
    
    int endPos = p.indexOf('/', pos);
    if (endPos == -1) endPos = p.length();
    
    String paramName = p.substring(pos + 1, endPos);
    paramNames.push_back(paramName);
    
    regexStr += "([^/]+)";
    start = endPos;
  }
  
  if (start < p.length()) {
    regexStr += p.substring(start);
  }
  
  // Handle trailing slash
  if (regexStr.endsWith("/")) {
    regexStr += "?";
  } else {
    regexStr += "/?";
  }
  
  regexStr += "$";
  
  // Create regex
  pathRegex = std::regex(regexStr.c_str(), std::regex_constants::icase);
}

bool Route::matches(HttpMethod m, const String& p) const {
  if (method != m && method != HttpMethod::ANY) {
    return false;
  }
  
  // If no parameters, simple comparison
  if (paramNames.empty()) {
    return path == p || path + "/" == p || (path == "/" && p == "/");
  }
  
  // Use regex for paths with parameters
  std::smatch matches;
  std::string pathStr = p.c_str();
  return std::regex_match(pathStr, matches, pathRegex);
}

void Route::extractParams(const String& p, std::map<String, String>& params) const {
  if (paramNames.empty()) {
    return;
  }
  
  std::smatch matches;
  std::string pathStr = p.c_str();
  if (std::regex_match(pathStr, matches, pathRegex)) {
    for (size_t i = 0; i < paramNames.size() && i + 1 < matches.size(); ++i) {
      params[paramNames[i]] = String(matches[i + 1].str().c_str());
    }
  }
}

// --- ContentProvider Implementation ---

void ContentProvider::addContent(const String& path, const String& data, const String& contentType) {
  // Check if content already exists
  for (auto& content : _contents) {
    if (content.path == path) {
      content.data = data;
      content.contentType = contentType;
      return;
    }
  }
  
  // Add new content
  _contents.push_back({path, contentType, data});
}

const ContentProvider::Content* ContentProvider::getContent(const String& path) const {
  for (const auto& content : _contents) {
    if (content.path == path) {
      return &content;
    }
  }
  return nullptr;
}

void ContentProvider::removeContent(const String& path) {
  for (auto it = _contents.begin(); it != _contents.end(); ++it) {
    if (it->path == path) {
      _contents.erase(it);
      return;
    }
  }
}

// --- Express Implementation ---

Express::Express(int port)
  : _server(port), _running(false) {
  
  // Default 404 handler
  _notFoundHandler = [](const Request& req, Response& res) {
    res.status(404, "Not Found: " + req.path);
  };
}

void Express::get(const String& path, RouteHandler handler) {
  on(HttpMethod::GET, path, handler);
}

void Express::post(const String& path, RouteHandler handler) {
  on(HttpMethod::POST, path, handler);
}

void Express::put(const String& path, RouteHandler handler) {
  on(HttpMethod::PUT, path, handler);
}

void Express::del(const String& path, RouteHandler handler) {
  on(HttpMethod::DELETE, path, handler);
}

void Express::patch(const String& path, RouteHandler handler) {
  on(HttpMethod::PATCH, path, handler);
}

void Express::options(const String& path, RouteHandler handler) {
  on(HttpMethod::OPTIONS, path, handler);
}

void Express::all(const String& path, RouteHandler handler) {
  on(HttpMethod::ANY, path, handler);
}

void Express::on(HttpMethod method, const String& path, RouteHandler handler) {
  _routes.emplace_back(method, path, handler);
}

void Express::use(Middleware middleware) {
  _middlewares.emplace_back("", middleware);
}

void Express::use(const String& path, Middleware middleware) {
  _middlewares.emplace_back(path, middleware);
}

void Express::serveContent(const String& virtualPath, const String& content, const String& contentType) {
  _contentProvider.addContent(virtualPath, content, contentType);
  
  // Create a route handler for this virtual path
  get(virtualPath, [this, virtualPath](const Request& req, Response& res) {
    const auto* content = _contentProvider.getContent(virtualPath);
    if (content) {
      res.send(content->data, content->contentType);
    } else {
      res.status(404, "Content not found");
    }
  });
}

void Express::onNotFound(RouteHandler handler) {
  _notFoundHandler = handler;
}

void Express::handle() {
  WiFiClient client = _server.available();
  if (client) {
    processClient(client);
  }
}

void Express::processClient(WiFiClient& client) {
  // Set connection timeout
  unsigned long timeout = millis() + 3000; // 3 second timeout
  
  // Wait for data to become available
  while (client.connected() && !client.available()) {
    if (millis() > timeout) {
      client.stop();
      return;
    }
    delay(1);
  }
  
  Request req;
  Response res(client);
  
  // Parse the request
  if (!parseRequest(client, req)) {
    res.status(400, "Bad Request");
    client.stop();
    return;
  }
  
  // Apply middleware
  if (!applyMiddleware(req, res)) {
    if (!res.isSent()) {
      res.end();
    }
    client.stop();
    return;
  }
  
  // Find matching route
  bool routeFound = false;
  for (const auto& route : _routes) {
    if (route.matches(req.method, req.path)) {
      route.extractParams(req.path, req.params);
      route.handler(req, res);
      routeFound = true;
      break;
    }
  }
  
  // Check for static content if no route found
  if (!routeFound && !res.isSent()) {
    const auto* content = _contentProvider.getContent(req.path);
    if (content) {
      res.send(content->data, content->contentType);
      routeFound = true;
    }
  }
  
  // If no route or content found, use 404 handler
  if (!routeFound && !res.isSent()) {
    _notFoundHandler(req, res);
  }
  
  // Ensure response is sent
  if (!res.isSent()) {
    res.end();
  }
  
  // Wait a short time before closing
  delay(1);
  client.stop();
}

bool Express::parseRequest(WiFiClient& client, Request& req) {
  // Read first line - e.g., "GET /path?query HTTP/1.1"
  String requestLine = client.readStringUntil('\r');
  client.readStringUntil('\n'); // consume CRLF
  
  int firstSpace = requestLine.indexOf(' ');
  int secondSpace = requestLine.indexOf(' ', firstSpace + 1);
  
  if (firstSpace == -1 || secondSpace == -1) {
    return false;
  }
  
  // Parse method, URL, and HTTP version
  String methodStr = requestLine.substring(0, firstSpace);
  String url = requestLine.substring(firstSpace + 1, secondSpace);
  
  req.method = stringToMethod(methodStr);
  
  // Parse path and query
  int queryPos = url.indexOf('?');
  if (queryPos != -1) {
    req.path = url.substring(0, queryPos);
    req.query = url.substring(queryPos + 1);
  } else {
    req.path = url;
  }
  
  // Read headers until empty line
  String line;
  while (client.available()) {
    line = client.readStringUntil('\r');
    client.readStringUntil('\n'); // consume CRLF
    
    if (line.isEmpty()) {
      break; // End of headers
    }
    
    int colonPos = line.indexOf(':');
    if (colonPos > 0) {
      String name = line.substring(0, colonPos);
      String value = line.substring(colonPos + 1);
      value.trim();
      req.headers[name] = value;
    }
  }
  
  // Read body if Content-Length is set
  if (req.hasHeader("Content-Length")) {
    int contentLength = req.getHeader("Content-Length", "0").toInt();
    
    // Limit body size to prevent memory issues
    const int maxBodySize = 4096; // Adjust as needed
    contentLength = min(contentLength, maxBodySize);
    
    if (contentLength > 0) {
      char* buffer = new char[contentLength + 1];
      size_t bytesRead = 0;
      
      // Read in chunks to avoid buffer overflows
      unsigned long bodyTimeout = millis() + 5000; // 5 second timeout for body
      while (bytesRead < contentLength && client.connected()) {
        if (client.available()) {
          int chunkSize = client.read((uint8_t*)buffer + bytesRead, contentLength - bytesRead);
          if (chunkSize <= 0) break;
          bytesRead += chunkSize;
        }
        
        // Check for timeout
        if (millis() > bodyTimeout) {
          break;
        }
        
        yield(); // Allow ESP32 to handle background tasks
      }
      
      buffer[bytesRead] = '\0';
      req.body = String(buffer);
      delete[] buffer;
    }
  }
  
  return true;
}

bool Express::applyMiddleware(Request& req, Response& res) {
  for (const auto& mw : _middlewares) {
    const String& path = mw.first;
    
    // If middleware has a path prefix, check if request path matches
    if (!path.isEmpty() && !req.path.startsWith(path)) {
      continue;
    }
    
    // Apply middleware
    if (!mw.second(req, res)) {
      return false; // Middleware blocked the request
    }
    
    // If response was sent by middleware, stop processing
    if (res.isSent()) {
      return false;
    }
  }
  
  return true;
}

void Express::listen(const std::function<void()>& onStart) {
  _server.begin();
  _running = true;
  
  if (onStart) {
    onStart();
  }
  
  // Blocking loop
  while (_running) {
    handle();
    yield();
  }
}

void Express::close() {
  _server.stop();
  _running = false;
}

bool Express::isRunning() const {
  return _running;
}

void Express::sendBinaryData(WiFiClient& client, const uint8_t* data, size_t length) {
  // Send in chunks to avoid buffer issues
  const size_t chunkSize = 512; // Smaller chunk size for better responsiveness
  size_t sentBytes = 0;
  
  while (sentBytes < length) {
    size_t bytesToSend = min(chunkSize, length - sentBytes);
    client.write(data + sentBytes, bytesToSend);
    sentBytes += bytesToSend;
    yield(); // Allow ESP32 to handle background tasks
  }
}
