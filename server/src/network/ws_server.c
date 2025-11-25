#include "network/ws_server.h"
#include "network/ws_protocol.h"
#include "network/ws_handler.h"
#include "utils/logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define MAX_CLIENTS 100

// ====================== Thread client ======================
void* client_thread(void* arg) {
    int client_sock = (int)(intptr_t)arg;
    message_t msg;

    log_info("Client connected: socket=%d", client_sock);

    struct timeval timeout;
    timeout.tv_sec = 300;  // 5 minutes
    timeout.tv_usec = 0;
    
    if (setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        log_warn("Không thể set socket timeout");
    }
    
    // Perform WebSocket handshake
    if (ws_handshake(client_sock) < 0) {
        log_error("WebSocket handshake failed for socket %d", client_sock);
        close(client_sock);
        return NULL;
    }

    log_info("WebSocket handshake completed for socket %d", client_sock);

    while (1) {
        log_debug("Waiting for WebSocket message from client %d...", client_sock);
        
        int n = ws_recv_message(client_sock, &msg);
        
        if (n == 0) {
            log_info("Client %d closed WebSocket connection gracefully", client_sock);
            break;
        }
        
        if (n < 0) {
            log_error("ws_recv_message error for client %d", client_sock);
            break;
        }

        log_info("Received WebSocket message type=%d from client %d", msg.type, client_sock);

        // Handle message
        handle_message(client_sock, &msg);
        
        log_info("Message handled successfully for client %d, waiting for next message...", client_sock);
    }

    log_info("Client %d disconnected", client_sock);
    ws_close(client_sock, 1000); // Normal closure
    close(client_sock);
    return NULL;
}

// ====================== Setup server ======================
int setup_ws_server(uint16_t port) {
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        log_error("Socket creation failed");
        return -1;
    }

    int opt = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        log_error("Setsockopt failed");
        close(server_sock);
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        log_error("Bind failed");
        close(server_sock);
        return -1;
    }

    if (listen(server_sock, MAX_CLIENTS) < 0) {
        log_error("Listen failed");
        close(server_sock);
        return -1;
    }

    log_info("WebSocket server listening on port %d", port);
    return server_sock;
}

// ====================== Start server ======================
void start_ws_server(uint16_t port) {
    int server_sock = setup_ws_server(port);
    if (server_sock < 0) return;

    log_info("WebSocket server ready to accept connections");

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        
        log_debug("Waiting for new WebSocket client connection...");
        int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_len);
        
        if (client_sock < 0) {
            log_error("Accept failed");
            continue;
        }

        log_info("New WebSocket connection accepted from %s:%d", 
                 inet_ntoa(client_addr.sin_addr), 
                 ntohs(client_addr.sin_port));

        // Create thread for client
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, client_thread, (void*)(intptr_t)client_sock) == 0) {
            log_debug("Thread created for WebSocket client socket=%d", client_sock);
            pthread_detach(thread_id); // Auto cleanup when thread finishes
        } else {
            log_error("Failed to create thread for WebSocket client");
            close(client_sock);
        }
    }

    close(server_sock);
    log_info("WebSocket server shutdown");
}