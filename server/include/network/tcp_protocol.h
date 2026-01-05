#ifndef TCP_PROTOCOL_H
#define TCP_PROTOCOL_H

#define MAX_JWT_LEN 512

#include <stdint.h>
#include <sys/socket.h>
#include "game/game_board.h"

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
    MSG_CHALLENGE_PLAYER = 19,
    MSG_CHALLENGE_RECEIVED = 20,
    MSG_CHALLENGE_ACCEPT = 21,
    MSG_CHALLENGE_DECLINE = 22,
    MSG_CHALLENGE_DECLINED = 23,
    MSG_CHALLENGE_EXPIRED = 24,
    MSG_CHALLENGE_CANCEL = 25,
    MSG_CHALLENGE_CANCELLED = 26,
    MSG_AUTH_TOKEN = 27,
    MSG_TURN_WARNING = 28,
    MSG_GAME_TIMEOUT = 29,
    MSG_CHAT_MESSAGE = 30,
    MSG_GAME_RESULT = 31,
    MSG_GAME_LOGS = 32,
    MSG_RESIGN = 33,
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
    int ship_type;
    int row;
    int col;
    uint8_t is_horizontal;
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
    uint8_t _padding[1];
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
    char reason[64];
} game_timeout_payload;

typedef struct __attribute__((packed)) {
    char player_username[32];
    int row;
    int col;
    bool is_hit;
    bool is_sunk;
    int sunk_ship_type;
    int turn_number;
    uint32_t timestamp;
} game_log_entry_t;

typedef struct {
    ship_type_t type;
    int start_row;
    int start_col;
    bool is_horizontal;
} ship_log_t;

typedef struct __attribute__((packed)) {
    char game_id[65];

    char player1_id[64];
    char player1_username[32];

    char player2_id[64];
    char player2_username[32];

    int chunk_index;
    int total_chunks;
    int log_count;

    game_log_entry_t logs[50];

    int player1_ship_count;
    ship_log_t player1_ships[MAX_SHIPS];

    int player2_ship_count;
    ship_log_t player2_ships[MAX_SHIPS];
} game_logs_payload;


typedef struct __attribute__((packed)) {
    char game_id[65];
} resign_payload;


typedef struct __attribute__((packed)) {
    char game_id[65];
    char winner_id[64];
    char winner_username[32];
    char loser_username[32];
    int total_turns;
    uint32_t game_duration;      // Seconds
    int winner_old_elo;
    int winner_new_elo;
    int loser_old_elo;
    int loser_new_elo;
    int winner_hits;
    int winner_misses;
    int loser_hits;
    int loser_misses;
} game_result_payload;
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
        game_result_payload game_result; 
        game_logs_payload game_logs;
        resign_payload resign;
    } payload;
} message_t;

// TCP functions
ssize_t tcp_send_message(int sock, message_t *msg);
ssize_t tcp_recv_message(int sock, message_t *msg);
void tcp_close(int sock);

#endif