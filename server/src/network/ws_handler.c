#include "network/ws_handler.h"
#include "auth/auth.h"         // register/login logic
#include "game/game.h"         // move xử lý
#include "matchmaking/matcher.h"
#include <stdio.h>

void handle_message(int client_sock, message_t *msg) {
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
        default:
            printf("Unknown message type: %d\n", msg->type);
            break;
    }
}

// Các hàm chi tiết gọi vào module Auth/Game
void handle_register(int client_sock, auth_payload *auth) {
    // Gọi auth module, trả auth_success / auth_failed
}

void handle_login(int client_sock, auth_payload *auth) {
    auth_result_t *res = auth_login(auth->username, auth->password);
    message_t resp;

    if (res->success) {
        resp.type = MSG_AUTH_SUCCESS;
        strncpy(resp.payload.auth_suc.token, res->token, MAX_JWT_LEN - 1);
        resp.payload.auth_suc.token[MAX_JWT_LEN - 1] = '\0'; // đảm bảo kết thúc chuỗi
        strncpy(resp.payload.auth_suc.username, res->user->username, 31);
        resp.payload.auth_suc.username[31] = '\0';
    } else {
        resp.type = MSG_AUTH_FAILED;
        strncpy(resp.payload.auth_fail.reason, res->error_message, 63);
        resp.payload.auth_fail.reason[63] = '\0';
    }

    send_message(client_sock, &resp);
    auth_result_free(res);
}

void handle_join_queue(int client_sock) {
    // Gọi matchmaking
}

void handle_player_move(int client_sock, move_payload *move) {
    // Gọi game module, gửi move_result
}

void handle_chat(int client_sock, chat_payload *chat) {
    // Gửi lại message cho opponent
}
