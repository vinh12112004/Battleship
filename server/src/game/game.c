#include "game/game.h"
#include "game/game_board.h"
#include "database/mongo.h"
#include "utils/logger.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "database/mongo_user.h"
#include "network/ws_protocol.h"
#include "network/ws_server.h"
#define COLLECTION_GAMES "games"
#define MAX_ACTIVE_GAMES 100

// In-memory cache for active games
static game_session_t *active_games[MAX_ACTIVE_GAMES];
static int active_game_count = 0;

// ==================== Helper: Serialize board to BSON ====================
static void board_to_bson(bson_t *parent, const char *key, const board_t *board) {
    bson_t board_doc, ships_array;
    
    BSON_APPEND_DOCUMENT_BEGIN(parent, key, &board_doc);
    
    // Serialize ships
    BSON_APPEND_ARRAY_BEGIN(&board_doc, "ships", &ships_array);
    for (int i = 0; i < board->ship_count; i++) {
        bson_t ship_doc;
        char index_str[16];
        snprintf(index_str, sizeof(index_str), "%d", i);
        
        BSON_APPEND_DOCUMENT_BEGIN(&ships_array, index_str, &ship_doc);
        BSON_APPEND_INT32(&ship_doc, "type", (int)board->ships[i].type);
        BSON_APPEND_INT32(&ship_doc, "start_row", board->ships[i].start_row);
        BSON_APPEND_INT32(&ship_doc, "start_col", board->ships[i].start_col);
        BSON_APPEND_BOOL(&ship_doc, "is_horizontal", board->ships[i].is_horizontal);
        BSON_APPEND_INT32(&ship_doc, "hits", board->ships[i].hits);
        BSON_APPEND_BOOL(&ship_doc, "is_sunk", board->ships[i].is_sunk);
        bson_append_document_end(&ships_array, &ship_doc);
    }
    bson_append_array_end(&board_doc, &ships_array);
    
    BSON_APPEND_INT32(&board_doc, "ships_remaining", board->ships_remaining);
    
    bson_t grid_array;
    BSON_APPEND_ARRAY_BEGIN(&board_doc, "grid", &grid_array);
    
    for (int y = 0; y < GRID_SIZE; y++) {
        bson_t row;
        char key_str[16];
        snprintf(key_str, sizeof(key_str), "%d", y);
        BSON_APPEND_ARRAY_BEGIN(&grid_array, key_str, &row);
        
        for (int x = 0; x < GRID_SIZE; x++) {
            char subkey[16];
            snprintf(subkey, sizeof(subkey), "%d", x);
            
            int val = (int)board->grid[y * GRID_SIZE + x];
            bson_append_int32(&row, subkey, -1, val);
        }
        bson_append_array_end(&grid_array, &row);
    }
    bson_append_array_end(&board_doc, &grid_array);
    
    bson_append_document_end(parent, &board_doc);
}

// ==================== Helper: Deserialize board from BSON ====================
static bool bson_to_board(const bson_t *doc, const char *key, board_t *board) {
    bson_iter_t iter, child, array_iter;
    
    if (!bson_iter_init_find(&iter, doc, key) || !bson_iter_recurse(&iter, &child)) {
        return false;
    }
    
    board_init(board);
    
    // Deserialize ships
    if (bson_iter_find(&child, "ships") && bson_iter_recurse(&child, &array_iter)) {
        int idx = 0;
        while (bson_iter_next(&array_iter) && idx < MAX_SHIPS) {
            bson_iter_t ship_iter;
            if (bson_iter_recurse(&array_iter, &ship_iter)) {
                ship_t *ship = &board->ships[idx];
                
                if (bson_iter_find(&ship_iter, "type"))
                    ship->type = (ship_type_t)bson_iter_int32(&ship_iter);
                if (bson_iter_find(&ship_iter, "start_row"))
                    ship->start_row = bson_iter_int32(&ship_iter);
                if (bson_iter_find(&ship_iter, "start_col"))
                    ship->start_col = bson_iter_int32(&ship_iter);
                if (bson_iter_find(&ship_iter, "is_horizontal"))
                    ship->is_horizontal = bson_iter_bool(&ship_iter);
                if (bson_iter_find(&ship_iter, "hits"))
                    ship->hits = bson_iter_int32(&ship_iter);
                if (bson_iter_find(&ship_iter, "is_sunk"))
                    ship->is_sunk = bson_iter_bool(&ship_iter);
                
                idx++;
            }
        }
        board->ship_count = idx;
    }
    
    // Deserialize ships_remaining
    bson_iter_init(&child, doc);
    if (bson_iter_find(&child, key) && bson_iter_recurse(&child, &child)) {
        if (bson_iter_find(&child, "ships_remaining"))
            board->ships_remaining = bson_iter_int32(&child);
    }
    
    bson_iter_init(&child, doc);
    if (bson_iter_find(&child, key) && bson_iter_recurse(&child, &child) &&
        bson_iter_find(&child, "grid") && bson_iter_recurse(&child, &array_iter)) {
        
        int row = 0;
        while (bson_iter_next(&array_iter) && row < GRID_SIZE) {
            bson_iter_t row_iter;
            if (bson_iter_recurse(&array_iter, &row_iter)) {
                int col = 0;
                while (bson_iter_next(&row_iter) && col < GRID_SIZE) {
                    board->grid[row * GRID_SIZE + col] = (cell_state_t)bson_iter_int32(&row_iter);
                    col++;
                }
            }
            row++;
        }
    }
    
    return true;
}

// ==================== Game Create ====================
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
    // Check in-memory cache first
    for (int i = 0; i < active_game_count; i++) {
        if (strcmp(active_games[i]->game_id, game_id) == 0) {
            return active_games[i];
        }
    }
    
    // Not in cache, load from MongoDB
    log_info("Game %s not in cache, loading from MongoDB...", game_id);
    
    if (strlen(game_id) != 24) {
        log_error("Invalid game_id format: %s", game_id);
        return NULL;
    }
    
    mongoc_client_t *client = mongo_get_client(g_mongo_ctx);
    if (!client) return NULL;
    
    mongoc_collection_t *collection = mongo_get_collection(client, COLLECTION_GAMES);
    if (!collection) {
        mongo_release_client(g_mongo_ctx, client);
        return NULL;
    }
    
    bson_t *query = bson_new();
    bson_oid_t oid;
    bson_oid_init_from_string(&oid, game_id);
    BSON_APPEND_OID(query, "_id", &oid);
    
    mongoc_cursor_t *cursor = mongoc_collection_find_with_opts(collection, query, NULL, NULL);
    
    game_session_t *game = NULL;
    const bson_t *doc;
    
    if (mongoc_cursor_next(cursor, &doc)) {
        game = (game_session_t*)calloc(1, sizeof(game_session_t));
        bson_iter_t iter;
        
        strncpy(game->game_id, game_id, 64);
        
        if (bson_iter_init_find(&iter, doc, "player1_id"))
            strncpy(game->player1_id, bson_iter_utf8(&iter, NULL), 63);
        
        if (bson_iter_init_find(&iter, doc, "player2_id"))
            strncpy(game->player2_id, bson_iter_utf8(&iter, NULL), 63);
        
        if (bson_iter_init_find(&iter, doc, "phase")) {
            const char *state_str = bson_iter_utf8(&iter, NULL);
            if (strcmp(state_str, "placing_ships") == 0)
                game->state = GAME_STATE_PLACING_SHIPS;
            else if (strcmp(state_str, "playing") == 0)
                game->state = GAME_STATE_PLAYING;
            else if (strcmp(state_str, "finished") == 0)
                game->state = GAME_STATE_FINISHED;
        }
        
        if (bson_iter_init_find(&iter, doc, "current_turn"))
            strncpy(game->current_turn, bson_iter_utf8(&iter, NULL), 63);
        
        if (bson_iter_init_find(&iter, doc, "player1_ready"))
            game->player1_ready = bson_iter_bool(&iter);
        
        if (bson_iter_init_find(&iter, doc, "player2_ready"))
            game->player2_ready = bson_iter_bool(&iter);
        
        // Load boards
        bson_to_board(doc, "player1_board", &game->player1_board);
        bson_to_board(doc, "player2_board", &game->player2_board);
        
        // Add to cache
        if (active_game_count < MAX_ACTIVE_GAMES) {
            active_games[active_game_count++] = game;
        }
        
        log_info("Game loaded from DB: %s", game_id);
    }
    
    mongoc_cursor_destroy(cursor);
    bson_destroy(query);
    mongoc_collection_destroy(collection);
    mongo_release_client(g_mongo_ctx, client);
    
    return game;
}

game_session_t* game_load_from_db(const char *game_id) {
    mongoc_client_t *client = mongo_get_client(g_mongo_ctx);
    if (!client) return NULL;

    mongoc_collection_t *collection = mongo_get_collection(client, COLLECTION_GAMES);
    if (!collection) {
        mongo_release_client(g_mongo_ctx, client);
        return NULL;
    }
    
    bson_t *query = bson_new();
    bson_oid_t oid;
    bson_oid_init_from_string(&oid, game_id);
    BSON_APPEND_OID(query, "_id", &oid);
    
    mongoc_cursor_t *cursor = mongoc_collection_find_with_opts(collection, query, NULL, NULL);
    const bson_t *doc;
    
    game_session_t *game = NULL;
    
    if (mongoc_cursor_next(cursor, &doc)) {
        game = (game_session_t*)malloc(sizeof(game_session_t));
        memset(game, 0, sizeof(game_session_t));
        
        bson_iter_t iter;
        
        // Parse game_id
        if (bson_iter_init_find(&iter, doc, "_id")) {
            bson_oid_to_string(bson_iter_oid(&iter), game->game_id);
        }
        
        if (bson_iter_init_find(&iter, doc, "player1_id"))
            strncpy(game->player1_id, bson_iter_utf8(&iter, NULL), 63);
        
        if (bson_iter_init_find(&iter, doc, "player2_id"))
            strncpy(game->player2_id, bson_iter_utf8(&iter, NULL), 63);
        
        if (bson_iter_init_find(&iter, doc, "phase")) {
            const char *state_str = bson_iter_utf8(&iter, NULL);
            if (strcmp(state_str, "placing_ships") == 0)
                game->state = GAME_STATE_PLACING_SHIPS;
            else if (strcmp(state_str, "playing") == 0)
                game->state = GAME_STATE_PLAYING;
            else if (strcmp(state_str, "finished") == 0)
                game->state = GAME_STATE_FINISHED;
        }
        
        if (bson_iter_init_find(&iter, doc, "current_turn"))
            strncpy(game->current_turn, bson_iter_utf8(&iter, NULL), 63);
        
        if (bson_iter_init_find(&iter, doc, "player1_ready"))
            game->player1_ready = bson_iter_bool(&iter);
        
        if (bson_iter_init_find(&iter, doc, "player2_ready"))
            game->player2_ready = bson_iter_bool(&iter);
        
        // Load boards
        bson_to_board(doc, "player1_board", &game->player1_board);
        bson_to_board(doc, "player2_board", &game->player2_board);
        
        // Add to cache
        if (active_game_count < MAX_ACTIVE_GAMES) {
            active_games[active_game_count++] = game;
        }
        
        log_info("Game loaded from DB: %s", game_id);
    }
    
    mongoc_cursor_destroy(cursor);
    bson_destroy(query);
    mongoc_collection_destroy(collection);
    mongo_release_client(g_mongo_ctx, client);
    
    return game;
}

// ==================== Game Update (sync to DB) ====================
static bool game_sync_to_db(game_session_t *game) {
    mongoc_client_t *client = mongo_get_client(g_mongo_ctx);
    if (!client) return false;
    
    mongoc_collection_t *collection = mongo_get_collection(client, COLLECTION_GAMES);
    if (!collection) {
        mongo_release_client(g_mongo_ctx, client);
        return false;
    }
    
    bson_t *query = bson_new();
    bson_oid_t oid;
    bson_oid_init_from_string(&oid, game->game_id);
    BSON_APPEND_OID(query, "_id", &oid);
    
    bson_t *update = bson_new();
    bson_t set_doc;
    BSON_APPEND_DOCUMENT_BEGIN(update, "$set", &set_doc);
    
    // State
    const char *state_str = "unknown";
    switch (game->state) {
        case GAME_STATE_PLACING_SHIPS: state_str = "placing_ships"; break;
        case GAME_STATE_PLAYING: state_str = "playing"; break;
        case GAME_STATE_FINISHED: state_str = "finished"; break;
        default: break;
    }
    BSON_APPEND_UTF8(&set_doc, "state", state_str);
    BSON_APPEND_UTF8(&set_doc, "current_turn", game->current_turn);
    BSON_APPEND_BOOL(&set_doc, "player1_ready", game->player1_ready);
    BSON_APPEND_BOOL(&set_doc, "player2_ready", game->player2_ready);
    
    // Boards
    board_to_bson(&set_doc, "player1_board", &game->player1_board);
    board_to_bson(&set_doc, "player2_board", &game->player2_board);
    
    bson_append_date_time(&set_doc, "updated_at", -1, (int64_t)time(NULL) * 1000);
    
    bson_append_document_end(update, &set_doc);
    
    bson_error_t error;
    bool success = mongoc_collection_update_one(collection, query, update, NULL, NULL, &error);
    
    if (!success) {
        log_error("Failed to sync game %s to DB: %s", game->game_id, error.message);
    }
    
    bson_destroy(query);
    bson_destroy(update);
    mongoc_collection_destroy(collection);
    mongo_release_client(g_mongo_ctx, client);
    
    return success;
}

// ==================== Game Operations (with DB sync) ====================

game_session_t* game_find_by_player(const char *player_id) {
    // Check in-memory first
    for (int i = 0; i < active_game_count; i++) {
        game_session_t *game = active_games[i];
        if (strcmp(game->player1_id, player_id) == 0 || 
            strcmp(game->player2_id, player_id) == 0) {
            return game;
        }
    }
    
    // Query MongoDB
    mongoc_client_t *client = mongo_get_client(g_mongo_ctx);
    if (!client) return NULL;
    
    mongoc_collection_t *collection = mongo_get_collection(client, COLLECTION_GAMES);
    if (!collection) {
        mongo_release_client(g_mongo_ctx, client);
        return NULL;
    }
    
    bson_t *query = bson_new();
    bson_t or_array;
    BSON_APPEND_ARRAY_BEGIN(query, "$or", &or_array);
    
    bson_t cond1, cond2;
    BSON_APPEND_DOCUMENT_BEGIN(&or_array, "0", &cond1);
    BSON_APPEND_UTF8(&cond1, "player1_id", player_id);
    bson_append_document_end(&or_array, &cond1);
    
    BSON_APPEND_DOCUMENT_BEGIN(&or_array, "1", &cond2);
    BSON_APPEND_UTF8(&cond2, "player2_id", player_id);
    bson_append_document_end(&or_array, &cond2);
    
    bson_append_array_end(query, &or_array);
    
    mongoc_cursor_t *cursor = mongoc_collection_find_with_opts(collection, query, NULL, NULL);
    const bson_t *doc;
    
    game_session_t *game = NULL;
    if (mongoc_cursor_next(cursor, &doc)) {
        bson_iter_t iter;
        if (bson_iter_init_find(&iter, doc, "_id")) {
            const bson_oid_t *oid = bson_iter_oid(&iter);
            char game_id[25];
            bson_oid_to_string(oid, game_id);
            game = game_get(game_id);
        }
    }
    
    mongoc_cursor_destroy(cursor);
    bson_destroy(query);
    mongoc_collection_destroy(collection);
    mongo_release_client(g_mongo_ctx, client);
    
    return game;
}

bool game_place_ship(const char *game_id, const char *player_id,
                     ship_type_t type, int row, int col, bool is_horizontal) {
    game_session_t *game = game_get(game_id);
    if (!game) {
        log_error("Game not found: %s", game_id);
        return false;
    }
    
    if (game->state != GAME_STATE_PLACING_SHIPS) {
        log_warn("Cannot place ships in current game state");
        return false;
    }
    
    board_t *board = NULL;
    bool *ready_flag = NULL;
    
    if (strcmp(game->player1_id, player_id) == 0) {
        board = &game->player1_board;
        ready_flag = &game->player1_ready;
    } else if (strcmp(game->player2_id, player_id) == 0) {
        board = &game->player2_board;
        ready_flag = &game->player2_ready;
    } else {
        log_error("Player not in game: %s", player_id);
        return false;
    }
    
    if (!board_place_ship(board, type, row, col, is_horizontal)) {
        return false;
    }
    
    if (board->ship_count == MAX_SHIPS) {
        *ready_flag = true;
        log_info("Player %s ready (all ships placed)", player_id);
        
        if (game->player1_ready && game->player2_ready) {
            game->state = GAME_STATE_PLAYING;
            log_info("Game %s started! Both players ready", game_id);
        }
    }
    
    // ✅ Sync to DB
    game_sync_to_db(game);
    
    return true;
}

bool game_is_player_turn(const char *game_id, const char *player_id) {
    game_session_t *game = game_get(game_id);
    if (!game) return false;
    return strcmp(game->current_turn, player_id) == 0;
}

void game_switch_turn(game_session_t *game) {
    if (strcmp(game->current_turn, game->player1_id) == 0) {
        strncpy(game->current_turn, game->player2_id, 63);
    } else {
        strncpy(game->current_turn, game->player1_id, 63);
    }
    game->turn_started_at = time(NULL);
    game->turn_timeout_warned = false;
    log_info("Turn switched to: %s", game->current_turn);
}

shot_result_t game_process_shot(const char *game_id, const char *player_id, int row, int col) {
    shot_result_t result = {false, false, 0, false};
    
    game_session_t *game = game_get(game_id);
    if (!game) {
        log_error("Game not found: %s", game_id);
        return result;
    }
    
    if (game->state != GAME_STATE_PLAYING) {
        log_warn("Game not in playing state");
        return result;
    }
    
    // ✅ Check turn
    if (!game_is_player_turn(game_id, player_id)) {
        log_warn("Not player's turn: %s", player_id);
        return result;
    }
    
    // ✅ Get opponent's board
    board_t *target_board = NULL;
    
    if (strcmp(game->player1_id, player_id) == 0) {
        target_board = &game->player2_board;
    } else if (strcmp(game->player2_id, player_id) == 0) {
        target_board = &game->player1_board;
    } else {
        log_error("Player not in game");
        return result;
    }
    
    // ✅ Process shot on target board
    result = board_process_shot(target_board, row, col);
    
    log_info("Shot result: hit=%d, sunk=%d, type=%d, game_over=%d",
             result.is_hit, result.is_sunk, result.sunk_ship_type, result.game_over);
    
    if (result.game_over) {
        game->state = GAME_STATE_FINISHED;
        log_info("🏆 Game over! Winner: %s", player_id);
        game_end(game_id, player_id);
    } else {
        game_switch_turn(game);
    }
    
    game_sync_to_db(game);
    
    return result;
}

bool game_end(const char *game_id, const char *winner_id) {
    game_session_t *game = game_get(game_id);
    if (!game) return false;
    
    game->state = GAME_STATE_FINISHED;
    
    // Update MongoDB with winner and final state
    mongoc_client_t *client = mongo_get_client(g_mongo_ctx);
    if (!client) return false;
    
    mongoc_collection_t *collection = mongo_get_collection(client, COLLECTION_GAMES);
    if (!collection) {
        mongo_release_client(g_mongo_ctx, client);
        return false;
    }
    
    bson_t *query = bson_new();
    bson_oid_t oid;
    bson_oid_init_from_string(&oid, game_id);
    BSON_APPEND_OID(query, "_id", &oid);
    
    bson_t *update = bson_new();
    bson_t set_doc;
    BSON_APPEND_DOCUMENT_BEGIN(update, "$set", &set_doc);
    BSON_APPEND_UTF8(&set_doc, "state", "finished");
    BSON_APPEND_UTF8(&set_doc, "winner_id", winner_id);
    bson_append_date_time(&set_doc, "finished_at", -1, (int64_t)time(NULL) * 1000);
    bson_append_document_end(update, &set_doc);
    
    bson_error_t error;
    bool success = mongoc_collection_update_one(collection, query, update, NULL, NULL, &error);
    
    if (success) {
        log_info("Game ended: %s, winner: %s", game_id, winner_id);
    } else {
        log_error("Failed to update game end: %s", error.message);
    }
    
    bson_destroy(query);
    bson_destroy(update);
    mongoc_collection_destroy(collection);
    mongo_release_client(g_mongo_ctx, client);
    
    return success;
}

void game_free(game_session_t *game) {
    if (!game) return;
    
    for (int i = 0; i < active_game_count; i++) {
        if (active_games[i] == game) {
            for (int j = i; j < active_game_count - 1; j++) {
                active_games[j] = active_games[j + 1];
            }
            active_game_count--;
            break;
        }
    }

    if (game) free(game);
}

void* game_timeout_monitor_thread(void* arg) {
    log_info("[TIMEOUT_MONITOR] Thread started");
    
    while (1) {
        sleep(1);
        time_t now = time(NULL);
        
        // ✅ Create snapshot
        int snapshot_count = active_game_count;
        game_session_t *snapshot[MAX_ACTIVE_GAMES];
        
        for (int i = 0; i < snapshot_count; i++) {
            snapshot[i] = active_games[i];
        }
        
        // ✅ Process snapshot
        for (int i = 0; i < snapshot_count; i++) {
            game_session_t *game = snapshot[i];
            
            if (!game || game->state != GAME_STATE_PLAYING) {
                continue;
            }
            
            int elapsed = (int)(now - game->turn_started_at);
            int remaining = game->turn_timeout_seconds - elapsed;
            
            // ✅ CASE 1: Send warning (ONLY to current player)
            if (remaining <= 10 && remaining > 0 && !game->turn_timeout_warned) {
                log_warn("[TIMEOUT_MONITOR] Game %s: %d seconds remaining for %s", 
                         game->game_id, remaining, game->current_turn);
                
                int current_socket = -1;
                
                // ✅ Compare current_turn (USER_ID) with player IDs
                if (strcmp(game->current_turn, game->player1_id) == 0) {
                    current_socket = game->player1_socket;
                    log_info("[TIMEOUT_MONITOR] Warning → Player1 (socket %d)", current_socket);
                } else if (strcmp(game->current_turn, game->player2_id) == 0) {
                    current_socket = game->player2_socket;
                    log_info("[TIMEOUT_MONITOR] Warning → Player2 (socket %d)", current_socket);
                } else {
                    log_error("[TIMEOUT_MONITOR] ❌ current_turn mismatch: '%s'", game->current_turn);
                }
                
                if (current_socket > 0) {
                    message_t warning = {0};
                    warning.type = MSG_TURN_WARNING;
                    warning.payload.turn_warning.seconds_remaining = remaining;
                    
                    ws_send_message(current_socket, &warning);
                    log_info("[TIMEOUT_MONITOR] ✅ Sent TURN_WARNING: %d seconds remaining", remaining);
                    
                    game->turn_timeout_warned = true;
                } else {
                    log_error("[TIMEOUT_MONITOR] ❌ Invalid socket!");
                }
            }
            
            // ✅ CASE 2: Timeout expired
            if (elapsed > game->turn_timeout_seconds) {
                log_error("[TIMEOUT_MONITOR] ⏰ TIMEOUT! Game %s", game->game_id);
                
                // ... (keep existing timeout logic)
                char winner_username[64] = {0};
                char loser_username[64] = {0};
                int winner_socket = -1;
                int loser_socket = -1;
                
                if (strcmp(game->current_turn, game->player1_id) == 0) {
                    // Player 1 timeout → Player 2 wins
                    user_t *winner_user = user_find_by_id(game->player2_id);
                    user_t *loser_user = user_find_by_id(game->player1_id);
                    
                    if (winner_user) {
                        strncpy(winner_username, winner_user->username, 63);
                        user_free(winner_user);
                    }
                    
                    if (loser_user) {
                        strncpy(loser_username, loser_user->username, 63);
                        user_free(loser_user);
                    }
                    
                    winner_socket = game->player2_socket;
                    loser_socket = game->player1_socket;
                } else {
                    // Player 2 timeout → Player 1 wins
                    user_t *winner_user = user_find_by_id(game->player1_id);
                    user_t *loser_user = user_find_by_id(game->player2_id);
                    
                    if (winner_user) {
                        strncpy(winner_username, winner_user->username, 63);
                        user_free(winner_user);
                    }
                    
                    if (loser_user) {
                        strncpy(loser_username, loser_user->username, 63);
                        user_free(loser_user);
                    }
                    
                    winner_socket = game->player1_socket;
                    loser_socket = game->player2_socket;
                }
                
                game->state = GAME_STATE_FINISHED;
                strncpy(game->winner_id, winner_username, 63);
                game_end(game->game_id, winner_username);
                
                message_t timeout_msg = {0};
                timeout_msg.type = MSG_GAME_TIMEOUT;
                strncpy(timeout_msg.payload.game_timeout.winner_id, winner_username, 63);
                strncpy(timeout_msg.payload.game_timeout.loser_id, loser_username, 63);
                strncpy(timeout_msg.payload.game_timeout.reason, "timeout", 63);
                
                if (winner_socket > 0) {
                    ws_send_message(winner_socket, &timeout_msg);
                    log_info("[TIMEOUT_MONITOR] ✅ Sent GAME_TIMEOUT to winner %s", winner_username);
                }
                
                if (loser_socket > 0) {
                    ws_send_message(loser_socket, &timeout_msg);
                    log_info("[TIMEOUT_MONITOR] ✅ Sent GAME_TIMEOUT to loser %s", loser_username);
                }
                game_end(game->game_id, winner_username);
    
                game->player1_socket = 0;
                game->player2_socket = 0;
                
                log_info("[TIMEOUT_MONITOR] ✅ Game %s ended. Winner: %s", 
                         game->game_id, winner_username);
            }
        }
    }
    
    return NULL;
}

// Khởi tạo timeout monitor thread
void game_init_timeout_monitor() {
    pthread_t tid;
    int result = pthread_create(&tid, NULL, game_timeout_monitor_thread, NULL);
    
    if (result == 0) {
        pthread_detach(tid);
        log_info("[TIMEOUT_MONITOR] ✅ Thread initialized successfully");
    } else {
        log_error("[TIMEOUT_MONITOR] ❌ Failed to create thread: %d", result);
    }
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

    // =================================================================================
    // BƯỚC 1: Tìm game để xác định vai trò (P1 hay P2) và trạng thái đối thủ
    // =================================================================================
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
    bool both_ready = false;

    // ✅ Khai báo buffer để lưu player IDs
    char p1_id[65] = {0};
    char p2_id[65] = {0};

    if (mongoc_cursor_next(cursor, &doc)) {
        bson_iter_t iter;
        
        // ✅ Check Player 1 (dùng tên biến khác)
        if (bson_iter_init_find(&iter, doc, "player1_id") && BSON_ITER_HOLDS_UTF8(&iter)) {
            const char *p1_id_from_db = bson_iter_utf8(&iter, NULL);  // ✅ ĐỔI TÊN
            strncpy(p1_id, p1_id_from_db, 64);  // Copy vào buffer
            p1_id[64] = '\0';
            
            if (strcmp(p1_id, player_id) == 0) {
                is_p1 = true;
            }
        }

        // ✅ Check Player 2 (dùng tên biến khác)
        if (bson_iter_init_find(&iter, doc, "player2_id") && BSON_ITER_HOLDS_UTF8(&iter)) {
            const char *p2_id_from_db = bson_iter_utf8(&iter, NULL);  // ✅ ĐỔI TÊN
            strncpy(p2_id, p2_id_from_db, 64);  // Copy vào buffer
            p2_id[64] = '\0';
            
            if (strcmp(p2_id, player_id) == 0) {
                is_p2 = true;
            }
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
    mongoc_cursor_destroy(cursor);

    if (!is_p1 && !is_p2) {
        log_error("Player %s not found in game %s", player_id, game_id);
        bson_destroy(query);
        mongoc_collection_destroy(collection);
        mongo_release_client(g_mongo_ctx, client);
        return false;
    }

    // =================================================================================
    // BƯỚC 2: Chuẩn bị dữ liệu Update
    // =================================================================================
    bson_t *update = bson_new();
    bson_t set_doc;
    BSON_APPEND_DOCUMENT_BEGIN(update, "$set", &set_doc);

    // Set trạng thái ready
    const char *ready_field = is_p1 ? "player1_ready" : "player2_ready";
    BSON_APPEND_BOOL(&set_doc, ready_field, true);

    // Nếu đối thủ đã ready, chuyển game sang trạng thái playing
    if (other_ready) {
        BSON_APPEND_UTF8(&set_doc, "phase", "playing");
        both_ready = true;
        
        // ✅ Fetch username của player1 để set làm current_turn
        user_t *p1_user = user_find_by_id(p1_id);  // ✅ DÙNG buffer p1_id
        if (p1_user) {
            BSON_APPEND_UTF8(&set_doc, "current_turn", p1_user->username);
            log_info("Set current_turn to: %s (username of player1)", p1_user->username);
            user_free(p1_user);
        } else {
            // Fallback: dùng ID nếu không tìm thấy user
            BSON_APPEND_UTF8(&set_doc, "current_turn", p1_id);
            log_warn("Could not find user for player1_id, using ID as current_turn");
        }
    }

    // --- QUAN TRỌNG: Dùng Dot Notation để chỉ update Grid, KHÔNG ghi đè Ships ---
    char grid_key_dot[64];
    if (is_p1) {
        snprintf(grid_key_dot, sizeof(grid_key_dot), "player1_board.grid");
    } else {
        snprintf(grid_key_dot, sizeof(grid_key_dot), "player2_board.grid");
    }

    bson_t grid_arr;
    BSON_APPEND_ARRAY_BEGIN(&set_doc, grid_key_dot, &grid_arr);

    for (int y = 0; y < 10; y++) {
        bson_t row;
        char key[16];
        snprintf(key, sizeof(key), "%d", y);
        BSON_APPEND_ARRAY_BEGIN(&grid_arr, key, &row);

        for (int x = 0; x < 10; x++) {
            char subkey[16];
            snprintf(subkey, sizeof(subkey), "%d", x);
            
            int val = (int)board[y * 10 + x]; 
            bson_append_int32(&row, subkey, -1, val);
        }
        bson_append_array_end(&grid_arr, &row);
    }
    bson_append_array_end(&set_doc, &grid_arr);
    
    bson_append_document_end(update, &set_doc);

    // =================================================================================
    // BƯỚC 3: Thực thi Update
    // =================================================================================
    bson_error_t error;
    if (mongoc_collection_update_one(collection, query, update, NULL, NULL, &error)) {
        success = true;
        
        // ✅ RELOAD GAME FROM DB TO UPDATE IN-MEMORY STATE
        game_session_t *game = game_get(game_id);
        if (!game) {
            // Game chưa có trong memory → load từ DB
            log_warn("Game not in cache, loading from DB: %s", game_id);
            game = game_load_from_db(game_id);
        } else {
            // Game đã có trong memory → reload board từ DB
            log_info("Reloading board from DB for game: %s", game_id);
            
            // ✅ Re-fetch document từ MongoDB - đồng bộ với ram
            bson_t *reload_query = bson_new();
            bson_oid_t reload_oid;
            bson_oid_init_from_string(&reload_oid, game_id);
            BSON_APPEND_OID(reload_query, "_id", &reload_oid);
            
            mongoc_cursor_t *reload_cursor = mongoc_collection_find_with_opts(
                collection, reload_query, NULL, NULL
            );
            
            const bson_t *reload_doc;
            if (mongoc_cursor_next(reload_cursor, &reload_doc)) {
    // ✅ Update in-memory board từ MongoDB
    if (is_p1) {
        bson_to_board(reload_doc, "player1_board", &game->player1_board);
        log_info("✅ Reloaded player1_board from DB");
        
        // ✅ DEBUG: Dump grid state
        log_info("=== PLAYER1 GRID STATE ===");
        for (int y = 0; y < 10; y++) {
            char row_str[200] = {0};
            for (int x = 0; x < 10; x++) {
                char cell[5];
                snprintf(cell, sizeof(cell), "%2d ", game->player1_board.grid[y * 10 + x]);
                strcat(row_str, cell);
            }
            log_info("Row %d: %s", y, row_str);
        }
        
        // ✅ ========== RECONSTRUCT SHIPS FROM GRID PATTERN ==========
        log_info("=== RECONSTRUCTING PLAYER1 SHIPS ===");
        
        // Reset ships array
        game->player1_board.ship_count = 0;
        game->player1_board.ships_remaining = 0;
        memset(game->player1_board.ships, 0, sizeof(game->player1_board.ships));
        
        // Track visited cells
        bool visited[BOARD_SIZE] = {false};
        
        for (int row = 0; row < GRID_SIZE; row++) {
            for (int col = 0; col < GRID_SIZE; col++) {
                int index = row * GRID_SIZE + col;
                int ship_size = game->player1_board.grid[index];
                
                // Skip if visited or water
                if (visited[index] || ship_size == 0) {
                    continue;
                }
                
                // Detect ship orientation (horizontal)
                int length = 0;
                for (int i = 0; i < ship_size && col + i < GRID_SIZE; i++) {
                    int check_index = row * GRID_SIZE + (col + i);
                    if (game->player1_board.grid[check_index] == ship_size) {
                        length++;
                        visited[check_index] = true;
                    } else {
                        break;
                    }
                }
                
                // Found complete ship
                if (length == ship_size && game->player1_board.ship_count < MAX_SHIPS) {
                    ship_t *ship = &game->player1_board.ships[game->player1_board.ship_count];
                    
                    // Map size to type
                    switch (ship_size) {
                        case 5: ship->type = SHIP_CARRIER; break;
                        case 4: ship->type = SHIP_BATTLESHIP; break;
                        case 3: ship->type = SHIP_DESTROYER; break;
                        case 2: ship->type = SHIP_SUBMARINE; break;
                        case 1: ship->type = SHIP_PATROL; break;
                        default: ship->type = SHIP_CARRIER; break;
                    }
                    
                    ship->start_row = row;
                    ship->start_col = col;
                    ship->is_horizontal = true;
                    ship->hits = 0;
                    ship->is_sunk = false;
                    
                    // ✅ CONVERT GRID VALUES FROM SIZE → CELL_SHIP
                    for (int i = 0; i < ship_size; i++) {
                        game->player1_board.grid[row * GRID_SIZE + (col + i)] = CELL_SHIP;
                    }
                    
                    game->player1_board.ship_count++;
                    game->player1_board.ships_remaining++;
                    
                    log_info("✅ Reconstructed ship %d: type=%d, pos=(%d,%d), size=%d", 
                             game->player1_board.ship_count - 1, ship->type, row, col, ship_size);
                }
            }
        }
        
        log_info("✅ Reconstruction complete: %d ships found", game->player1_board.ship_count);
        
        // ✅ DEBUG: Dump ships
        log_info("=== PLAYER1 SHIPS (AFTER RECONSTRUCTION) ===");
        for (int i = 0; i < game->player1_board.ship_count; i++) {
            ship_t *ship = &game->player1_board.ships[i];
            log_info("Ship %d: type=%d, pos=(%d,%d), horizontal=%d, hits=%d, sunk=%d",
                     i, ship->type, ship->start_row, ship->start_col,
                     ship->is_horizontal, ship->hits, ship->is_sunk);
        }
        
    } else {
        // ✅ ========== SAME LOGIC FOR PLAYER 2 ==========
        bson_to_board(reload_doc, "player2_board", &game->player2_board);
        log_info("✅ Reloaded player2_board from DB");
        
        // ✅ DEBUG: Dump grid state
        log_info("=== PLAYER2 GRID STATE ===");
        for (int y = 0; y < 10; y++) {
            char row_str[200] = {0};
            for (int x = 0; x < 10; x++) {
                char cell[5];
                snprintf(cell, sizeof(cell), "%2d ", game->player2_board.grid[y * 10 + x]);
                strcat(row_str, cell);
            }
            log_info("Row %d: %s", y, row_str);
        }
        
        // ✅ RECONSTRUCT SHIPS
        log_info("=== RECONSTRUCTING PLAYER2 SHIPS ===");
        
        game->player2_board.ship_count = 0;
        game->player2_board.ships_remaining = 0;
        memset(game->player2_board.ships, 0, sizeof(game->player2_board.ships));
        
        bool visited[BOARD_SIZE] = {false};
        
        for (int row = 0; row < GRID_SIZE; row++) {
            for (int col = 0; col < GRID_SIZE; col++) {
                int index = row * GRID_SIZE + col;
                int ship_size = game->player2_board.grid[index];
                
                if (visited[index] || ship_size == 0) {
                    continue;
                }
                
                int length = 0;
                for (int i = 0; i < ship_size && col + i < GRID_SIZE; i++) {
                    int check_index = row * GRID_SIZE + (col + i);
                    if (game->player2_board.grid[check_index] == ship_size) {
                        length++;
                        visited[check_index] = true;
                    } else {
                        break;
                    }
                }
                
                if (length == ship_size && game->player2_board.ship_count < MAX_SHIPS) {
                    ship_t *ship = &game->player2_board.ships[game->player2_board.ship_count];
                    
                    switch (ship_size) {
                        case 5: ship->type = SHIP_CARRIER; break;
                        case 4: ship->type = SHIP_BATTLESHIP; break;
                        case 3: ship->type = SHIP_DESTROYER; break;
                        case 2: ship->type = SHIP_SUBMARINE; break;
                        case 1: ship->type = SHIP_PATROL; break;
                        default: ship->type = SHIP_CARRIER; break;
                    }
                    
                    ship->start_row = row;
                    ship->start_col = col;
                    ship->is_horizontal = true;
                    ship->hits = 0;
                    ship->is_sunk = false;
                    
                    // ✅ CONVERT GRID VALUES
                    for (int i = 0; i < ship_size; i++) {
                        game->player2_board.grid[row * GRID_SIZE + (col + i)] = CELL_SHIP;
                    }
                    
                    game->player2_board.ship_count++;
                    game->player2_board.ships_remaining++;
                    
                    log_info("✅ Reconstructed ship %d: type=%d, pos=(%d,%d), size=%d", 
                             game->player2_board.ship_count - 1, ship->type, row, col, ship_size);
                }
            }
        }
        
        log_info("✅ Reconstruction complete: %d ships found", game->player2_board.ship_count);
        
        // ✅ DEBUG: Dump ships
        log_info("=== PLAYER2 SHIPS (AFTER RECONSTRUCTION) ===");
        for (int i = 0; i < game->player2_board.ship_count; i++) {
            ship_t *ship = &game->player2_board.ships[i];
            log_info("Ship %d: type=%d, pos=(%d,%d), horizontal=%d, hits=%d, sunk=%d",
                     i, ship->type, ship->start_row, ship->start_col,
                     ship->is_horizontal, ship->hits, ship->is_sunk);
        }
    }
}
            
            mongoc_cursor_destroy(reload_cursor);
            bson_destroy(reload_query);
        }
        
        // Nếu cả 2 đã ready -> Gửi thông báo Start Game
        if (both_ready && game) {
            game->state = GAME_STATE_PLAYING;
            log_info("Game %s started! Both players ready.", game_id);
            log_info("In-memory game state updated to PLAYING");
            game->turn_timeout_seconds = 30;  // 30 giây mỗi turn
            game->turn_started_at = time(NULL);
            game->turn_timeout_warned = false;
            // ✅ DEBUG: Log board state
            if (is_p1) {
                log_info("Player1 board: ship_count=%d, ships_remaining=%d",
                         game->player1_board.ship_count,
                         game->player1_board.ships_remaining);
            } else {
                log_info("Player2 board: ship_count=%d, ships_remaining=%d",
                         game->player2_board.ship_count,
                         game->player2_board.ships_remaining);
            }
            
            // --- Gửi tin nhắn START_GAME ---
            message_t start_msg = {0};
            start_msg.type = MSG_START_GAME;
            
            strncpy(start_msg.payload.start_game.game_id, game_id, 63);
            
            // ✅ Fetch username để gửi current_turn
            user_t *p1_user_for_msg = user_find_by_id(p1_id);
            if (p1_user_for_msg) {
                strncpy(start_msg.payload.start_game.current_turn, p1_user_for_msg->username, 31);
                user_free(p1_user_for_msg);
            }

            // Gửi cho Player 1
            if (game->player1_socket > 0) {
                user_t *p2_user = user_find_by_id(p2_id);
                if (p2_user) {
                    strncpy(start_msg.payload.start_game.opponent, p2_user->username, 31);
                    user_free(p2_user);
                }
                ws_send_message(game->player1_socket, &start_msg);
                log_info("✅ Sent START_GAME to player1 (socket %d)", game->player1_socket);
            }
            
            // Gửi cho Player 2
            if (game->player2_socket > 0) {
                user_t *p1_user_again = user_find_by_id(p1_id);
                if (p1_user_again) {
                    strncpy(start_msg.payload.start_game.opponent, p1_user_again->username, 31);
                    user_free(p1_user_again);
                }
                ws_send_message(game->player2_socket, &start_msg);
                log_info("✅ Sent START_GAME to player2 (socket %d)", game->player2_socket);
            }
        }
    } else {
        log_error("Failed to set player ready: %s", error.message);
    }

    bson_destroy(query);
    bson_destroy(update);
    mongoc_collection_destroy(collection);
    mongo_release_client(g_mongo_ctx, client);

    return success;
}