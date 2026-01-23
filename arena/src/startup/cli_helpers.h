#pragma once

#include <chrono>
#include <optional>
#include <string>

std::optional<std::string> readLineWithTimeout(std::chrono::milliseconds timeout);
std::optional<std::string> readLineWithTimeoutCountdown(std::chrono::seconds timeout);
