#include "game/game_chat.h"
#include "game/game.h"
#include "database/mongo.h"
#include "database/mongo_user.h"
#include "network/ws_protocol.h"
#include "network/ws_server.h"
#include "utils/logger.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define COLLECTION_CHAT "game_chats"
#define MAX_CACHED_CHATS 50

// In-memory cache for chat histories
static game_chat_history_t *chat_cache[MAX_CACHED_CHATS];
static int cached_chat_count = 0;

// ==================== Helper: Get or Create Chat History ====================
static game_chat_history_t* get_or_create_chat_history(const char *game_id) {
    // Check cache first
    for (int i = 0; i < cached_chat_count; i++) {
        if (strcmp(chat_cache[i]->game_id, game_id) == 0) {
            return chat_cache[i];
        }
    }
    
    // Not in cache, try loading from DB
    game_chat_history_t *history = game_chat_load_from_db(game_id);
    
    // If not in DB, create new
    if (!history) {
        history = (game_chat_history_t*)calloc(1, sizeof(game_chat_history_t));
        strncpy(history->game_id, game_id, 64);
        history->message_count = 0;
    }
    
    // Add to cache if space available
    if (cached_chat_count < MAX_CACHED_CHATS) {
        chat_cache[cached_chat_count++] = history;
    }
    
    return history;
}

// ==================== Send Chat Message ====================
bool game_chat_send_message(const char *game_id, const char *sender_id, const char *text) {
    if (!sender_id || !text) {
        log_error("Invalid parameters for chat message");
        return false;
    }
    
    // If game_id not provided, find game by player
    game_session_t *game = NULL;
    if (game_id && game_id[0] != '\0') {
        game = game_get(game_id);
    } else {
        game = game_find_by_player(sender_id);
    }
    
    if (!game) {
        log_error("Game not found for player: %s", sender_id);
        return false;
    }
    
    // Get sender info
    user_t *sender = user_find_by_id(sender_id);
    if (!sender) {
        log_error("Sender not found: %s", sender_id);
        return false;
    }
    
    // Create chat message
    chat_message_t msg;
    memset(&msg, 0, sizeof(msg));
    
    strncpy(msg.sender_id, sender_id, 63);
    strncpy(msg.sender_name, sender->username, 31);
    strncpy(msg.text, text, MAX_CHAT_MESSAGE_LENGTH - 1);
    msg.timestamp = (int64_t)time(NULL) * 1000;
    
    // Save to chat history
    game_chat_history_t *history = get_or_create_chat_history(game->game_id);
    if (history->message_count < MAX_CHAT_HISTORY) {
        history->messages[history->message_count++] = msg;
    } else {
        // Shift old messages and add new one
        memmove(&history->messages[0], &history->messages[1], 
                sizeof(chat_message_t) * (MAX_CHAT_HISTORY - 1));
        history->messages[MAX_CHAT_HISTORY - 1] = msg;
    }
    
    // Save to database
    game_chat_save_to_db(game->game_id, &msg);
    
    // Prepare WebSocket message
    message_t ws_msg = {0};
    ws_msg.type = MSG_CHAT_MESSAGE;
    
    strncpy(ws_msg.payload.chat_msg.username, sender->username, 63);
    strncpy(ws_msg.payload.chat_msg.text, text, 127);
    
    // Send to both players
    bool sent = false;
    
    if (game->player1_socket > 0) {
        ws_send_message(game->player1_socket, &ws_msg);
        log_info("Chat message sent to player1 (socket %d)", game->player1_socket);
        sent = true;
    }
    
    if (game->player2_socket > 0) {
        ws_send_message(game->player2_socket, &ws_msg);
        log_info("Chat message sent to player2 (socket %d)", game->player2_socket);
        sent = true;
    }
    
    user_free(sender);
    
    if (!sent) {
        log_warn("No active sockets to send chat message");
    }
    
    return sent;
}

// ==================== Get Chat History ====================
game_chat_history_t* game_chat_get_history(const char *game_id) {
    return get_or_create_chat_history(game_id);
}

// ==================== Save to Database ====================
bool game_chat_save_to_db(const char *game_id, const chat_message_t *message) {
    mongoc_client_t *client = mongo_get_client(g_mongo_ctx);
    if (!client) return false;
    
    mongoc_collection_t *collection = mongo_get_collection(client, COLLECTION_CHAT);
    if (!collection) {
        mongo_release_client(g_mongo_ctx, client);
        return false;
    }
    
    bson_t *query = bson_new();
    BSON_APPEND_UTF8(query, "game_id", game_id);
    
    // Create message document
    bson_t msg_doc;
    BSON_APPEND_DOCUMENT_BEGIN(query, "message", &msg_doc);
    BSON_APPEND_UTF8(&msg_doc, "sender_id", message->sender_id);
    BSON_APPEND_UTF8(&msg_doc, "sender_name", message->sender_name);
    BSON_APPEND_UTF8(&msg_doc, "text", message->text);
    bson_append_date_time(&msg_doc, "timestamp", -1, message->timestamp);
    bson_append_document_end(query, &msg_doc);
    
    // Use $push to add message to array
    bson_t *update = bson_new();
    bson_t push_doc, msg_array;
    
    BSON_APPEND_DOCUMENT_BEGIN(update, "$push", &push_doc);
    BSON_APPEND_DOCUMENT_BEGIN(&push_doc, "messages", &msg_array);
    BSON_APPEND_UTF8(&msg_array, "sender_id", message->sender_id);
    BSON_APPEND_UTF8(&msg_array, "sender_name", message->sender_name);
    BSON_APPEND_UTF8(&msg_array, "text", message->text);
    bson_append_date_time(&msg_array, "timestamp", -1, message->timestamp);
    bson_append_document_end(&push_doc, &msg_array);
    bson_append_document_end(update, &push_doc);
    
    // Set upsert option
    bson_t *opts = bson_new();
    BSON_APPEND_BOOL(opts, "upsert", true);
    
    bson_error_t error;
    bool success = mongoc_collection_update_one(collection, query, update, opts, NULL, &error);
    
    if (!success) {
        log_error("Failed to save chat message: %s", error.message);
    }
    
    bson_destroy(query);
    bson_destroy(update);
    bson_destroy(opts);
    mongoc_collection_destroy(collection);
    mongo_release_client(g_mongo_ctx, client);
    
    return success;
}

// ==================== Load from Database ====================
game_chat_history_t* game_chat_load_from_db(const char *game_id) {
    mongoc_client_t *client = mongo_get_client(g_mongo_ctx);
    if (!client) return NULL;
    
    mongoc_collection_t *collection = mongo_get_collection(client, COLLECTION_CHAT);
    if (!collection) {
        mongo_release_client(g_mongo_ctx, client);
        return NULL;
    }
    
    bson_t *query = bson_new();
    BSON_APPEND_UTF8(query, "game_id", game_id);
    
    mongoc_cursor_t *cursor = mongoc_collection_find_with_opts(collection, query, NULL, NULL);
    const bson_t *doc;
    
    game_chat_history_t *history = NULL;
    
    if (mongoc_cursor_next(cursor, &doc)) {
        history = (game_chat_history_t*)calloc(1, sizeof(game_chat_history_t));
        strncpy(history->game_id, game_id, 64);
        
        bson_iter_t iter, array_iter;
        
        if (bson_iter_init_find(&iter, doc, "messages") && 
            BSON_ITER_HOLDS_ARRAY(&iter) &&
            bson_iter_recurse(&iter, &array_iter)) {
            
            int idx = 0;
            while (bson_iter_next(&array_iter) && idx < MAX_CHAT_HISTORY) {
                bson_iter_t msg_iter;
                if (bson_iter_recurse(&array_iter, &msg_iter)) {
                    chat_message_t *msg = &history->messages[idx];
                    
                    if (bson_iter_find(&msg_iter, "sender_id"))
                        strncpy(msg->sender_id, bson_iter_utf8(&msg_iter, NULL), 63);
                    
                    if (bson_iter_find(&msg_iter, "sender_name"))
                        strncpy(msg->sender_name, bson_iter_utf8(&msg_iter, NULL), 31);
                    
                    if (bson_iter_find(&msg_iter, "text"))
                        strncpy(msg->text, bson_iter_utf8(&msg_iter, NULL), MAX_CHAT_MESSAGE_LENGTH - 1);
                    
                    if (bson_iter_find(&msg_iter, "timestamp"))
                        msg->timestamp = bson_iter_date_time(&msg_iter);
                    
                    idx++;
                }
            }
            history->message_count = idx;
        }
        
        log_info("Loaded %d chat messages for game %s", history->message_count, game_id);
    }
    
    mongoc_cursor_destroy(cursor);
    bson_destroy(query);
    mongoc_collection_destroy(collection);
    mongo_release_client(g_mongo_ctx, client);
    
    return history;
}

// ==================== Free Chat History ====================
void game_chat_free(game_chat_history_t *history) {
    if (!history) return;
    
    // Remove from cache
    for (int i = 0; i < cached_chat_count; i++) {
        if (chat_cache[i] == history) {
            for (int j = i; j < cached_chat_count - 1; j++) {
                chat_cache[j] = chat_cache[j + 1];
            }
            cached_chat_count--;
            break;
        }
    }
    
    free(history);
}