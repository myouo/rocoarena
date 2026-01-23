#pragma once

#include <functional>
#include <string>
#include <thread>

#include <nlohmann/json.hpp>

struct HttpRequest {
    std::string method;
    std::string path;
    std::string body;
    std::string query;
};

struct HttpResponse {
    int status = 200;
    std::string body;
    std::string contentType = "application/json";
};

class HttpServer {
  public:
    using Handler = std::function<HttpResponse(const HttpRequest&)>;

    explicit HttpServer(int port);
    ~HttpServer();

    void setHandler(Handler handler);
    bool start(std::string* error = nullptr);
    void stop();

  private:
    void acceptLoop();
    HttpResponse handleRawRequest(const std::string& raw) const;

    int port_ = 0;
    int serverFd_ = -1;
    bool running_ = false;
    Handler handler_;
    std::thread acceptThread_;
};
