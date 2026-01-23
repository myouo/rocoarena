#include "http_server.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <sstream>
#include <unordered_map>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace {
std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    auto end = s.find_last_not_of(" \t\r\n");
    if (start == std::string::npos) return {};
    return s.substr(start, end - start + 1);
}

std::string getHeaderValue(const std::unordered_map<std::string, std::string>& headers, const std::string& key) {
    auto it = headers.find(toLower(key));
    if (it == headers.end()) return {};
    return it->second;
}

#ifdef _WIN32
using SocketType = SOCKET;
constexpr SocketType kInvalidSocket = INVALID_SOCKET;
#else
using SocketType = int;
constexpr SocketType kInvalidSocket = -1;
#endif

void closeSocket(SocketType fd) {
#ifdef _WIN32
    closesocket(fd);
#else
    close(fd);
#endif
}
} // namespace

HttpServer::HttpServer(int port) : port_(port) {}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::setHandler(Handler handler) {
    handler_ = std::move(handler);
}

bool HttpServer::start(std::string* error) {
    if (running_) return true;

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        if (error) *error = "WSAStartup failed";
        return false;
    }
#endif

    SocketType serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd == kInvalidSocket) {
        if (error) *error = "failed to create socket";
        return false;
    }

    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(static_cast<uint16_t>(port_));

    if (bind(serverFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        if (error) *error = "bind failed";
        closeSocket(serverFd);
        return false;
    }

    if (listen(serverFd, 16) < 0) {
        if (error) *error = "listen failed";
        closeSocket(serverFd);
        return false;
    }

    serverFd_ = static_cast<int>(serverFd);
    running_ = true;
    acceptThread_ = std::thread(&HttpServer::acceptLoop, this);
    return true;
}

void HttpServer::stop() {
    if (!running_) return;
    running_ = false;
    if (serverFd_ != -1) {
        closeSocket(static_cast<SocketType>(serverFd_));
        serverFd_ = -1;
    }
    if (acceptThread_.joinable()) {
        acceptThread_.join();
    }
#ifdef _WIN32
    WSACleanup();
#endif
}

void HttpServer::acceptLoop() {
    while (running_) {
        sockaddr_in clientAddr{};
#ifdef _WIN32
        int len = sizeof(clientAddr);
        SocketType clientFd = accept(static_cast<SocketType>(serverFd_), reinterpret_cast<sockaddr*>(&clientAddr), &len);
#else
        socklen_t len = sizeof(clientAddr);
        SocketType clientFd = accept(static_cast<SocketType>(serverFd_), reinterpret_cast<sockaddr*>(&clientAddr), &len);
#endif
        if (clientFd == kInvalidSocket) {
            continue;
        }

        std::thread([this, clientFd]() {
            std::string raw;
            char buffer[4096];
            int received = 0;
            bool headerParsed = false;
            std::size_t headerEnd = std::string::npos;
            std::size_t contentLength = 0;

            while ((received = recv(clientFd, buffer, sizeof(buffer), 0)) > 0) {
                raw.append(buffer, buffer + received);
                if (!headerParsed) {
                    headerEnd = raw.find("\r\n\r\n");
                    if (headerEnd != std::string::npos) {
                        headerParsed = true;
                        std::string headerPart = raw.substr(0, headerEnd + 4);
                        std::istringstream hs(headerPart);
                        std::string line;
                        while (std::getline(hs, line)) {
                            if (line.find("Content-Length:") != std::string::npos ||
                                line.find("content-length:") != std::string::npos) {
                                auto pos = line.find(':');
                                if (pos != std::string::npos) {
                                    std::string value = trim(line.substr(pos + 1));
                                    try {
                                        contentLength = static_cast<std::size_t>(std::stoul(value));
                                    } catch (...) {
                                        contentLength = 0;
                                    }
                                }
                            }
                        }
                    }
                }

                if (headerParsed) {
                    std::size_t totalNeeded = headerEnd + 4 + contentLength;
                    if (raw.size() >= totalNeeded) {
                        break;
                    }
                }
            }

            HttpResponse resp = handleRawRequest(raw);

            std::ostringstream oss;
            oss << "HTTP/1.1 " << resp.status << " OK\r\n";
            oss << "Content-Type: " << resp.contentType << "\r\n";
            oss << "Content-Length: " << resp.body.size() << "\r\n";
            oss << "Connection: close\r\n\r\n";
            oss << resp.body;
            std::string out = oss.str();
            send(clientFd, out.c_str(), static_cast<int>(out.size()), 0);
            closeSocket(clientFd);
        }).detach();
    }
}

HttpResponse HttpServer::handleRawRequest(const std::string& raw) const {
    std::istringstream iss(raw);
    std::string line;
    if (!std::getline(iss, line)) {
        return { 400, "{\"error\":\"invalid request\"}" };
    }

    std::istringstream lineStream(line);
    HttpRequest req;
    lineStream >> req.method;
    std::string path;
    lineStream >> path;

    auto qpos = path.find('?');
    if (qpos != std::string::npos) {
        req.path = path.substr(0, qpos);
        req.query = path.substr(qpos + 1);
    } else {
        req.path = path;
    }

    std::unordered_map<std::string, std::string> headers;
    while (std::getline(iss, line)) {
        if (line == "\r" || line.empty()) {
            break;
        }
        auto pos = line.find(':');
        if (pos == std::string::npos) continue;
        std::string key = toLower(trim(line.substr(0, pos)));
        std::string value = trim(line.substr(pos + 1));
        headers[key] = value;
    }

    std::string contentLenStr = getHeaderValue(headers, "content-length");
    int contentLen = 0;
    if (!contentLenStr.empty()) {
        try {
            contentLen = std::stoi(contentLenStr);
        } catch (...) {
            contentLen = 0;
        }
    }

    if (contentLen > 0) {
        std::string body;
        body.resize(static_cast<std::size_t>(contentLen));
        iss.read(body.data(), contentLen);
        req.body = body;
    }

    if (!handler_) {
        return { 500, "{\"error\":\"no handler\"}" };
    }

    return handler_(req);
}
