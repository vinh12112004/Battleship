#include "tcp_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>

// ==================== CONNECTION ====================
int tcp_connect(const char *host, uint16_t port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket() failed");
        return -1;
    }
    
    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;

    server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, host, &server_addr.sin_addr) <= 0) {
        perror("inet_pton() failed");
        close(sockfd);
        return -1;
    }
    
    printf("[TCP] Connecting to %s:%d...\n", host, port);
    
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect() failed");
        close(sockfd);
        return -1;
    }
    
    printf("[TCP] Connected successfully (fd=%d)\n", sockfd);
    return sockfd;
}

int tcp_disconnect(int sockfd) {
    if (sockfd < 0) return -1;
    
    printf("[TCP] Closing connection (fd=%d)\n", sockfd);
    close(sockfd);
    return 0;
}

// ==================== SET NON-BLOCKING ====================
int tcp_set_nonblocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl(F_GETFL) failed");
        return -1;
    }
    
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl(F_SETFL) failed");
        return -1;
    }
    
    printf("[TCP] Socket set to non-blocking mode\n");
    return 0;
}

// ==================== MESSAGE HELPERS ====================
void tcp_message_init(tcp_message_t *msg, int32_t type) {
    // Xóa sạch bộ nhớ (đảm bảo padding bằng 0)
    memset(msg, 0, sizeof(tcp_message_t));
    
    // Gán Type trực tiếp (Little Endian / Native)
    msg->type = type;
    
    // Token mặc định là rỗng (do memset), Python sẽ điền sau nếu cần
}

void tcp_message_set_payload(tcp_message_t *msg, const void *data, uint16_t len) {
    if (len > MAX_PAYLOAD_SIZE) {
        fprintf(stderr, "[TCP] Payload too large: %u > %u\n", len, MAX_PAYLOAD_SIZE);
        return;
    }
    
    // Copy dữ liệu vào vùng payload
    memcpy(msg->payload, data, len);
    
    // Không cần set payload_len vì cấu trúc mới là cố định (Fixed Size)
}

// ==================== SEND MESSAGE ====================
int tcp_send(int sockfd, tcp_message_t *msg) {
    if (sockfd < 0) {
        fprintf(stderr, "[TCP] Invalid socket\n");
        return -1;
    }

    ssize_t total_sent = 0;
    // Gửi toàn bộ kích thước cố định (5520 bytes)
    ssize_t remaining = MESSAGE_SIZE;
    uint8_t *buf = (uint8_t*)msg;
    
    while (remaining > 0) {
        ssize_t sent = send(sockfd, buf + total_sent, remaining, 0);
        
        if (sent < 0) {
            if (errno == EINTR) continue;  // Bị ngắt, thử lại
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Socket non-blocking đang bận, chờ xíu
                usleep(1000);  // 1ms
                continue;
            }
            perror("send() failed");
            return -1;
        }
        
        total_sent += sent;
        remaining -= sent;
    }
    
    // In log debug (Không dùng ntohs vì đang dùng Native Endian)
    printf("[TCP] Sent message: type=%d (%zd bytes)\n", msg->type, total_sent);
    
    return 0;
}

// ==================== RECEIVE MESSAGE ====================
int tcp_recv(int sockfd, tcp_message_t *msg) {
    if (sockfd < 0) {
        fprintf(stderr, "[TCP] Invalid socket\n");
        return -1;
    }
    
    memset(msg, 0, sizeof(tcp_message_t));
    
    ssize_t total_recv = 0;
    // Nhận đúng kích thước cố định (5520 bytes)
    ssize_t remaining = MESSAGE_SIZE;
    uint8_t *buf = (uint8_t*)msg;
    
    while (remaining > 0) {
        ssize_t received = recv(sockfd, buf + total_recv, remaining, 0);
        
        if (received < 0) {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Non-blocking, chưa có dữ liệu
                return 0;
            }
            perror("recv() failed");
            return -1;
        }
        
        if (received == 0) {
            // Server đóng kết nối
            printf("[TCP] Connection closed by server\n");
            return -2;
        }
        
        total_recv += received;
        remaining -= received;
    }
    
    printf("[TCP] Received message: type=%d\n", msg->type);
    
    return 1;  // Success
}