#include "network/ws_server.h"
#include "network/ws_protocol.h"
#include "network/ws_handler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

#define MAX_CLIENTS 100

// ====================== Thread client ======================
DWORD WINAPI client_thread(LPVOID arg) {
    SOCKET client_sock = (SOCKET)(intptr_t)arg;
    message_t msg;

    printf("Client connected: %llu\n", (unsigned long long)client_sock);

    while (1) {
        int n = recv_message(client_sock, &msg);
        if (n <= 0) {
            printf("Client disconnected: %llu\n", (unsigned long long)client_sock);
            break;
        }

        // Xử lý message
        handle_message((int)client_sock, &msg);
    }

    closesocket(client_sock);
    return 0;
}

// ====================== Setup server ======================
int setup_ws_server(uint16_t port) {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("WSAStartup failed\n");
        return -1;
    }

    SOCKET server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_sock == INVALID_SOCKET) {
        printf("Socket creation failed\n");
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
        printf("Bind failed\n");
        closesocket(server_sock);
        WSACleanup();
        return -1;
    }

    if (listen(server_sock, MAX_CLIENTS) == SOCKET_ERROR) {
        printf("Listen failed\n");
        closesocket(server_sock);
        WSACleanup();
        return -1;
    }

    printf("Server listening on port %d\n", port);
    return (int)server_sock;
}

// ====================== Start server ======================
void start_ws_server(uint16_t port) {
    SOCKET server_sock = (SOCKET)setup_ws_server(port);
    if (server_sock == INVALID_SOCKET) return;

    while (1) {
        struct sockaddr_in client_addr;
        int addr_len = sizeof(client_addr);
        SOCKET client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_len);
        if (client_sock == INVALID_SOCKET) {
            printf("Accept failed\n");
            continue;
        }

        // Tạo thread cho client
        DWORD tid;
        HANDLE hThread = CreateThread(NULL, 0, client_thread, (LPVOID)(intptr_t)client_sock, 0, &tid);
        if (hThread) CloseHandle(hThread);
    }

    closesocket(server_sock);
    WSACleanup();
}
