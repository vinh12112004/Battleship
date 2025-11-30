#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include "game_board.h"
typedef enum {
    GAME_STATE_PLACING_SHIPS,
    GAME_STATE_PLAYING,
    GAME_STATE_FINISHED
} game_state_t;

typedef struct {
    char game_id[65];
    char player1_id[64];
    char player2_id[64];
    int player1_socket;
    int player2_socket;
    
    board_t player1_board;
    board_t player2_board;
    
    game_state_t state;
    char current_turn[64];  // user_id of current player
    long long created_at;
    
    bool player1_ready;  // Ships placed?
    bool player2_ready;
} game_session_t;

bool game_is_player_turn(const char *game_id, const char *player_id);
void game_switch_turn(game_session_t *game);
game_session_t* game_find_by_player(const char *player_id);

// Game operations
bool game_create(const char *player1_id, const char *player2_id, char *out_game_id);
game_session_t* game_get(const char *game_id);
bool game_place_ship(const char *game_id, const char *player_id, 
                     ship_type_t type, int row, int col, bool is_horizontal);
shot_result_t game_process_shot(const char *game_id, const char *player_id, int row, int col);
bool game_update_state(const char *game_id, game_state_t new_state);
bool game_end(const char *game_id, const char *winner_id);
void game_free(game_session_t *game);

#endif // GAME_H