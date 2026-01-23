#include <iostream>
#include <string>

#include "startup/client.h"
#include "startup/local_battle.h"
#include "startup/server.h"

namespace {
void printUsage(const char* argv0) {
    std::cout << "Usage:\n";
    std::cout << "  " << argv0 << " local [--pets <db>] [--skills <dir>]\n";
    std::cout << "  " << argv0 << " server --port <port> [--pets <db>] [--skills <dir>]\n";
    std::cout << "  " << argv0 << " client --host <host> --port <port>\n";
}

std::string getArg(int& i, int argc, char** argv) {
    if (i + 1 >= argc) return {};
    return argv[++i];
}
} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string mode = argv[1];
    std::string petsDb = "arena/assets/pets/pets.db";
    std::string skillsDir = "arena/assets/skills/skills_src";

    if (mode == "local") {
        for (int i = 2; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--pets") {
                petsDb = getArg(i, argc, argv);
            } else if (arg == "--skills") {
                skillsDir = getArg(i, argc, argv);
            }
        }
        return runLocalBattle(petsDb, skillsDir);
    }

    if (mode == "server") {
        int port = 8080;
        for (int i = 2; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--port") {
                port = std::stoi(getArg(i, argc, argv));
            } else if (arg == "--pets") {
                petsDb = getArg(i, argc, argv);
            } else if (arg == "--skills") {
                skillsDir = getArg(i, argc, argv);
            }
        }
        return runServer(port, petsDb, skillsDir);
    }

    if (mode == "client") {
        std::string host = "127.0.0.1";
        int port = 8080;
        for (int i = 2; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--host") {
                host = getArg(i, argc, argv);
            } else if (arg == "--port") {
                port = std::stoi(getArg(i, argc, argv));
            }
        }
        return runClient(host, port);
    }

    printUsage(argv[0]);
    return 1;
}
