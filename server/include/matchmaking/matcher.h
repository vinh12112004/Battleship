#ifndef MATCHER_H
#define MATCHER_H

#include <stdbool.h>

typedef struct {
    char user_id[64];
    int socket;
    int elo_rating;
    char game_type[32]; // "ranked", "casual", etc.
    long long join_time; // timestamp
} queue_player_t;

// Matchmaking functions
void matcher_init();
void matcher_cleanup();

// Queue operations
bool matcher_add_to_queue(int client_sock, const char *user_id, int elo_rating, const char *game_type);
bool matcher_remove_from_queue(const char *user_id);
void matcher_find_match(); // Tìm cặp đấu

// Helper
queue_player_t* matcher_get_player(const char *user_id);
int matcher_get_queue_size();

#endif // MATCHER_H