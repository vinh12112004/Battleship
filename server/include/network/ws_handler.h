#ifndef WS_HANDLER_H
#define WS_HANDLER_H

#include "ws_protocol.h"

// Xử lý message chung
void handle_message(int client_sock, message_t *msg);

// Các hàm xử lý chi tiết
void handle_register(int client_sock, auth_payload *auth);
void handle_login(int client_sock, auth_payload *auth);
void handle_join_queue(int client_sock);
void handle_player_move(int client_sock, move_payload *move);
void handle_chat(int client_sock, chat_payload *chat);
void handle_logout(int client_sock, logout_payload *logout);

#endif
