#include "game/game.h"
#include "game/game_board.h"
#include "database/mongo.h"
#include "utils/logger.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

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
    
    // Serialize grid (100 cells)
    bson_t grid_array;
    BSON_APPEND_ARRAY_BEGIN(&board_doc, "grid", &grid_array);
    for (int i = 0; i < BOARD_SIZE * BOARD_SIZE; i++) {
        char idx[8];
        snprintf(idx, sizeof(idx), "%d", i);
        BSON_APPEND_INT32(&grid_array, idx, (int)board->grid[i / BOARD_SIZE][i % BOARD_SIZE]);
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
    
    // Deserialize grid
    bson_iter_init(&child, doc);
    if (bson_iter_find(&child, key) && bson_iter_recurse(&child, &child) &&
        bson_iter_find(&child, "grid") && bson_iter_recurse(&child, &array_iter)) {
        int idx = 0;
        while (bson_iter_next(&array_iter) && idx < BOARD_SIZE * BOARD_SIZE) {
            int row = idx / BOARD_SIZE;
            int col = idx % BOARD_SIZE;
            board->grid[row][col] = (cell_state_t)bson_iter_int32(&array_iter);
            idx++;
        }
    }
    
    return true;
}

// ==================== Game Create ====================
bool game_create(const char *player1_id, const char *player2_id, char *out_game_id) {
    if (active_game_count >= MAX_ACTIVE_GAMES) {
        log_error("Maximum active games reached");
        return false;
    }
    
    mongoc_client_t *client = mongo_get_client(g_mongo_ctx);
    if (!client) return false;
    
    mongoc_collection_t *collection = mongo_get_collection(client, COLLECTION_GAMES);
    if (!collection) {
        mongo_release_client(g_mongo_ctx, client);
        return false;
    }
    
    // Create game document
    bson_t *doc = bson_new();
    bson_oid_t oid;
    bson_oid_init(&oid, NULL);
    
    BSON_APPEND_OID(doc, "_id", &oid);
    BSON_APPEND_UTF8(doc, "player1_id", player1_id);
    BSON_APPEND_UTF8(doc, "player2_id", player2_id);
    BSON_APPEND_UTF8(doc, "state", "placing_ships");
    BSON_APPEND_UTF8(doc, "current_turn", player1_id);
    BSON_APPEND_BOOL(doc, "player1_ready", false);
    BSON_APPEND_BOOL(doc, "player2_ready", false);
    bson_append_date_time(doc, "created_at", -1, (int64_t)time(NULL) * 1000);
    bson_append_date_time(doc, "updated_at", -1, (int64_t)time(NULL) * 1000);
    
    // Initialize empty boards
    board_t empty_board;
    board_init(&empty_board);
    board_to_bson(doc, "player1_board", &empty_board);
    board_to_bson(doc, "player2_board", &empty_board);
    
    bson_error_t error;
    bool success = mongoc_collection_insert_one(collection, doc, NULL, NULL, &error);
    
    if (success) {
        char oid_str[25];
        bson_oid_to_string(&oid, oid_str);
        strcpy(out_game_id, oid_str);
        
        // Create in-memory session
        game_session_t *game = (game_session_t*)calloc(1, sizeof(game_session_t));
        strncpy(game->game_id, oid_str, 64);
        strncpy(game->player1_id, player1_id, 63);
        strncpy(game->player2_id, player2_id, 63);
        game->state = GAME_STATE_PLACING_SHIPS;
        strncpy(game->current_turn, player1_id, 63);
        game->created_at = time(NULL);
        board_init(&game->player1_board);
        board_init(&game->player2_board);
        
        active_games[active_game_count++] = game;
        
        log_info("Game created: %s (%s vs %s)", oid_str, player1_id, player2_id);
    } else {
        log_error("Failed to create game: %s", error.message);
    }
    
    bson_destroy(doc);
    mongoc_collection_destroy(collection);
    mongo_release_client(g_mongo_ctx, client);
    
    return success;
}

// ==================== Game Get (with DB fallback) ====================
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
        
        if (bson_iter_init_find(&iter, doc, "state")) {
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
    
    if (!game_is_player_turn(game_id, player_id)) {
        log_warn("Not player's turn: %s", player_id);
        return result;
    }
    
    board_t *target_board = NULL;
    
    if (strcmp(game->player1_id, player_id) == 0) {
        target_board = &game->player2_board;
    } else if (strcmp(game->player2_id, player_id) == 0) {
        target_board = &game->player1_board;
    } else {
        log_error("Player not in game");
        return result;
    }
    
    result = board_process_shot(target_board, row, col);
    
    if (result.game_over) {
        game->state = GAME_STATE_FINISHED;
        log_info("Game over! Winner: %s", player_id);
        game_end(game_id, player_id);
    } else {
        game_switch_turn(game);
    }
    
    // ✅ Sync to DB after every move
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
    
    free(game);
}