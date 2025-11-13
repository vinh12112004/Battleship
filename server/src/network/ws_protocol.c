// ws_protocol.c
#include "network/ws_protocol.h"
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

// Hàm gửi message
ssize_t send_message(SOCKET sock, message_t *msg) {
    // Sao chép message để chuyển đổi network byte order
    message_t tmp;
    memcpy(&tmp, msg, sizeof(message_t));

    if (msg->type == MSG_PLAYER_MOVE || msg->type == MSG_MOVE_RESULT) {
        tmp.payload.move.x = htonl(msg->payload.move.x);
        tmp.payload.move.y = htonl(msg->payload.move.y);
        if (msg->type == MSG_MOVE_RESULT)
            tmp.payload.move_res.hit = htonl(msg->payload.move_res.hit);
    }
    printf("Sending token: %s\n", tmp.payload.auth_suc.token);
    return send(sock, (const char*)&tmp, sizeof(message_t), 0);
}

// Hàm nhận message
ssize_t recv_message(SOCKET sock, message_t *msg) {
    ssize_t n = recv(sock, (char*)msg, sizeof(message_t), 0);
    if (n <= 0) return n;

    // Chuyển các trường int từ network byte order về host byte order
    if (msg->type == MSG_PLAYER_MOVE || msg->type == MSG_MOVE_RESULT) {
        msg->payload.move.x = ntohl(msg->payload.move.x);
        msg->payload.move.y = ntohl(msg->payload.move.y);
        if (msg->type == MSG_MOVE_RESULT)
            msg->payload.move_res.hit = ntohl(msg->payload.move_res.hit);
    }
    return n;
}

// Hàm khởi tạo Winsock (nên gọi 1 lần khi chương trình bắt đầu)
int init_winsock() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return -1;
    }
    return 0;
}

// Hàm cleanup Winsock (gọi khi chương trình kết thúc)
void cleanup_winsock() {
    WSACleanup();
}
