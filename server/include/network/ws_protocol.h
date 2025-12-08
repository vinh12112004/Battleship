#ifndef WS_PROTOCOL_H
#define WS_PROTOCOL_H

#define MAX_JWT_LEN 512

#include <stdint.h>
#include <sys/socket.h>
#include "game/game_board.h"

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
    MSG_PLACE_SHIP = 15,
    MSG_PLAYER_READY,
    MSG_GET_ONLINE_PLAYERS = 17,
    MSG_ONLINE_PLAYERS_LIST = 18,
    MSG_CHALLENGE_PLAYER = 19,        // A → Server: Challenge B
    MSG_CHALLENGE_RECEIVED = 20,      // Server → B: You got challenged
    MSG_CHALLENGE_ACCEPT = 21,        // B → Server: Accept challenge
    MSG_CHALLENGE_DECLINE = 22,       // B → Server: Decline challenge
    MSG_CHALLENGE_DECLINED = 23,      // Server → A: B declined
    MSG_CHALLENGE_EXPIRED = 24,       // Server → A/B: Challenge expired
    MSG_CHALLENGE_CANCEL = 25,        // A → Server: Cancel challenge
    MSG_CHALLENGE_CANCELLED = 26,      // Server → B: A cancelled
    MSG_AUTH_TOKEN = 27,
    MSG_TURN_WARNING = 28,
    MSG_GAME_TIMEOUT = 29,
    MSG_CHAT_MESSAGE = 30,
} msg_type;

typedef struct __attribute__((packed)) {
    char challenger_id[64];
    char target_id[64];
    char challenge_id[65];
    char game_mode[32];
    int time_control;
} challenge_payload;

typedef struct __attribute__((packed)) {
    char challenger_username[64];
    char challenger_id[64];
    char challenge_id[65];
    char game_mode[32];
    int time_control;
    int64_t expires_at;
} challenge_received_payload;

typedef struct {
    char challenge_id[65];
} challenge_response_payload;

typedef struct {
    int ship_type;      // 5=Carrier, 4=Battleship, 3=Cruiser/Sub, 2=Destroyer
    int row;
    int col;
    uint8_t is_horizontal;  // 1=horizontal, 0=vertical
    uint8_t _padding[3];
} place_ship_payload;

// Payload structs
typedef struct { char username[32]; char password[32]; } auth_payload;
typedef struct { char token[MAX_JWT_LEN]; char username[32]; } auth_success_payload;
typedef struct { char reason[64]; } auth_failed_payload;
typedef struct __attribute__((packed)) {
    char game_id[65];
    int row;
    int col;
} move_payload;
typedef struct __attribute__((packed)) {
    int row;
    int col;
    uint8_t is_hit;
    uint8_t is_sunk;
    int sunk_ship_type;
    uint8_t game_over;
    uint8_t is_your_shot;
    uint8_t _padding[1];  // Align to 16 bytes
} move_result_payload;
typedef struct { char opponent[32]; char game_id[64]; char current_turn[32];} start_game_payload;
typedef struct { char game_id[64]; char message[128]; } chat_payload;
typedef struct { char username[64]; char text[128]; } chat_message_payload;

typedef struct __attribute__((packed)) {
    char game_id[65];
    uint8_t board_state[BOARD_SIZE];
} ready_payload;

typedef struct {
    int count;
    char players[50][64];
    int elo_ratings[50];
    char ranks[50][32];
} online_players_payload;

typedef struct {
    int seconds_remaining;
} turn_warning_payload;

typedef struct {
    char winner_id[64];
    char loser_id[64];
    char reason[64];  // "timeout", "disconnect", etc.
} game_timeout_payload;

// Message structure
typedef struct __attribute__((packed)) {
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
        chat_message_payload chat_msg;
        place_ship_payload place_ship;
        ready_payload ready;
        online_players_payload online_players;
        challenge_payload challenge;
        challenge_received_payload challenge_recv;
        challenge_response_payload challenge_resp;
        turn_warning_payload turn_warning;
        game_timeout_payload game_timeout;
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