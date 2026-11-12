#include "HttpServer.h"

#include <cstring>
#include <iostream>
#include <stdexcept>

#include "Json.h"
#include "Text.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif
using socket_t = SOCKET;
static const socket_t kBadSocket = INVALID_SOCKET;
static void closeSocket(socket_t s) { closesocket(s); }
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
using socket_t = int;
static const socket_t kBadSocket = -1;
static void closeSocket(socket_t s) { close(s); }
#endif

namespace {

const std::size_t kMaxRequestBytes = 1024 * 1024;  // refuse requests bigger than 1 MB

const char* reasonPhrase(int status) {
    switch (status) {
        case 200: return "OK";
        case 204: return "No Content";
        case 400: return "Bad Request";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 413: return "Payload Too Large";
        default: return "Internal Server Error";
    }
}

// Reads one HTTP request from the socket. Returns false if it is incomplete or too large.
bool readRequest(socket_t client, HttpRequest& request, int& errorStatus) {
    std::string data;
    char buffer[4096];
    std::string::size_type headerEnd = std::string::npos;

    while (headerEnd == std::string::npos) {
        int n = static_cast<int>(recv(client, buffer, sizeof buffer, 0));
        if (n <= 0) return false;
        data.append(buffer, static_cast<std::size_t>(n));
        if (data.size() > kMaxRequestBytes) {
            errorStatus = 413;
            return false;
        }
        headerEnd = data.find("\r\n\r\n");
    }

    // ---- request line + headers
    std::string head = data.substr(0, headerEnd);
    std::vector<std::string> headLines = Text::split(head, '\n');
    std::vector<std::string> first = Text::split(Text::trim(headLines[0]), ' ');
    if (first.size() < 2) {
        errorStatus = 400;
        return false;
    }
    request.method = first[0];
    request.path = first[1].substr(0, first[1].find('?'));  // drop "?query=..."
    for (std::vector<std::string>::size_type i = 1; i < headLines.size(); ++i) {
        std::string line = Text::trim(headLines[i]);
        std::string::size_type colon = line.find(':');
        if (colon == std::string::npos) continue;
        request.headers[Text::toLower(line.substr(0, colon))] = Text::trim(line.substr(colon + 1));
    }

    // ---- body
    std::size_t contentLength = 0;
    auto it = request.headers.find("content-length");
    if (it != request.headers.end()) {
        try {
            contentLength = static_cast<std::size_t>(std::stoul(it->second));
        } catch (const std::exception&) {
            errorStatus = 400;
            return false;
        }
    }
    if (contentLength > kMaxRequestBytes) {
        errorStatus = 413;
        return false;
    }
    std::string body = data.substr(headerEnd + 4);
    while (body.size() < contentLength) {
        int n = static_cast<int>(recv(client, buffer, sizeof buffer, 0));
        if (n <= 0) return false;
        body.append(buffer, static_cast<std::size_t>(n));
    }
    request.body = body.substr(0, contentLength);
    return true;
}

void sendAll(socket_t client, const std::string& data) {
    int flags = 0;
#ifdef MSG_NOSIGNAL
    flags = MSG_NOSIGNAL;  // do not crash if the browser closed the connection
#endif
    std::size_t sent = 0;
    while (sent < data.size()) {
        int n = static_cast<int>(send(client, data.data() + sent, static_cast<int>(data.size() - sent), flags));
        if (n <= 0) return;
        sent += static_cast<std::size_t>(n);
    }
}

void sendResponse(socket_t client, const HttpResponse& response) {
    std::string text = "HTTP/1.1 " + std::to_string(response.status) + " " + reasonPhrase(response.status) + "\r\n";
    text += "Content-Type: " + response.contentType + "; charset=utf-8\r\n";
    text += "Content-Length: " + std::to_string(response.body.size()) + "\r\n";
    // CORS headers let a web page from another port (the React dev server) call this API.
    text += "Access-Control-Allow-Origin: *\r\n";
    text += "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
    text += "Access-Control-Allow-Headers: Content-Type\r\n";
    text += "Connection: close\r\n\r\n";
    text += response.body;
    sendAll(client, text);
}

HttpResponse errorResponse(int status, const std::string& message) {
    JsonObject o;
    o.add("ok", false).add("error", message);
    return HttpResponse(status, "application/json", o.str());
}

}  // namespace

void HttpServer::route(const std::string& method, const std::string& path, Handler handler) {
    routes_[method + " " + path] = std::move(handler);
}

bool HttpServer::run(std::string& error) {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        error = "WSAStartup failed (could not start Windows sockets)";
        return false;
    }
#endif

    socket_t server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == kBadSocket) {
        error = "Could not create a socket";
        return false;
    }

    int reuse = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof reuse);

    sockaddr_in address;
    std::memset(&address, 0, sizeof address);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);  // 127.0.0.1: only this computer can connect
    address.sin_port = htons(static_cast<unsigned short>(port_));

    if (bind(server, reinterpret_cast<sockaddr*>(&address), sizeof address) != 0) {
        error = "Could not bind to port " + std::to_string(port_) +
                ". Is another program (or an old Pharma Guard) already using it? Try:  --port 8081";
        closeSocket(server);
        return false;
    }
    if (listen(server, 16) != 0) {
        error = "listen() failed";
        closeSocket(server);
        return false;
    }

    for (;;) {  // serve forever; press Ctrl+C to stop
        socket_t client = accept(server, nullptr, nullptr);
        if (client == kBadSocket) continue;

        // Do not let a silent client block the server for ever.
#ifdef _WIN32
        DWORD timeoutMs = 5000;
        setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeoutMs), sizeof timeoutMs);
#else
        timeval timeout;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);
#ifdef SO_NOSIGPIPE
        int noSigPipe = 1;
        setsockopt(client, SOL_SOCKET, SO_NOSIGPIPE, &noSigPipe, sizeof noSigPipe);
#endif
#endif

        HttpRequest request;
        int errorStatus = 400;
        if (!readRequest(client, request, errorStatus)) {
            sendResponse(client, errorResponse(errorStatus, "Could not read a complete HTTP request"));
            closeSocket(client);
            continue;
        }

        HttpResponse response;
        if (request.method == "OPTIONS") {
            response = HttpResponse(204, "text/plain", "");  // answer to the browser's CORS "preflight"
        } else {
            auto found = routes_.find(request.method + " " + request.path);
            if (found == routes_.end()) {
                bool pathExists = false;
                for (const auto& entry : routes_) {
                    if (entry.first.substr(entry.first.find(' ') + 1) == request.path) pathExists = true;
                }
                response = pathExists ? errorResponse(405, "Method not allowed for " + request.path)
                                      : errorResponse(404, "No such API route: " + request.path);
            } else {
                try {
                    response = found->second(request);
                } catch (const std::exception& e) {
                    response = errorResponse(500, std::string("Internal error: ") + e.what());
                }
            }
        }
        std::cout << "[HTTP] " << request.method << " " << request.path << " -> " << response.status << std::endl;
        sendResponse(client, response);
        closeSocket(client);
    }
}
