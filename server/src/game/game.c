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

    BSON_APPEND_BOOL(doc, "player1_ready", false);
    BSON_APPEND_BOOL(doc, "player2_ready", false);

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

// Thêm vào game.c

bool game_set_player_ready(const char *game_id, const char *player_id, const uint8_t board[BOARD_SIZE]) {
    mongoc_client_t *client = mongo_get_client(g_mongo_ctx);
    if (!client) return false;

    mongoc_collection_t *collection = mongo_get_collection(client, COLLECTION_GAMES);
    if (!collection) {
        mongo_release_client(g_mongo_ctx, client);
        return false;
    }

    // 1. Tìm game để xác định vai trò (P1 hay P2) và trạng thái đối thủ
    bson_t *query = bson_new();
    bson_oid_t oid;
    bson_oid_init_from_string(&oid, game_id);
    BSON_APPEND_OID(query, "_id", &oid);

    const bson_t *doc;
    mongoc_cursor_t *cursor = mongoc_collection_find_with_opts(collection, query, NULL, NULL);
    
    bool is_p1 = false;
    bool is_p2 = false;
    bool other_ready = false;
    bool success = false;

    if (mongoc_cursor_next(cursor, &doc)) {
        bson_iter_t iter;
        
        // Check Player 1
        if (bson_iter_init_find(&iter, doc, "player1_id") && BSON_ITER_HOLDS_UTF8(&iter)) {
            const char *p1_id = bson_iter_utf8(&iter, NULL);
            if (strcmp(p1_id, player_id) == 0) is_p1 = true;
        }

        // Check Player 2
        if (bson_iter_init_find(&iter, doc, "player2_id") && BSON_ITER_HOLDS_UTF8(&iter)) {
            const char *p2_id = bson_iter_utf8(&iter, NULL);
            if (strcmp(p2_id, player_id) == 0) is_p2 = true;
        }

        // Check trạng thái đối thủ
        if (is_p1) {
            if (bson_iter_init_find(&iter, doc, "player2_ready") && BSON_ITER_HOLDS_BOOL(&iter)) {
                other_ready = bson_iter_bool(&iter);
            }
        } else if (is_p2) {
            if (bson_iter_init_find(&iter, doc, "player1_ready") && BSON_ITER_HOLDS_BOOL(&iter)) {
                other_ready = bson_iter_bool(&iter);
            }
        }
    }
    mongoc_cursor_destroy(cursor); // Xong việc đọc, hủy cursor

    if (!is_p1 && !is_p2) {
        log_error("Player %s not found in game %s", player_id, game_id);
        bson_destroy(query);
        mongoc_collection_destroy(collection);
        mongo_release_client(g_mongo_ctx, client);
        return false;
    }

    // 2. Chuẩn bị dữ liệu Update
    bson_t *update = bson_new();
    bson_t set_doc;
    BSON_APPEND_DOCUMENT_BEGIN(update, "$set", &set_doc);

    // Set trạng thái ready
    const char *ready_field = is_p1 ? "player1_ready" : "player2_ready";
    BSON_APPEND_BOOL(&set_doc, ready_field, true);

    // Nếu đối thủ đã ready, chuyển game sang trạng thái playing
    if (other_ready) {
        BSON_APPEND_UTF8(&set_doc, "phase", "playing");
        log_info("Game %s started! Both players ready.", game_id);
    }

    // Xây dựng cấu trúc BSON cho Board (Convert 1D uint8_t -> 2D BSON Array)
    const char *board_field = is_p1 ? "player1_board" : "player2_board";
    bson_t board_doc, grid_arr;
    
    BSON_APPEND_DOCUMENT_BEGIN(&set_doc, board_field, &board_doc);
    BSON_APPEND_ARRAY_BEGIN(&board_doc, "grid", &grid_arr);

    for (int y = 0; y < 10; y++) {
        bson_t row;
        char key[16];
        snprintf(key, sizeof(key), "%d", y);
        BSON_APPEND_ARRAY_BEGIN(&grid_arr, key, &row);

        for (int x = 0; x < 10; x++) {
            char subkey[16];
            snprintf(subkey, sizeof(subkey), "%d", x);
            
            // Lấy giá trị từ mảng 1D: index = y * 10 + x
            int val = (int)board[y * 10 + x]; 
            bson_append_int32(&row, subkey, -1, val);
        }
        bson_append_array_end(&grid_arr, &row);
    }
    bson_append_array_end(&board_doc, &grid_arr);
    bson_append_document_end(&set_doc, &board_doc);
    
    bson_append_document_end(update, &set_doc);

    // 3. Thực thi Update
    bson_error_t error;
    if (mongoc_collection_update_one(collection, query, update, NULL, NULL, &error)) {
        success = true;
    } else {
        log_error("Failed to set player ready: %s", error.message);
    }

    bson_destroy(query);
    bson_destroy(update);
    mongoc_collection_destroy(collection);
    mongo_release_client(g_mongo_ctx, client);

    return success;
}