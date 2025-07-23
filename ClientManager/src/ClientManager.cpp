#include "ClientManager.h"

#include <cstring>
#include <iostream>

ClientManager::ClientManager() : running(false), serverSocket(INVALID_SOCKET) {}

ClientManager::~ClientManager() {
    stop();
}

void ClientManager::start(int port) {
    // windows initialisation
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fileLogger.log("WSAStartup failed", LogLevel::ERR);
        return;
    }
#endif

    /*
     serverSocket init:
     * AF_INET      - ipV4
     * SOCK_STREAM  - TCP connection, alternatives:
     * * SOCK_DGRAM - UDP connection
     * * SOCK_RAW   - working directly with network packets
     */
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        fileLogger.log("Socket creation failed: " + std::to_string(getError()), LogLevel::ERR);
        return;
    }

    sockaddr_in serverAddr{};
    /*
     * AF_INET    - ipV4
     * INADDR_ANY - (0.0.0.0) all ip addresses
     * htons      - (little endian <-> big endian)
     */
    serverAddr.sin_family      = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port        = htons(port);

    // attaching the address to the socket
    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        fileLogger.log("Bind failed: " + std::to_string(getError()), LogLevel::ERR);
        close(serverSocket);
        return;
    }

    // we are listening for connections, using the maximum queue size of SOMAXCONN
    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        fileLogger.log("Listen failed: " + std::to_string(getError()), LogLevel::ERR);
        close(serverSocket);
        return;
    }

    running = true;
    fileLogger.log("Server started on port " + std::to_string(port), LogLevel::INFO);
}

void ClientManager::stop() {
    if (!running)
        return;

    running = false;

    // Close server socket to unblock accept call
    if (serverSocket != INVALID_SOCKET) {
        close(serverSocket);
        serverSocket = INVALID_SOCKET;
    }

    if (connectionThread.joinable()) {
        connectionThread.join();
    }

    // Close all client sockets
    std::lock_guard<std::mutex> lock(clientsMutex);
    for (auto socket : clients) {
        close(socket);
    }
    clients.clear();

    // Wait for all client threads to finish
    for (auto& thread : clientThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    clientThreads.clear();

#ifdef _WIN32
    WSACleanup();
#endif

    fileLogger.log("Server stopped", LogLevel::INFO);
}

void ClientManager::broadcast(const std::string& message, socket_t excludeSocket) {
    std::lock_guard<std::mutex> lock(clientsMutex);

    for (auto socket : clients) {
        if (socket != excludeSocket) {
            if (send(socket, message.c_str(), message.size(), 0) == SOCKET_ERROR) {
                fileLogger.log(
                    "Send failed to socket " + std::to_string(socket) + ", error: " + std::to_string(getError()),
                    LogLevel::WARNING);
            }
        }
    }
}

void ClientManager::handleNewConnections() {
    while (running) {
        sockaddr_in clientAddr{};
        socklen_t clientAddrLen = sizeof(clientAddr);

        socket_t clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSocket == INVALID_SOCKET) {
            if (running) {
                fileLogger.log("Accept failed: " + std::to_string(getError()), LogLevel::ERR);
            }
            continue;
        }

        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
        fileLogger.log(
            "New connection from " + std::string(clientIP) + ":" + std::to_string(ntohs(clientAddr.sin_port)),
            LogLevel::INFO);

        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients.push_back(clientSocket);
        }

        clientThreads.emplace_back(&ClientManager::handleClient, this, clientSocket);
    }
}

void ClientManager::handleClient(socket_t clientSocket) {
    char buffer[BUFFER_SIZE];
    int bytesReceived;

    while (running) {
        bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesReceived <= 0) {
            if (bytesReceived == 0) {
                fileLogger.log("Client disconnected: " + std::to_string(clientSocket), LogLevel::INFO);
            } else {
                fileLogger.log(
                    "Recv failed for socket " + std::to_string(clientSocket) + ", error: " + std::to_string(getError()),
                    LogLevel::WARNING);
            }
            break;
        }

        std::string message(buffer, bytesReceived);
        fileLogger.log("Received from " + std::to_string(clientSocket) + ": " + message, LogLevel::MESSAGE);
        broadcast(message);
    }

    // Remove client from list
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        auto it = std::find(clients.begin(), clients.end(), clientSocket);
        if (it != clients.end()) {
            clients.erase(it);
        }
    }

    close(clientSocket);
}

void ClientManager::sleep(int milliseconds) {
#ifdef _WIN32
    ::Sleep(milliseconds);
#else
    usleep(milliseconds * 1000);
#endif
}