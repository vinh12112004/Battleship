#include "network/ws_handler.h"
#include "auth/auth.h"
#include "game/game.h"
#include "matchmaking/matcher.h"
#include "utils/logger.h"
#include <stdio.h>
#include <string.h>

void handle_message(int client_sock, message_t *msg) {
    log_debug("Handling message type=%d for client %d", msg->type, client_sock);
    
    switch(msg->type) {
        case MSG_REGISTER:
            handle_register(client_sock, &msg->payload.auth);
            break;
        case MSG_LOGIN:
            handle_login(client_sock, &msg->payload.auth);
            break;
        case MSG_JOIN_QUEUE:
            handle_join_queue(client_sock);
            break;
        case MSG_PLAYER_MOVE:
            handle_player_move(client_sock, &msg->payload.move);
            break;
        case MSG_CHAT:
            handle_chat(client_sock, &msg->payload.chat);
            break;
        case MSG_LOGOUT:
            handle_logout(client_sock, &msg->payload.logout);
            break;   
        default:
            log_warn("Unknown message type: %d from client %d", msg->type, client_sock);
            break;
    }
    
    log_debug("Message handler finished for type=%d, client=%d", msg->type, client_sock);
}

void handle_register(int client_sock, auth_payload *auth) {
    if (!auth || !auth->username[0] || !auth->password[0]) {
        log_warn("Invalid registration payload from client %d", client_sock);
        return;
    }

    log_info("Processing registration for username: %s", auth->username);

    // Gọi auth module để đăng ký
    auth_result_t *res = auth_register(auth->username, auth->username, auth->password);
    if (!res) {
        log_error("auth_register returned NULL for client %d", client_sock);
        return;
    }

    // Tạo response
    message_t resp;

    if (res->success) {
        resp.type = MSG_AUTH_SUCCESS;
        strncpy(resp.payload.auth_suc.token, res->token, MAX_JWT_LEN - 1);
        resp.payload.auth_suc.token[MAX_JWT_LEN - 1] = '\0';
        strncpy(resp.payload.auth_suc.username, res->user->username, 31);
        resp.payload.auth_suc.username[31] = '\0';
        log_info("Registration successful for %s", auth->username);
    } else {
        resp.type = MSG_AUTH_FAILED;
        strncpy(resp.payload.auth_fail.reason, res->error_message, 63);
        resp.payload.auth_fail.reason[63] = '\0';
        log_warn("Registration failed for client %d: %s", client_sock, res->error_message);
    }

    // Gửi message về client
    ssize_t sent = send_message((SOCKET)client_sock, &resp);
    if (sent <= 0) {
        log_error("Failed to send registration response to client %d", client_sock);
    }

    log_debug("About to free auth_result for client %d", client_sock);
    auth_result_free(res);
    log_debug("auth_result freed successfully for client %d", client_sock);
}

void handle_login(int client_sock, auth_payload *auth) {
    log_info("Processing login for username: %s", auth->username);
    
    auth_result_t *res = auth_login(auth->username, auth->password);
    message_t resp;

    if (res->success) {
        resp.type = MSG_AUTH_SUCCESS;
        strncpy(resp.payload.auth_suc.token, res->token, MAX_JWT_LEN - 1);
        resp.payload.auth_suc.token[MAX_JWT_LEN - 1] = '\0';
        strncpy(resp.payload.auth_suc.username, res->user->username, 31);
        resp.payload.auth_suc.username[31] = '\0';
        
        log_info("Login successful for %s, sending response...", auth->username);
    } else {
        resp.type = MSG_AUTH_FAILED;
        strncpy(resp.payload.auth_fail.reason, res->error_message, 63);
        resp.payload.auth_fail.reason[63] = '\0';
        
        log_warn("Login failed for %s: %s", auth->username, res->error_message);
    }

    ssize_t sent = send_message(client_sock, &resp);
    if (sent <= 0) {
        log_error("Failed to send login response to client %d: %d", client_sock, sent);
    } else {
        log_info("Login response sent successfully to client %d (%zd bytes)", client_sock, sent);
    }
    
    log_debug("About to free auth_result for client %d", client_sock);
    auth_result_free(res);
    log_debug("auth_result freed successfully for client %d", client_sock);
}

void handle_logout(int client_sock, logout_payload *logout) {
    log_info("Processing logout for client %d", client_sock);
    
    message_t resp;
    bool success = auth_logout(logout->token);

    if (success) {
        resp.type = MSG_AUTH_SUCCESS;
        strncpy(resp.payload.auth_suc.username, "logout", 31);
        resp.payload.auth_suc.username[31] = '\0';
        resp.payload.auth_suc.token[0] = '\0';
        log_info("User logged out successfully");
    } else {
        resp.type = MSG_AUTH_FAILED;
        strncpy(resp.payload.auth_fail.reason, "Logout failed", 63);
        resp.payload.auth_fail.reason[63] = '\0';
        log_warn("Logout failed for client %d", client_sock);
    }

    ssize_t sent = send_message(client_sock, &resp);
    if (sent <= 0) {
        log_error("Failed to send logout response to client %d", client_sock);
    } else {
        log_info("Logout response sent to client %d", client_sock);
    }
}

void handle_join_queue(int client_sock) {
    log_info("Client %d joining matchmaking queue", client_sock);
    // Gọi matchmaking
}

void handle_player_move(int client_sock, move_payload *move) {
    log_info("Player move from client %d: (%d, %d)", client_sock, move->x, move->y);
    // Gọi game module, gửi move_result
}

void handle_chat(int client_sock, chat_payload *chat) {
    log_info("Chat message from client %d: %s", client_sock, chat->message);
    // Gửi lại message cho opponent
}