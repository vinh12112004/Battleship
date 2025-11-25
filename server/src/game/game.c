#include "game/game.h"
#include "database/mongo.h"
#include "utils/logger.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define COLLECTION_GAMES "games"

bool game_create(const char *player1_id, const char *player2_id, char *out_game_id) {
    mongoc_client_t *client = mongo_get_client(g_mongo_ctx);
    if (!client) return false;
    
    mongoc_collection_t *collection = mongo_get_collection(client, COLLECTION_GAMES);
    if (!collection) {
        mongo_release_client(g_mongo_ctx, client);
        return false;
    }
    
    // Tạo game document
    bson_t *doc = bson_new();
    bson_oid_t oid;
    bson_oid_init(&oid, NULL);
    
    BSON_APPEND_OID(doc, "_id", &oid);
    BSON_APPEND_UTF8(doc, "player1_id", player1_id);
    BSON_APPEND_UTF8(doc, "player2_id", player2_id);
    BSON_APPEND_UTF8(doc, "state", "placing_ships");
    BSON_APPEND_UTF8(doc, "current_turn", player1_id);
    BSON_APPEND_NOW_UTC(doc, "created_at");
    
    bson_error_t error;
    bool success = mongoc_collection_insert_one(collection, doc, NULL, NULL, &error);
    
    if (success) {
        // Return game_id
        char oid_str[25];
        bson_oid_to_string(&oid, oid_str);
        strcpy(out_game_id, oid_str);
        log_info("Game created: %s", oid_str);
    } else {
        log_error("Failed to create game: %s", error.message);
    }
    
    bson_destroy(doc);
    mongoc_collection_destroy(collection);
    mongo_release_client(g_mongo_ctx, client);
    
    return success;
}

game_session_t* game_get(const char *game_id) {
    // TODO: Implement get game from MongoDB
    return NULL;
}

bool game_update_state(const char *game_id, game_state_t new_state) {
    // TODO: Implement update game state
    return false;
}

bool game_end(const char *game_id, const char *winner_id) {
    // TODO: Implement end game
    return false;
}

void game_free(game_session_t *game) {
    if (game) free(game);
}