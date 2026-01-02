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
    shutdown(sockfd, SHUT_RDWR);
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
    memset(msg, 0, sizeof(tcp_message_t));
    msg->type = type;
}

void tcp_message_set_payload(tcp_message_t *msg, const void *data, uint16_t len) {
    if (len > MAX_PAYLOAD_SIZE) {
        fprintf(stderr, "[TCP] Payload too large: %u > %u\n", len, MAX_PAYLOAD_SIZE);
        return;
    }
    memcpy(msg->payload, data, len);
}

// ==================== SEND MESSAGE ====================
int tcp_send(int sockfd, tcp_message_t *msg) {
    if (sockfd < 0) {
        fprintf(stderr, "[TCP] Invalid socket\n");
        return -1;
    }

    // Step 1: Send 4-byte length header (network byte order)
    uint32_t msg_len = htonl(MESSAGE_SIZE);
    
    ssize_t sent = send(sockfd, &msg_len, sizeof(msg_len), 0);
    if (sent != sizeof(msg_len)) {
        perror("send() length header failed");
        return -1;
    }
    
    printf("[TCP] Sent length header: %u bytes\n", MESSAGE_SIZE);
    
    // Step 2: Send message body
    ssize_t total_sent = 0;
    ssize_t remaining = MESSAGE_SIZE;
    uint8_t *buf = (uint8_t*)msg;
    
    while (remaining > 0) {
        sent = send(sockfd, buf + total_sent, remaining, 0);
        
        if (sent < 0) {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep(1000);
                continue;
            }
            perror("send() body failed");
            return -1;
        }
        
        total_sent += sent;
        remaining -= sent;
    }
    
    printf("[TCP] Sent message body: type=%d (%zd bytes total)\n", 
           msg->type, total_sent);
    
    return 0;
}

// ==================== RECEIVE MESSAGE ====================
int tcp_recv(int sockfd, tcp_message_t *msg) {
    if (sockfd < 0) {
        fprintf(stderr, "[TCP] Invalid socket\n");
        return -1;
    }
    
    memset(msg, 0, sizeof(tcp_message_t));
    
    // Step 1: Read 4-byte length header
    uint32_t msg_len = 0;
    ssize_t bytes_received = 0;
    ssize_t remaining = sizeof(msg_len);
    uint8_t *len_buf = (uint8_t*)&msg_len;
    
    while (remaining > 0) {
        ssize_t received = recv(sockfd, len_buf + bytes_received, remaining, 0);
        
        if (received < 0) {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return 0; // Non-blocking, no data yet
            }
            perror("recv() length header failed");
            return -1;
        }
        
        if (received == 0) {
            printf("[TCP] Connection closed by server\n");
            return -2;
        }
        
        bytes_received += received;
        remaining -= received;
    }
    
    msg_len = ntohl(msg_len); // Network to host byte order
    printf("[TCP] Received length header: %u bytes\n", msg_len);
    
    // Step 2: Validate message size
    if (msg_len != MESSAGE_SIZE) {
        fprintf(stderr, "[TCP] Invalid message size: expected=%u, got=%u\n", 
                MESSAGE_SIZE, msg_len);
        return -1;
    }
    
    // Step 3: Read message body
    bytes_received = 0;
    remaining = MESSAGE_SIZE;
    uint8_t *buf = (uint8_t*)msg;
    
    while (remaining > 0) {
        ssize_t received = recv(sockfd, buf + bytes_received, remaining, 0);
        
        if (received < 0) {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep(1000);
                continue;
            }
            perror("recv() body failed");
            return -1;
        }
        
        if (received == 0) {
            printf("[TCP] Connection closed during body read\n");
            return -2;
        }
        
        bytes_received += received;
        remaining -= received;
    }
    
    printf("[TCP] Received message: type=%d (%zd bytes)\n", 
           msg->type, bytes_received);
    
    return 1; // Success
}