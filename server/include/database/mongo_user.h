#ifndef MONGO_USER_H
#define MONGO_USER_H

#include <mongoc/mongoc.h>
#include <stdbool.h>

typedef struct {
    char *id;  // ObjectId as string
    char *username;
    char *email;
    char *password_hash;
    char *display_name;
    char *avatar_url;
    char *status;
    int elo_rating;
    char *rank;
} user_t;

typedef struct {
    int count;
    char **usernames;
    int *elo_ratings;
    char **ranks;
} online_players_t;

// User operations
user_t* user_create(const char *username, const char *email, const char *password_hash);
user_t* user_find_by_username(const char *username);
user_t* user_find_by_email(const char *email);
user_t* user_find_by_id(const char *user_id);
bool user_update_status(const char *user_id, const char *status);
bool user_update_elo(const char *user_id, int new_elo);
void user_free(user_t *user);

online_players_t* user_get_online_players(void);
void online_players_free(online_players_t *players);

#endif // MONGO_USER_H