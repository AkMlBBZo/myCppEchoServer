#ifndef CLIENT_MANAGER_H
#define CLIENT_MANAGER_H

// Socket libs init
#ifdef _WIN32
#include <winsock2.h>
// pupupu
#include <windows.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#define close closesocket
typedef SOCKET socket_t;
#define getError() WSAGetLastError()
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
constexpr int SOCKET_ERROR   = -1;
constexpr int INVALID_SOCKET = -1;
typedef int socket_t;
#define getError() errno
#endif  // _WIN32

#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "Logger.h"
#include "config.h"

class ClientManager {
public:
    ClientManager();
    ~ClientManager();

    void start(int port);
    void stop();
    void broadcast(const std::string& message, socket_t excludeSocket = INVALID_SOCKET);

    static void sleep(int milliseconds);

private:
    void handleNewConnections();
    void handleClient(socket_t clientSocket);
    void cleanupDisconnectedClients();

    std::vector<socket_t> clients;
    std::mutex clientsMutex;
    std::atomic<bool> running;
    socket_t serverSocket;
    std::thread connectionThread;
    std::vector<std::thread> clientThreads;

    static inline constexpr size_t BUFFER_SIZE = 4096;
    static inline auto fileLogger              = AsyncLogger(std::make_unique<FileLogger>(CLIENT_MANAGER_LOG_FILE));
};

#endif  // CLIENT_MANAGER_H