#include "network/tcp_protocol.h"
#include "utils/logger.h"
#include <string.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdint.h>

// Send TCP message with length prefix
ssize_t tcp_send_message(int sock, message_t *msg) {
    // Step 1: Send 4-byte length header (network byte order)
    uint32_t msg_len = htonl(sizeof(message_t));
    
    ssize_t sent = send(sock, &msg_len, sizeof(msg_len), 0);
    if (sent != sizeof(msg_len)) {
        log_error("Failed to send message length header: sent=%zd", sent);
        return -1;
    }
    
    log_debug("Sent length header: %u bytes", ntohl(msg_len));
    
    // Step 2: Send message body
    sent = send(sock, msg, sizeof(message_t), 0);
    if (sent != sizeof(message_t)) {
        log_error("Failed to send message body: sent=%zd, expected=%lu", sent, sizeof(message_t));
        return -1;
    }
    
    log_debug("Sent message body: %zd bytes (type=%d)", sent, msg->type);
    
    return sizeof(message_t);
}

// Receive TCP message with length prefix
ssize_t tcp_recv_message(int sock, message_t *msg) {
    // Step 1: Read 4-byte length header
    uint32_t msg_len = 0;
    ssize_t bytes_received = recv(sock, &msg_len, sizeof(msg_len), MSG_WAITALL);
    
    if (bytes_received == 0) {
        log_info("Client closed connection normally");
        return 0;
    }
    
    if (bytes_received != sizeof(msg_len)) {
        log_error("Failed to receive length header: ret=%zd, errno=%d (%s)", 
                  bytes_received, errno, strerror(errno));
        return -1;
    }
    
    msg_len = ntohl(msg_len); // Network to host byte order
    
    log_debug("Received length header: %u bytes", msg_len);
    
    // Step 2: Validate message size
    if (msg_len != sizeof(message_t)) {
        log_error("Invalid message size: expected=%lu, got=%u", sizeof(message_t), msg_len);
        return -1;
    }
    
    if (msg_len > 1048576) { // Safety check: max 1MB
        log_error("Message too large: %u bytes (max 1MB)", msg_len);
        return -1;
    }
    
    // Step 3: Read message body
    bytes_received = recv(sock, msg, msg_len, MSG_WAITALL);
    
    if (bytes_received != (ssize_t)msg_len) {
        log_error("Failed to receive message body: expected=%u, got=%zd, errno=%d (%s)",
                  msg_len, bytes_received, errno, strerror(errno));
        return -1;
    }
    
    log_debug("Received message: %zd bytes (type=%d)", bytes_received, msg->type);
    
    return bytes_received;
}

// Close TCP connection gracefully
void tcp_close(int sock) {
    if (sock > 0) {
        shutdown(sock, SHUT_RDWR);
        close(sock);
        log_debug("TCP socket %d closed", sock);
    }
}