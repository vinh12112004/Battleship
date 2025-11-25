#include "matchmaking/matcher.h"
#include "game/game.h"
#include "network/ws_protocol.h"
#include "utils/logger.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_QUEUE_SIZE 1000
#define ELO_TOLERANCE 200 // Chênh lệch ELO tối đa

static queue_player_t queue[MAX_QUEUE_SIZE];
static int queue_count = 0;

void matcher_init() {
    queue_count = 0;
    log_info("Matchmaking system initialized");
}

void matcher_cleanup() {
    queue_count = 0;
    log_info("Matchmaking system cleaned up");
}

// ==================== Queue Operations ====================
bool matcher_add_to_queue(int client_sock, const char *user_id, int elo_rating, const char *game_type) {
    if (queue_count >= MAX_QUEUE_SIZE) {
        log_warn("Queue full, cannot add player %s", user_id);
        return false;
    }
    
    // Kiểm tra xem player đã trong queue chưa
    for (int i = 0; i < queue_count; i++) {
        if (strcmp(queue[i].user_id, user_id) == 0) {
            log_warn("Player %s already in queue", user_id);
            return false;
        }
    }
    
    // Thêm vào queue
    queue_player_t *player = &queue[queue_count];
    strncpy(player->user_id, user_id, 63);
    player->user_id[63] = '\0';
    player->socket = client_sock;
    player->elo_rating = elo_rating;
    strncpy(player->game_type, game_type, 31);
    player->game_type[31] = '\0';
    player->join_time = time(NULL);
    
    queue_count++;
    log_info("Player %s added to queue (ELO: %d, Type: %s). Queue size: %d", 
             user_id, elo_rating, game_type, queue_count);
    
    // Tự động tìm trận đấu
    matcher_find_match();
    
    return true;
}

bool matcher_remove_from_queue(const char *user_id) {
    for (int i = 0; i < queue_count; i++) {
        if (strcmp(queue[i].user_id, user_id) == 0) {
            // Xóa bằng cách shift array
            for (int j = i; j < queue_count - 1; j++) {
                queue[j] = queue[j + 1];
            }
            queue_count--;
            log_info("Player %s removed from queue. Queue size: %d", user_id, queue_count);
            return true;
        }
    }
    return false;
}

queue_player_t* matcher_get_player(const char *user_id) {
    for (int i = 0; i < queue_count; i++) {
        if (strcmp(queue[i].user_id, user_id) == 0) {
            return &queue[i];
        }
    }
    return NULL;
}

int matcher_get_queue_size() {
    return queue_count;
}

// ==================== Matchmaking Logic ====================
void matcher_find_match() {
    if (queue_count < 2) {
        log_debug("Not enough players in queue (%d)", queue_count);
        return;
    }
    
    // Simple matching: tìm 2 player có ELO gần nhau
    for (int i = 0; i < queue_count - 1; i++) {
        for (int j = i + 1; j < queue_count; j++) {
            queue_player_t *p1 = &queue[i];
            queue_player_t *p2 = &queue[j];
            
            // Kiểm tra game type giống nhau
            if (strcmp(p1->game_type, p2->game_type) != 0) {
                continue;
            }
            
            // Kiểm tra ELO chênh lệch
            int elo_diff = abs(p1->elo_rating - p2->elo_rating);
            if (elo_diff > ELO_TOLERANCE) {
                continue;
            }
            
            log_info("Match found! %s (ELO: %d) vs %s (ELO: %d)", 
                     p1->user_id, p1->elo_rating, p2->user_id, p2->elo_rating);
            
            // Tạo game session
            char game_id[65];
            if (!game_create(p1->user_id, p2->user_id, game_id)) {
                log_error("Failed to create game for %s vs %s", p1->user_id, p2->user_id);
                continue;
            }
            
            // Gửi START_GAME message cho cả 2 players
            message_t msg1 = {0};
            msg1.type = MSG_START_GAME;
            strncpy(msg1.payload.start_game.opponent, p2->user_id, 31);
            // ✅ TODO: Thêm gameId vào payload (cần update struct start_game_payload trong ws_protocol.h)
            ws_send_message(p1->socket, &msg1);
            
            message_t msg2 = {0};
            msg2.type = MSG_START_GAME;
            strncpy(msg2.payload.start_game.opponent, p1->user_id, 31);
            ws_send_message(p2->socket, &msg2);
            
            log_info("Game started: %s", game_id);
            
            // Xóa cả 2 players khỏi queue
            matcher_remove_from_queue(p1->user_id);
            matcher_remove_from_queue(p2->user_id);
            
            // Tiếp tục tìm match cho các player còn lại
            matcher_find_match();
            return;
        }
    }
    
    log_debug("No suitable matches found");
}
