// ws_protocol.c
#include "network/ws_protocol.h"
#include "utils/logger.h"
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

// Hàm gửi message
ssize_t send_message(SOCKET sock, message_t *msg) {
    size_t total = 0;
    size_t to_send = sizeof(message_t);

    log_debug("Sending message type=%d to socket=%llu (%zu bytes)", 
              msg->type, (unsigned long long)sock, to_send);

    // Sao chép và chuyển int sang network order
    message_t tmp;
    memcpy(&tmp, msg, sizeof(message_t));
    if (msg->type == MSG_PLAYER_MOVE || msg->type == MSG_MOVE_RESULT) {
        tmp.payload.move.x = htonl(msg->payload.move.x);
        tmp.payload.move.y = htonl(msg->payload.move.y);
        if (msg->type == MSG_MOVE_RESULT)
            tmp.payload.move_res.hit = htonl(msg->payload.move_res.hit);
    }

    char *buf = (char*)&tmp;
    while (to_send > 0) {
        int n = send(sock, buf + total, to_send, 0);
        if (n <= 0) {
            int err = WSAGetLastError();
            log_error("send() failed: n=%d, error=%d, socket=%llu", n, err, (unsigned long long)sock);
            return n;
        }
        total += n;
        to_send -= n;
        log_debug("Sent %d bytes, %zu remaining", n, to_send);
    }
    
    log_debug("Message sent successfully: %zu bytes to socket=%llu", total, (unsigned long long)sock);
    return total;
}

// Hàm nhận message
ssize_t recv_message(SOCKET sock, message_t *msg) {
    size_t total = 0;
    size_t to_read = sizeof(message_t);
    char *buf = (char*)msg;

    log_debug("Receiving message from socket=%llu (%zu bytes expected)", 
              (unsigned long long)sock, to_read);

    while (to_read > 0) {
        int n = recv(sock, buf + total, to_read, 0);
        if (n == 0) {
            log_info("recv() returned 0: client closed connection (socket=%llu)", (unsigned long long)sock);
            return 0;
        }
        if (n < 0) {
            int err = WSAGetLastError();
            log_error("recv() failed: n=%d, error=%d, socket=%llu", n, err, (unsigned long long)sock);
            return -1;
        }
        total += n;
        to_read -= n;
        log_debug("Received %d bytes, %zu remaining", n, to_read);
    }

    // convert các field int từ network byte order
    if (msg->type == MSG_PLAYER_MOVE || msg->type == MSG_MOVE_RESULT) {
        msg->payload.move.x = ntohl(msg->payload.move.x);
        msg->payload.move.y = ntohl(msg->payload.move.y);
        if (msg->type == MSG_MOVE_RESULT)
            msg->payload.move_res.hit = ntohl(msg->payload.move_res.hit);
    }

    log_debug("Message received successfully: type=%d, %zu bytes from socket=%llu", 
              msg->type, total, (unsigned long long)sock);
    return total;
}

// Hàm khởi tạo Winsock
int init_winsock() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        log_error("WSAStartup failed");
        return -1;
    }
    log_info("Winsock initialized successfully");
    return 0;
}

// Hàm cleanup Winsock
void cleanup_winsock() {
    WSACleanup();
    log_info("Winsock cleaned up");
}