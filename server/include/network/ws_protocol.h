#ifndef WS_PROTOCOL_H
#define WS_PROTOCOL_H

#define MAX_JWT_LEN 512

#include <stdint.h>
#include <sys/socket.h>

// WebSocket opcode
typedef enum {
    WS_OPCODE_CONTINUATION = 0x0,
    WS_OPCODE_TEXT = 0x1,
    WS_OPCODE_BINARY = 0x2,
    WS_OPCODE_CLOSE = 0x8,
    WS_OPCODE_PING = 0x9,
    WS_OPCODE_PONG = 0xA
} ws_opcode_t;

// WebSocket frame header
typedef struct {
    uint8_t fin;
    uint8_t opcode;
    uint8_t mask;
    uint64_t payload_len;
    uint8_t masking_key[4];
} ws_frame_t;

// Message types
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
    MSG_LOGOUT,
    MSG_PING = 13,    
    MSG_PONG = 14,
    MSG_PLACE_SHIP = 15
} msg_type;

typedef struct {
    int ship_type;      // 5=Carrier, 4=Battleship, 3=Cruiser/Sub, 2=Destroyer
    int row;
    int col;
    uint8_t is_horizontal;  // 1=horizontal, 0=vertical
} place_ship_payload;

// Payload structs
typedef struct { char username[32]; char password[32]; } auth_payload;
typedef struct { char token[MAX_JWT_LEN]; char username[32]; } auth_success_payload;
typedef struct { char reason[64]; } auth_failed_payload;
typedef struct { int x; int y; } move_payload;
typedef struct { int x; int y; int hit; char sunk[32]; } move_result_payload;
typedef struct { char opponent[32]; } start_game_payload;
typedef struct { char message[128]; } chat_payload;

// Message structure
typedef struct {
    msg_type type;
    char token[MAX_JWT_LEN];
    union {
        auth_payload auth;
        auth_success_payload auth_suc;
        auth_failed_payload auth_fail;
        move_payload move;
        move_result_payload move_res;
        start_game_payload start_game;
        chat_payload chat;
        place_ship_payload place_ship;
    } payload;
} message_t;

// WebSocket functions
int ws_handshake(int sock);
ssize_t ws_send_message(int sock, message_t *msg);
ssize_t ws_recv_message(int sock, message_t *msg);
int ws_send_frame(int sock, uint8_t opcode, const char *payload, size_t len);
int ws_recv_frame(int sock, ws_frame_t *frame, char **payload);
void ws_close(int sock, uint16_t code);

#endif