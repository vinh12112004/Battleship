#include "network/ws_server.h"
#include "network/ws_protocol.h"
#include "network/ws_handler.h"
#include "utils/logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <winsock2.h>

#define MAX_CLIENTS 100

// ====================== Thread client ======================
DWORD WINAPI client_thread(LPVOID arg) {
    SOCKET client_sock = (SOCKET)(intptr_t)arg;
    message_t msg;

    log_info("Client connected: socket=%llu", (unsigned long long)client_sock);

    DWORD timeout = 300000; // 5 minutes
    if (setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)) < 0) {
        log_warn("Không thể set socket timeout");
    }
    
    // Perform WebSocket handshake
    if (ws_handshake(client_sock) < 0) {
        log_error("WebSocket handshake failed for socket %llu", (unsigned long long)client_sock);
        closesocket(client_sock);
        return 1;
    }

    log_info("WebSocket handshake completed for socket %llu", (unsigned long long)client_sock);

    while (1) {
        log_debug("Waiting for WebSocket message from client %llu...", (unsigned long long)client_sock);
        
        int n = ws_recv_message(client_sock, &msg);
        
        if (n == 0) {
            log_info("Client %llu closed WebSocket connection gracefully", (unsigned long long)client_sock);
            break;
        }
        
        if (n < 0) {
            log_error("ws_recv_message error for client %llu: %d", (unsigned long long)client_sock, WSAGetLastError());
            break;
        }

        log_info("Received WebSocket message type=%d from client %llu", msg.type, (unsigned long long)client_sock);

        // Handle message
        handle_message((int)client_sock, &msg);
        
        log_info("Message handled successfully for client %llu, waiting for next message...", (unsigned long long)client_sock);
    }

    log_info("Client %llu disconnected", (unsigned long long)client_sock);
    ws_close(client_sock, 1000); // Normal closure
    closesocket(client_sock);
    return 0;
}

// ====================== Setup server ======================
int setup_ws_server(uint16_t port) {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        log_error("WSAStartup failed");
        return -1;
    }

    SOCKET server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_sock == INVALID_SOCKET) {
        log_error("Socket creation failed");
        WSACleanup();
        return -1;
    }

    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        log_error("Bind failed: %d", WSAGetLastError());
        closesocket(server_sock);
        WSACleanup();
        return -1;
    }

    if (listen(server_sock, MAX_CLIENTS) == SOCKET_ERROR) {
        log_error("Listen failed: %d", WSAGetLastError());
        closesocket(server_sock);
        WSACleanup();
        return -1;
    }

    log_info("WebSocket server listening on port %d", port);
    return (int)server_sock;
}

// ====================== Start server ======================
void start_ws_server(uint16_t port) {
    SOCKET server_sock = (SOCKET)setup_ws_server(port);
    if (server_sock == INVALID_SOCKET) return;

    log_info("WebSocket server ready to accept connections");

    while (1) {
        struct sockaddr_in client_addr;
        int addr_len = sizeof(client_addr);
        
        log_debug("Waiting for new WebSocket client connection...");
        SOCKET client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_len);
        
        if (client_sock == INVALID_SOCKET) {
            log_error("Accept failed: %d", WSAGetLastError());
            continue;
        }

        log_info("New WebSocket connection accepted from %s:%d", 
                 inet_ntoa(client_addr.sin_addr), 
                 ntohs(client_addr.sin_port));

        // Create thread for client
        DWORD tid;
        HANDLE hThread = CreateThread(NULL, 0, client_thread, (LPVOID)(intptr_t)client_sock, 0, &tid);
        if (hThread) {
            log_debug("Thread created for WebSocket client socket=%llu, tid=%lu", 
                     (unsigned long long)client_sock, tid);
            CloseHandle(hThread);
        } else {
            log_error("Failed to create thread for WebSocket client");
            closesocket(client_sock);
        }
    }

    closesocket(server_sock);
    WSACleanup();
    log_info("WebSocket server shutdown");
}