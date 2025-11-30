#ifndef WS_HANDLER_H
#define WS_HANDLER_H

#include "ws_protocol.h"

typedef struct {
    char user_id[64];
} auth_user_t;
// Xử lý message chung
void handle_message(int client_sock, message_t *msg);

// Các hàm xử lý chi tiết
void handle_register(int client_sock, auth_payload *auth);
void handle_login(int client_sock, auth_payload *auth);
void handle_join_queue(int client_sock, const char *token);
void handle_leave_queue(int client_sock, const char *token);
void handle_player_move(int client_sock, move_payload *move);
void handle_chat(int client_sock, chat_payload *chat);
void handle_place_ship(int client_sock, place_ship_payload *payload, const char *token);
void handle_logout(int client_sock, message_t *msg);
int check_token(int client_sock, const char *token, auth_user_t *out_user);
void handle_player_ready(int client_sock, message_t *msg);


#endif
