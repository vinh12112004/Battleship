#include "network/ws_handler.h"
#include "auth/auth.h"
#include "game/game.h"
#include "matchmaking/matcher.h"
#include "utils/logger.h"
#include <stdio.h>
#include <string.h>
#include "auth/jwt.h"


void handle_message(int client_sock, message_t *msg) {
    log_debug("Handling WebSocket message type=%d for client %d", msg->type, client_sock);

    auth_user_t user; 
    int requires_auth = 0;

    // Các message cần xác thực
    switch(msg->type) {
        case MSG_JOIN_QUEUE:
        case MSG_PLAYER_MOVE:
        case MSG_CHAT:
        case MSG_LOGOUT:
            requires_auth = 1;
            break;
        default:
            break;
    }

    if (requires_auth) {
        if (!check_token(client_sock, msg->token, &user)) {
            // Token không hợp lệ => dừng xử lý
            return;
        }
    }

    // Gọi handler tương ứng
    switch(msg->type) {
        case MSG_REGISTER:
            handle_register(client_sock, &msg->payload.auth);
            break;
        case MSG_LOGIN:
            handle_login(client_sock, &msg->payload.auth);
            break;
        case MSG_JOIN_QUEUE:
            // handle_join_queue(client_sock, user.user_id);
            break;
        case MSG_PLAYER_MOVE:
            handle_player_move(client_sock, &msg->payload.move);
            break;
        case MSG_CHAT:
            handle_chat(client_sock, &msg->payload.chat);
            break;
        case MSG_LOGOUT:
            handle_logout(client_sock, msg);
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

    // Call auth module to register
    auth_result_t *res = auth_register(auth->username, auth->username, auth->password);
    if (!res) {
        log_error("auth_register returned NULL for client %d", client_sock);
        return;
    }

    // Create response
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

    // Send message via WebSocket
    ssize_t sent = ws_send_message((SOCKET)client_sock, &resp);
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

    ssize_t sent = ws_send_message(client_sock, &resp);
    if (sent <= 0) {
        log_error("Failed to send login response to client %d: %d", client_sock, sent);
    } else {
        log_info("Login response sent successfully to client %d (%zd bytes)", client_sock, sent);
    }
    
    log_debug("About to free auth_result for client %d", client_sock);
    auth_result_free(res);
    log_debug("auth_result freed successfully for client %d", client_sock);
}

void handle_logout(int client_sock, message_t *msg) {
    auth_user_t user;
    if (!check_token(client_sock, msg->token, &user)) return;

    log_info("User %s logging out", user.user_id);
    auth_logout(msg->token);

    message_t resp = {0};
    resp.type = MSG_AUTH_SUCCESS;
    ws_send_message(client_sock, &resp);
}


// void handle_join_queue(int client_sock, const char *token) {
//     auth_user_t user;
//     if (!check_token(client_sock, token, &user)) {
//         return; // dừng nếu token invalid
//     }

//     log_info("Client %d (%s) joining queue", client_sock, user.user_id);
//     add_to_queue(client_sock, user.user_id);
// }

void handle_player_move(int client_sock, move_payload *move) {
    log_info("Player move from client %d: (%d, %d)", client_sock, move->x, move->y);
    // Call game module, send move_result
}

void handle_chat(int client_sock, chat_payload *chat) {
    log_info("Chat message from client %d: %s", client_sock, chat->message);
    // Send message to opponent
}

int check_token(int client_sock, const char *token, auth_user_t *out_user) {
    char *user_id = jwt_verify(token);
    if (!user_id) {
        log_warn("Client %d gửi token không hợp lệ", client_sock);

        message_t resp;
        resp.type = MSG_AUTH_FAILED;
        strncpy(resp.payload.auth_fail.reason, "Invalid or expired token", 63);
        resp.payload.auth_fail.reason[63] = '\0';
        ws_send_message(client_sock, &resp);

        return 0; // không hợp lệ
    }

    strncpy(out_user->user_id, user_id, 63);
    out_user->user_id[63] = '\0';
    free(user_id);

    return 1; // hợp lệ
}
