#pragma once

#include <string>

#include <nlohmann/json.hpp>

struct HttpClientResponse {
    int status = 0;
    std::string body;
};

class HttpClient {
  public:
    HttpClient(std::string host, int port);

    HttpClientResponse get(const std::string& path);
    HttpClientResponse post(const std::string& path, const nlohmann::json& payload);

  private:
    HttpClientResponse sendRequest(const std::string& method, const std::string& path, const std::string& body);

    std::string host_;
    int port_ = 0;
};
