#include "network/ws_server.h"
#include "network/ws_protocol.h"
#include "network/ws_handler.h"
#include "utils/logger.h"
#include "matchmaking/matcher.h"
#include "game/game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include "matchmaking/challenge_manager.h"

#define MAX_CLIENTS 100

typedef struct {
    int socket;
    char user_id[64];
    bool authenticated;
} client_info_t;

static client_info_t g_clients[MAX_CLIENTS];
static pthread_mutex_t g_clients_mutex = PTHREAD_MUTEX_INITIALIZER;

// ✅ Register client khi login thành công
void client_register(int client_sock, const char *user_id) {
    pthread_mutex_lock(&g_clients_mutex);

    // ✅ Step 1: Check if user already registered (including disconnected with socket=-1)
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (g_clients[i].authenticated && 
            strcmp(g_clients[i].user_id, user_id) == 0) {
            
            int old_socket = g_clients[i].socket;
            
            if (old_socket == -1) {
                // User was disconnected, now reconnecting
                log_info("✅ [CLIENT_REGISTER] User %s reconnected (new socket=%d)", 
                         user_id, client_sock);
            } else if (old_socket != client_sock) {
                // User logging in from different socket
                log_warn("🔄 [CLIENT_REGISTER] User %s already registered (old socket=%d → new socket=%d)", 
                         user_id, old_socket, client_sock);
                
                // Close old socket if still open
                if (old_socket > 0) {
                    log_info("   Closing old socket %d", old_socket);
                    close(old_socket);
                }
            } else {
                // Same socket, same user (redundant call)
                log_info("✅ [CLIENT_REGISTER] User %s already at socket %d", 
                         user_id, client_sock);
            }
            
            // Update to new socket
            g_clients[i].socket = client_sock;
            
            // Update user status to online
            user_update_status(user_id, "online");
            
            pthread_mutex_unlock(&g_clients_mutex);
            return;
        }
    }
    
    // ✅ Step 2: User not found → Register new entry
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (g_clients[i].socket == 0 || g_clients[i].socket == -1) {
            g_clients[i].socket = client_sock;
            strncpy(g_clients[i].user_id, user_id, 63);
            g_clients[i].user_id[63] = '\0';
            g_clients[i].authenticated = true;
            
            log_info("✅ [CLIENT_REGISTER] New registration at slot %d: socket=%d, user_id=%s", 
                     i, client_sock, user_id);
            
            // Set user status to online
            user_update_status(user_id, "online");
            
            pthread_mutex_unlock(&g_clients_mutex);
            return;
        }
    }
    
    //Step 3: Registry full
    log_error("❌ [CLIENT_REGISTER] Registry FULL! Cannot register user_id=%s", 
              user_id);
    
    pthread_mutex_unlock(&g_clients_mutex);
}

int get_socket_by_user_id(const char *user_id) {
    if (!user_id) {
        log_error("get_socket_by_user_id: user_id is NULL");
        return -1;
    }
    
    pthread_mutex_lock(&g_clients_mutex);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (g_clients[i].authenticated && 
            g_clients[i].socket > 0 &&
            strcmp(g_clients[i].user_id, user_id) == 0) {
            
            int socket = g_clients[i].socket;
            pthread_mutex_unlock(&g_clients_mutex);
            
            log_debug("Found socket %d for user_id: %s", socket, user_id);
            return socket;
        }
    }
    
    pthread_mutex_unlock(&g_clients_mutex);
    log_warn("Socket not found for user_id: %s", user_id);
    return -1;
}

void* challenge_expiration_thread(void* arg) {
    log_info("Challenge expiration thread started");
    
    while (1) {
        sleep(5);  // Check every 5 seconds
        // Check for expired challenges
        challenge_check_expired();
    }
    
    return NULL;
}

// Cleanup khi disconnect
static void client_cleanup(int client_sock) {
    pthread_mutex_lock(&g_clients_mutex);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (g_clients[i].socket == client_sock) {
            if (g_clients[i].authenticated) {
                log_info("[CLEANUP] Client socket=%d, user_id=%s", 
                         client_sock, g_clients[i].user_id);
                
                // Xóa khỏi matchmaking queue
                matcher_remove_from_queue(g_clients[i].user_id);
                g_clients[i].socket = -1;
                char user_id[64];
                strncpy(user_id, g_clients[i].user_id, 63);
                user_id[63] = '\0';
                if (user_id[0] != '\0') {
                user_update_status(user_id, "offline");
                log_info("✅ User %s marked offline in database", user_id);
            }
            } else {
                // User chưa authenticated → xóa luôn
                g_clients[i].socket = 0;
                g_clients[i].user_id[0] = '\0';
                g_clients[i].authenticated = false;
            }
            break;
        }
    }
    
    pthread_mutex_unlock(&g_clients_mutex);
}

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
            log_debug("%d", n);
            log_error("ws_recv_message error for client %d", client_sock);
            break;
        }

        log_info("Received WebSocket message type=%d from client %d", msg.type, client_sock);

        // Handle message
        handle_message(client_sock, &msg);
        
        log_info("Message handled successfully for client %d, waiting for next message...", client_sock);
    }

    log_info("[DISCONNECT] Client %d disconnecting, cleaning up...", client_sock);
    client_cleanup(client_sock);
    
    close(client_sock);
    log_info("Client %d thread terminated", client_sock);
    
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
    memset(g_clients, 0, sizeof(g_clients));
    challenge_manager_init();
    pthread_t expiration_tid;
    int result = pthread_create(&expiration_tid, NULL, challenge_expiration_thread, NULL);
    if (result == 0) {
        pthread_detach(expiration_tid);
        log_info("Challenge expiration thread created");
    } else {
        log_error("Failed to create expiration thread: %d", result);
    }
    game_init_timeout_monitor();
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

    challenge_manager_cleanup();
    close(server_sock);
    log_info("WebSocket server shutdown");
}