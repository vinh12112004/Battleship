#ifndef GAME_H
#define GAME_H

#include <stdbool.h>

typedef enum {
    GAME_STATE_WAITING,
    GAME_STATE_PLACING_SHIPS,
    GAME_STATE_PLAYING,
    GAME_STATE_FINISHED
} game_state_t;

typedef struct {
    char game_id[65];
    char player1_id[64];
    char player2_id[64];
    game_state_t state;
    char current_turn[64];
    long long created_at;
} game_session_t;

// Game operations
bool game_create(const char *player1_id, const char *player2_id, char *out_game_id);
game_session_t* game_get(const char *game_id);
bool game_update_state(const char *game_id, game_state_t new_state);
bool game_end(const char *game_id, const char *winner_id);
void game_free(game_session_t *game);

#endif // GAME_H