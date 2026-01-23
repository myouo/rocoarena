#include "cli_helpers.h"

#include <iostream>
#include <string>
#include <thread>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <sys/select.h>
#include <unistd.h>
#endif

std::optional<std::string> readLineWithTimeout(std::chrono::milliseconds timeout) {
#ifdef _WIN32
    auto deadline = std::chrono::steady_clock::now() + timeout;
    std::string line;
    while (std::chrono::steady_clock::now() < deadline) {
        if (_kbhit()) {
            int ch = _getch();
            if (ch == '\r' || ch == '\n') {
                std::cout << "\n";
                return line;
            }
            if (ch == '\b') {
                if (!line.empty()) {
                    line.pop_back();
                    std::cout << "\b \b";
                }
                continue;
            }
            line.push_back(static_cast<char>(ch));
            std::cout << static_cast<char>(ch);
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
    return std::nullopt;
#else
    fd_set set;
    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);

    struct timeval tv;
    tv.tv_sec = static_cast<long>(timeout.count() / 1000);
    tv.tv_usec = static_cast<long>((timeout.count() % 1000) * 1000);

    int res = select(STDIN_FILENO + 1, &set, nullptr, nullptr, &tv);
    if (res <= 0) {
        return std::nullopt;
    }
    std::string line;
    if (!std::getline(std::cin, line)) {
        return std::nullopt;
    }
    return line;
#endif
}

std::optional<std::string> readLineWithTimeoutCountdown(std::chrono::seconds timeout) {
    auto printRemaining = [](long remaining) {
        std::cout << "\rTime left: " << remaining << "s   " << std::flush;
    };

#ifdef _WIN32
    auto deadline = std::chrono::steady_clock::now() + timeout;
    std::string line;
    long lastShown = -1;
    while (std::chrono::steady_clock::now() < deadline) {
        auto now = std::chrono::steady_clock::now();
        auto remaining = std::chrono::duration_cast<std::chrono::seconds>(deadline - now).count();
        if (remaining != lastShown) {
            printRemaining(remaining);
            lastShown = remaining;
        }

        if (_kbhit()) {
            int ch = _getch();
            if (ch == '\r' || ch == '\n') {
                std::cout << "\n";
                return line;
            }
            if (ch == '\b') {
                if (!line.empty()) {
                    line.pop_back();
                    std::cout << "\b \b";
                }
                continue;
            }
            line.push_back(static_cast<char>(ch));
            std::cout << static_cast<char>(ch);
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
    std::cout << "\n";
    return std::nullopt;
#else
    long remaining = static_cast<long>(timeout.count());
    while (remaining > 0) {
        printRemaining(remaining);

        fd_set set;
        FD_ZERO(&set);
        FD_SET(STDIN_FILENO, &set);

        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        int res = select(STDIN_FILENO + 1, &set, nullptr, nullptr, &tv);
        if (res > 0) {
            std::string line;
            if (!std::getline(std::cin, line)) {
                std::cout << "\n";
                return std::nullopt;
            }
            std::cout << "\n";
            return line;
        }

        --remaining;
    }
    std::cout << "\n";
    return std::nullopt;
#endif
}
