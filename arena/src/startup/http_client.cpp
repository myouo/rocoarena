#include "http_client.h"

#include <cstring>
#include <sstream>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace {
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

HttpClient::HttpClient(std::string host, int port) : host_(std::move(host)), port_(port) {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

HttpClientResponse HttpClient::get(const std::string& path) {
    return sendRequest("GET", path, "");
}

HttpClientResponse HttpClient::post(const std::string& path, const nlohmann::json& payload) {
    return sendRequest("POST", path, payload.dump());
}

HttpClientResponse HttpClient::sendRequest(const std::string& method, const std::string& path,
                                           const std::string& body) {
    HttpClientResponse resp;

    SocketType sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == kInvalidSocket) {
        resp.status = 0;
        resp.body = "socket failed";
        return resp;
    }

    struct hostent* server = gethostbyname(host_.c_str());
    if (!server) {
        resp.status = 0;
        resp.body = "host not found";
        closeSocket(sock);
        return resp;
    }

    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(static_cast<uint16_t>(port_));
    std::memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);

    if (connect(sock, reinterpret_cast<sockaddr*>(&serv_addr), sizeof(serv_addr)) < 0) {
        resp.status = 0;
        resp.body = "connect failed";
        closeSocket(sock);
        return resp;
    }

    std::ostringstream oss;
    oss << method << " " << path << " HTTP/1.1\r\n";
    oss << "Host: " << host_ << "\r\n";
    if (method == "POST") {
        oss << "Content-Type: application/json\r\n";
        oss << "Content-Length: " << body.size() << "\r\n";
    }
    oss << "Connection: close\r\n\r\n";
    if (method == "POST") {
        oss << body;
    }

    std::string request = oss.str();
    send(sock, request.c_str(), static_cast<int>(request.size()), 0);

    std::string raw;
    char buffer[4096];
    int received = 0;
    while ((received = recv(sock, buffer, sizeof(buffer), 0)) > 0) {
        raw.append(buffer, buffer + received);
    }

    closeSocket(sock);

    auto pos = raw.find("\r\n\r\n");
    if (pos == std::string::npos) {
        resp.status = 0;
        resp.body = raw;
        return resp;
    }

    std::string header = raw.substr(0, pos);
    resp.body = raw.substr(pos + 4);

    std::istringstream headerStream(header);
    std::string statusLine;
    std::getline(headerStream, statusLine);
    std::istringstream statusStream(statusLine);
    std::string httpVersion;
    statusStream >> httpVersion >> resp.status;

    return resp;
}
