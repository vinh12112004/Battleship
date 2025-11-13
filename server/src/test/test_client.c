#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include "network/ws_protocol.h"

#pragma comment(lib, "ws2_32.lib")

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        printf("Socket creation failed\n");
        return 1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Connect failed\n");
        closesocket(sock);
        return 1;
    }

    // Gửi MSG_LOGIN
    message_t msg;
    msg.type = MSG_LOGIN;
    strncpy(msg.payload.auth.username, "testuser", 32);
    strncpy(msg.payload.auth.password, "password123", 32);

    send_message(sock, &msg);

    // Nhận phản hồi
    message_t resp;
    recv_message(sock, &resp);

    if (resp.type == MSG_AUTH_SUCCESS) {
        printf("Login success! Token: %s\n", resp.payload.auth_suc.token);
    } else if (resp.type == MSG_AUTH_FAILED) {
        printf("Login failed: %s\n", resp.payload.auth_fail.reason);
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
