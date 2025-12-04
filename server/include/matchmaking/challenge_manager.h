#ifndef CHALLENGE_MANAGER_H
#define CHALLENGE_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

#define MAX_CHALLENGES 100
#define CHALLENGE_EXPIRE_TIME 60  // 60 seconds

typedef enum {
    CHALLENGE_STATUS_PENDING,
    CHALLENGE_STATUS_ACCEPTED,
    CHALLENGE_STATUS_DECLINED,
    CHALLENGE_STATUS_EXPIRED,
    CHALLENGE_STATUS_CANCELLED
} challenge_status_t;

typedef struct {
    char challenge_id[65];        // UUID
    char challenger_id[64];       // User ID
    char target_id[64];           // User ID
    int challenger_socket;
    int target_socket;
    char game_mode[32];
    int time_control;
    int64_t created_at;           // Timestamp (seconds)
    int64_t expires_at;           // Timestamp (seconds)
    challenge_status_t status;
    bool is_active;
} challenge_session_t;

// Challenge operations
void challenge_manager_init(void);
void challenge_manager_cleanup(void);

// Create/Delete
char* challenge_create(const char *challenger_id, const char *target_id, 
                       int challenger_sock, int target_sock,
                       const char *game_mode, int time_control);
bool challenge_cancel(const char *challenge_id);
bool challenge_remove(const char *challenge_id);

// Status updates
bool challenge_accept(const char *challenge_id);
bool challenge_decline(const char *challenge_id);
bool challenge_expire(const char *challenge_id);

// Lookup
challenge_session_t* challenge_get(const char *challenge_id);
challenge_session_t* challenge_find_by_challenger(const char *user_id);
challenge_session_t* challenge_find_by_target(const char *user_id);

// Cleanup
void challenge_check_expired(void);

#endif