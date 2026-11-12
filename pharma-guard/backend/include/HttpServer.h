// HttpServer.h - a very small HTTP/1.1 server written with plain sockets.
//
// No third-party library is needed: this works with the sockets that come with
// Linux, macOS (POSIX) and Windows (Winsock). It handles one request at a time,
// which is plenty for a classroom demo and keeps the code easy to read.
#pragma once

#include <functional>
#include <map>
#include <string>

struct HttpRequest {
    std::string method;                            // "GET", "POST", ...
    std::string path;                              // "/api/health" (query string removed)
    std::string body;                              // request body (JSON for our POST routes)
    std::map<std::string, std::string> headers;    // header names are lower-case
};

struct HttpResponse {
    int status;
    std::string contentType;
    std::string body;
    HttpResponse(int s = 200, const std::string& type = "application/json", const std::string& b = "")
        : status(s), contentType(type), body(b) {}
};

class HttpServer {
public:
    using Handler = std::function<HttpResponse(const HttpRequest&)>;

    explicit HttpServer(int port) : port_(port) {}

    void route(const std::string& method, const std::string& path, Handler handler);

    // Starts listening on 127.0.0.1:port and serves forever.
    // Returns false (with a message in `error`) if the server could not start.
    bool run(std::string& error);

private:
    int port_;
    std::map<std::string, Handler> routes_;  // key: "METHOD /path"
};
