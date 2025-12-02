#ifndef GAME_CHAT_H
#define GAME_CHAT_H

#include <stdbool.h>
#include <stdint.h>

#define MAX_CHAT_MESSAGE_LENGTH 512
#define MAX_CHAT_HISTORY 100

// Chat message structure
typedef struct {
    char sender_id[64];      // Player ID của người gửi
    char sender_name[32];    // Username của người gửi
    char text[MAX_CHAT_MESSAGE_LENGTH];
    int64_t timestamp;       // Unix timestamp (ms)
} chat_message_t;

// Chat history for a game
typedef struct {
    char game_id[65];
    chat_message_t messages[MAX_CHAT_HISTORY];
    int message_count;
} game_chat_history_t;

/**
 * Send a chat message in a game
 * @param game_id Game ID (optional, can be NULL to auto-detect from sender_id)
 * @param sender_id Player ID của người gửi
 * @param text Nội dung tin nhắn
 * @return true if successful
 */
bool game_chat_send_message(const char *game_id, const char *sender_id, const char *text);

/**
 * Get chat history for a game
 * @param game_id Game ID
 * @return Chat history or NULL if not found
 */
game_chat_history_t* game_chat_get_history(const char *game_id);

/**
 * Save chat message to database
 * @param game_id Game ID
 * @param message Chat message to save
 * @return true if successful
 */
bool game_chat_save_to_db(const char *game_id, const chat_message_t *message);

/**
 * Load chat history from database
 * @param game_id Game ID
 * @return Chat history or NULL if not found
 */
game_chat_history_t* game_chat_load_from_db(const char *game_id);

/**
 * Free chat history
 * @param history Chat history to free
 */
void game_chat_free(game_chat_history_t *history);

#endif // GAME_CHAT_H