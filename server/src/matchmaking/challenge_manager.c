#include "matchmaking/challenge_manager.h"
#include "network/ws_protocol.h"
#include "utils/logger.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

static challenge_session_t challenges[MAX_CHALLENGES];
static int challenge_count = 0;

extern ssize_t ws_send_message(int sock, message_t *msg);

void challenge_manager_init(void) {
    memset(challenges, 0, sizeof(challenges));
    challenge_count = 0;
    log_info("Challenge manager initialized");
}

void challenge_manager_cleanup(void) {
    challenge_count = 0;
    log_info("Challenge manager cleaned up");
}

static void generate_challenge_id(char *out_id) {
    sprintf(out_id, "chal_%ld_%d", time(NULL), rand() % 10000);
}

char* challenge_create(const char *challenger_id, const char *target_id,
                       int challenger_sock, int target_sock,
                       const char *game_mode, int time_control) {
    if (challenge_count >= MAX_CHALLENGES) {
        log_error("Challenge limit reached");
        return NULL;
    }
    
    // Check if already has pending challenge between these players
    for (int i = 0; i < MAX_CHALLENGES; i++) {
        if (challenges[i].is_active && 
            challenges[i].status == CHALLENGE_STATUS_PENDING) {
            if ((strcmp(challenges[i].challenger_id, challenger_id) == 0 &&
                 strcmp(challenges[i].target_id, target_id) == 0) ||
                (strcmp(challenges[i].challenger_id, target_id) == 0 &&
                 strcmp(challenges[i].target_id, challenger_id) == 0)) {
                log_warn("Challenge already exists between %s and %s", 
                         challenger_id, target_id);
                return NULL;
            }
        }
    }

    int slot = -1;
    for (int i = 0; i < MAX_CHALLENGES; i++) {
        if (!challenges[i].is_active) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        log_error("No free challenge slot");
        return NULL;
    }
    
    challenge_session_t *c = &challenges[slot];
    
    generate_challenge_id(c->challenge_id);
    strncpy(c->challenger_id, challenger_id, 63);
    strncpy(c->target_id, target_id, 63);
    c->challenger_socket = challenger_sock;
    c->target_socket = target_sock;
    strncpy(c->game_mode, game_mode, 31);
    c->time_control = time_control;
    c->created_at = time(NULL);
    c->expires_at = c->created_at + CHALLENGE_EXPIRE_TIME;
    c->status = CHALLENGE_STATUS_PENDING;
    c->is_active = true;
    
    challenge_count++;
    
    log_info("Challenge created: %s (%s → %s)", 
             c->challenge_id, challenger_id, target_id);
    
    return c->challenge_id;
}

bool challenge_accept(const char *challenge_id) {
    challenge_session_t *c = challenge_get(challenge_id);
    if (!c || c->status != CHALLENGE_STATUS_PENDING) {
        return false;
    }
    
    c->status = CHALLENGE_STATUS_ACCEPTED;
    log_info("Challenge accepted: %s", challenge_id);
    return true;
}

bool challenge_decline(const char *challenge_id) {
    challenge_session_t *c = challenge_get(challenge_id);
    if (!c || c->status != CHALLENGE_STATUS_PENDING) {
        return false;
    }
    
    c->status = CHALLENGE_STATUS_DECLINED;
    log_info("Challenge declined: %s", challenge_id);
    return true;
}

bool challenge_cancel(const char *challenge_id) {
    challenge_session_t *c = challenge_get(challenge_id);
    if (!c || c->status != CHALLENGE_STATUS_PENDING) {
        return false;
    }
    
    c->status = CHALLENGE_STATUS_CANCELLED;
    log_info("Challenge cancelled: %s", challenge_id);
    return true;
}

bool challenge_remove(const char *challenge_id) {
    for (int i = 0; i < MAX_CHALLENGES; i++) {
        if (challenges[i].is_active && 
            strcmp(challenges[i].challenge_id, challenge_id) == 0) {
            memset(&challenges[i], 0, sizeof(challenge_session_t));
            challenge_count--;
            log_info("Challenge removed: %s", challenge_id);
            return true;
        }
    }
    return false;
}

challenge_session_t* challenge_get(const char *challenge_id) {
    for (int i = 0; i < MAX_CHALLENGES; i++) {
        if (challenges[i].is_active && 
            strcmp(challenges[i].challenge_id, challenge_id) == 0) {
            return &challenges[i];
        }
    }
    return NULL;
}

challenge_session_t* challenge_find_by_challenger(const char *user_id) {
    for (int i = 0; i < MAX_CHALLENGES; i++) {
        if (challenges[i].is_active && 
            strcmp(challenges[i].challenger_id, user_id) == 0 &&
            challenges[i].status == CHALLENGE_STATUS_PENDING) {
            return &challenges[i];
        }
    }
    return NULL;
}

challenge_session_t* challenge_find_by_target(const char *user_id) {
    for (int i = 0; i < MAX_CHALLENGES; i++) {
        if (challenges[i].is_active && 
            strcmp(challenges[i].target_id, user_id) == 0 &&
            challenges[i].status == CHALLENGE_STATUS_PENDING) {
            return &challenges[i];
        }
    }
    return NULL;
}

void challenge_check_expired(void) {
    int64_t now = time(NULL);
    
    for (int i = 0; i < MAX_CHALLENGES; i++) {
        if (challenges[i].is_active && 
            challenges[i].status == CHALLENGE_STATUS_PENDING &&
            now >= challenges[i].expires_at) {
            
            log_info("Challenge expired: %s", challenges[i].challenge_id);
            
            // Update status
            challenges[i].status = CHALLENGE_STATUS_EXPIRED;
            
            // ✅ SEND EXPIRATION MESSAGES TO BOTH PLAYERS
            
            // Send to challenger
            if (challenges[i].challenger_socket > 0) {
                message_t expire_msg = {0};
                expire_msg.type = MSG_CHALLENGE_EXPIRED;
                strncpy(expire_msg.payload.challenge_resp.challenge_id, 
                        challenges[i].challenge_id, 64);
                
                ws_send_message(challenges[i].challenger_socket, &expire_msg);
                log_info("Sent CHALLENGE_EXPIRED to challenger (socket %d)", 
                         challenges[i].challenger_socket);
            }
            
            // Send to target
            if (challenges[i].target_socket > 0) {
                message_t expire_msg = {0};
                expire_msg.type = MSG_CHALLENGE_EXPIRED;
                strncpy(expire_msg.payload.challenge_resp.challenge_id, 
                        challenges[i].challenge_id, 64);
                
                ws_send_message(challenges[i].target_socket, &expire_msg);
                log_info("Sent CHALLENGE_EXPIRED to target (socket %d)", 
                         challenges[i].target_socket);
            }
            
            // ✅ Remove challenge after expiration
            challenge_remove(challenges[i].challenge_id);
        }
    }
}
