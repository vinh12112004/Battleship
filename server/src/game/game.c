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

    bson_t *doc = bson_new();
    bson_oid_t oid;
    bson_oid_init(&oid, NULL);

    BSON_APPEND_OID(doc, "_id", &oid);
    BSON_APPEND_UTF8(doc, "player1_id", player1_id);
    BSON_APPEND_UTF8(doc, "player2_id", player2_id);
    BSON_APPEND_UTF8(doc, "phase", "placing_ships");
    BSON_APPEND_UTF8(doc, "current_turn", player1_id);

    // --- TẠO BOARD 10×10 TOÀN 0 ---
    bson_t board1, board2, grid1, grid2;

    BSON_APPEND_DOCUMENT_BEGIN(doc, "player1_board", &board1);
    BSON_APPEND_ARRAY_BEGIN(&board1, "grid", &grid1);

    // tạo mảng 10 phần tử
    for (int y = 0; y < 10; y++) {
        bson_t row;
        char key[16];
        snprintf(key, sizeof(key), "%d", y);
        BSON_APPEND_ARRAY_BEGIN(&grid1, key, &row);

        for (int x = 0; x < 10; x++) {
            char subkey[16];
            snprintf(subkey, sizeof(subkey), "%d", x);
            bson_append_int32(&row, subkey, strlen(subkey), 0);
        }
        bson_append_array_end(&grid1, &row);
    }
    bson_append_array_end(&board1, &grid1);
    bson_append_document_end(doc, &board1);


    // BOARD PLAYER 2
    BSON_APPEND_DOCUMENT_BEGIN(doc, "player2_board", &board2);
    BSON_APPEND_ARRAY_BEGIN(&board2, "grid", &grid2);

    for (int y = 0; y < 10; y++) {
        bson_t row;
        char key[16];
        snprintf(key, sizeof(key), "%d", y);
        BSON_APPEND_ARRAY_BEGIN(&grid2, key, &row);

        for (int x = 0; x < 10; x++) {
            char subkey[16];
            snprintf(subkey, sizeof(subkey), "%d", x);
            bson_append_int32(&row, subkey, strlen(subkey), 0);
        }
        bson_append_array_end(&grid2, &row);
    }
    bson_append_array_end(&board2, &grid2);
    bson_append_document_end(doc, &board2);

    // created_at
    int64_t now_ms = (int64_t)time(NULL) * 1000;
    bson_append_date_time(doc, "created_at", strlen("created_at"), now_ms);

    bson_error_t error;
    bool success = mongoc_collection_insert_one(collection, doc, NULL, NULL, &error);

    if (success) {
        char oid_str[25];
        bson_oid_to_string(&oid, oid_str);
        snprintf(out_game_id, 25, "%s", oid_str);
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