#ifndef WS_PROTOCOL_H
#define WS_PROTOCOL_H

#define MAX_JWT_LEN 512

#include <stdint.h>
#include <winsock2.h>  // SOCKET typedef

// 1. Enum message type
typedef enum {
    MSG_REGISTER = 1,
    MSG_LOGIN,
    MSG_AUTH_SUCCESS,
    MSG_AUTH_FAILED,
    MSG_JOIN_QUEUE,
    MSG_LEAVE_QUEUE,
    MSG_START_GAME,
    MSG_PLAYER_MOVE,
    MSG_MOVE_RESULT,
    MSG_GAME_OVER,
    MSG_CHAT,
    MSG_LOGOUT
} msg_type;

// 2. Payload structs
typedef struct { char username[32]; char password[32]; } auth_payload;
typedef struct { char token[MAX_JWT_LEN]; char username[32]; } auth_success_payload;
typedef struct { char reason[64]; } auth_failed_payload;
typedef struct { int x; int y; } move_payload;
typedef struct { int x; int y; int hit; char sunk[32]; } move_result_payload;
typedef struct { char opponent[32]; } start_game_payload;
typedef struct { char message[128]; } chat_payload;
typedef struct { char token[MAX_JWT_LEN]; } logout_payload;


// 3. Wrapper message
typedef struct {
    msg_type type;
    union {
        auth_payload auth;
        auth_success_payload auth_suc;
        auth_failed_payload auth_fail;
        move_payload move;
        move_result_payload move_res;
        start_game_payload start_game;
        chat_payload chat;
        logout_payload logout;
    } payload;
} message_t;

// 4. Serialize / deserialize
ssize_t send_message(SOCKET sock, message_t *msg);
ssize_t recv_message(SOCKET sock, message_t *msg);

#endif
