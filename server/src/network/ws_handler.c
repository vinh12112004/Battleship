#include "network/ws_handler.h"
#include "network/ws_server.h"
#include "auth/auth.h"
#include "game/game.h"
#include "game/game_chat.h"
#include "matchmaking/matcher.h"
#include "utils/logger.h"
#include <stdio.h>
#include <string.h>
#include "auth/jwt.h"
#include "game/game.h"
#include "game/game_board.h"

void handle_message(int client_sock, message_t *msg) {
    log_debug("Handling WebSocket message type=%d for client %d", msg->type, client_sock);

    auth_user_t user; 
    int requires_auth = 0;

    // Các message cần xác thực
    switch(msg->type) {
        case MSG_JOIN_QUEUE:
        case MSG_PLAYER_MOVE:
        case MSG_CHAT:
        case MSG_LOGOUT:
            requires_auth = 1;
            break;
        case MSG_PLACE_SHIP:
            requires_auth = 1;
            break;
        default:
            break;
    }

    if (requires_auth) {
        if (!check_token(client_sock, msg->token, &user)) {
            // Token không hợp lệ => dừng xử lý
            return;
        }
    }

    // Gọi handler tương ứng
    switch(msg->type) {
        case MSG_REGISTER:
            handle_register(client_sock, &msg->payload.auth);
            break;
        case MSG_LOGIN:
            handle_login(client_sock, &msg->payload.auth);
            break;
        case MSG_PLAYER_MOVE:
            handle_player_move(client_sock, msg);
            break;
        case MSG_CHAT:
            handle_chat(client_sock, &msg->payload.chat, msg->token);
            break;
        case MSG_LOGOUT:
            handle_logout(client_sock, msg);
            break;
        case MSG_PING:
            log_debug("Received MSG_PING from client %d", client_sock);
            message_t pong = {0};
            pong.type = MSG_PONG;
            ws_send_message(client_sock, &pong);
            break;
            
        case MSG_PONG:
            log_debug("Received MSG_PONG from client %d", client_sock);
            break;   
        case MSG_JOIN_QUEUE:
            handle_join_queue(client_sock, msg->token);
            break;
            
        case MSG_LEAVE_QUEUE:
            handle_leave_queue(client_sock, msg->token);
            break;
        case MSG_PLACE_SHIP:
            handle_place_ship(client_sock, &msg->payload.place_ship, msg->token);
            break;
        case MSG_PLAYER_READY:
            handle_player_ready(client_sock, msg);
            break;
        case MSG_GET_ONLINE_PLAYERS:
            handle_get_online_players(client_sock, msg->token);
            break;
        default:
            log_warn("Unknown message type: %d from client %d", msg->type, client_sock);
            break;
    }

    log_debug("Message handler finished for type=%d, client=%d", msg->type, client_sock);
}


void handle_register(int client_sock, auth_payload *auth) {
    if (!auth || !auth->username[0] || !auth->password[0]) {
        log_warn("Invalid registration payload from client %d", client_sock);
        return;
    }

    log_info("Processing registration for username: %s", auth->username);

    // Call auth module to register
    auth_result_t *res = auth_register(auth->username, auth->username, auth->password);
    if (!res) {
        log_error("auth_register returned NULL for client %d", client_sock);
        return;
    }

    // Create response
    message_t resp;

    if (res->success) {
        resp.type = MSG_AUTH_SUCCESS;
        strncpy(resp.payload.auth_suc.token, res->token, MAX_JWT_LEN - 1);
        resp.payload.auth_suc.token[MAX_JWT_LEN - 1] = '\0';
        strncpy(resp.payload.auth_suc.username, res->user->username, 31);
        resp.payload.auth_suc.username[31] = '\0';
        client_register(client_sock, res->user->id);
        log_info("Registration successful for %s", auth->username);
    } else {
        resp.type = MSG_AUTH_FAILED;
        strncpy(resp.payload.auth_fail.reason, res->error_message, 63);
        resp.payload.auth_fail.reason[63] = '\0';
        log_warn("Registration failed for client %d: %s", client_sock, res->error_message);
    }

    // Send message via WebSocket
    ssize_t sent = ws_send_message((int)client_sock, &resp);
    if (sent <= 0) {
        log_error("Failed to send registration response to client %d", client_sock);
    }

    log_debug("About to free auth_result for client %d", client_sock);
    auth_result_free(res);
    log_debug("auth_result freed successfully for client %d", client_sock);
}

void handle_login(int client_sock, auth_payload *auth) {
    log_info("Processing login for username: %s", auth->username);
    
    auth_result_t *res = auth_login(auth->username, auth->password);
    message_t resp;

    if (res->success) {
        resp.type = MSG_AUTH_SUCCESS;
        strncpy(resp.payload.auth_suc.token, res->token, MAX_JWT_LEN - 1);
        resp.payload.auth_suc.token[MAX_JWT_LEN - 1] = '\0';
        strncpy(resp.payload.auth_suc.username, res->user->username, 31);
        resp.payload.auth_suc.username[31] = '\0';
        client_register(client_sock, res->user->id);
        log_info("Login successful for %s, sending response...", auth->username);
    } else {
        resp.type = MSG_AUTH_FAILED;
        strncpy(resp.payload.auth_fail.reason, res->error_message, 63);
        resp.payload.auth_fail.reason[63] = '\0';
        
        log_warn("Login failed for %s: %s", auth->username, res->error_message);
    }

    ssize_t sent = ws_send_message(client_sock, &resp);
    if (sent <= 0) {
        log_error("Failed to send login response to client %d: %d", client_sock, sent);
    } else {
        log_info("Login response sent successfully to client %d (%zd bytes)", client_sock, sent);
    }
    
    log_debug("About to free auth_result for client %d", client_sock);
    auth_result_free(res);
    log_debug("auth_result freed successfully for client %d", client_sock);
}

void handle_logout(int client_sock, message_t *msg) {
    auth_user_t user;
    if (!check_token(client_sock, msg->token, &user)) return;

    log_info("User %s logging out", user.user_id);
    auth_logout(msg->token);

    message_t resp = {0};
    resp.type = MSG_AUTH_SUCCESS;
    ws_send_message(client_sock, &resp);
}


// void handle_join_queue(int client_sock, const char *token) {
//     auth_user_t user;
//     if (!check_token(client_sock, token, &user)) {
//         return; // dừng nếu token invalid
//     }

//     log_info("Client %d (%s) joining queue", client_sock, user.user_id);
//     add_to_queue(client_sock, user.user_id);
// }

void handle_player_move(int client_sock, message_t *msg) {
    const char *token = msg->token;
    move_payload *move = &msg->payload.move;

    // Verify JWT
    char *user_id = jwt_verify(token);
    if (!user_id) {
        log_error("Invalid token in PLAYER_MOVE");
        return;
    }
    
    // Validate coordinates
    if (move->row < 0 || move->row >= 10 || move->col < 0 || move->col >= 10) {
        log_error("Invalid coordinates from player %s: row=%d, col=%d",
                  user_id, move->row, move->col);
        free(user_id);
        return;
    }
    
    log_info("Player %s shooting at (%d, %d) in game %s", 
             user_id, move->row, move->col, move->game_id);
    
    // Process shot
    shot_result_t result = game_process_shot(move->game_id, user_id, move->row, move->col);
    
    // ===== Response to shooter =====
    message_t response = {0};
    response.type = MSG_MOVE_RESULT;
    response.payload.move_res.row = move->row;
    response.payload.move_res.col = move->col;
    response.payload.move_res.is_hit = result.is_hit;
    response.payload.move_res.is_sunk = result.is_sunk;
    response.payload.move_res.sunk_ship_type = result.sunk_ship_type;
    response.payload.move_res.game_over = result.game_over;
    response.payload.move_res.is_your_shot = 1;  // ✅ SHOOTER: is_your_shot = 1
    
    ws_send_message(client_sock, &response);
    log_info("✅ Sent MOVE_RESULT to shooter: hit=%d, sunk=%d, game_over=%d",
             result.is_hit, result.is_sunk, result.game_over);
    
    // ===== Send to opponent =====
    game_session_t *game = game_get(move->game_id);
    if (game) {
        int opponent_sock = (client_sock == game->player1_socket) 
                            ? game->player2_socket 
                            : game->player1_socket;
        
        if (opponent_sock > 0) {
            message_t opponent_msg = {0};
            opponent_msg.type = MSG_MOVE_RESULT;
            opponent_msg.payload.move_res.row = move->row;
            opponent_msg.payload.move_res.col = move->col;
            opponent_msg.payload.move_res.is_hit = result.is_hit;
            opponent_msg.payload.move_res.is_sunk = result.is_sunk;
            opponent_msg.payload.move_res.sunk_ship_type = result.sunk_ship_type;
            opponent_msg.payload.move_res.game_over = result.game_over;
            opponent_msg.payload.move_res.is_your_shot = 0;  // ✅ DEFENDER: is_your_shot = 0
            
            ws_send_message(opponent_sock, &opponent_msg);
            log_info("✅ Sent MOVE_RESULT to opponent (socket %d)", opponent_sock);
        } else {
            log_warn("Opponent socket not found for game %s", move->game_id);
        }
    } else {
        log_error("Game session not found: %s", move->game_id);
    }
    
    free(user_id);
}

void handle_chat(int client_sock, chat_payload *chat, const char *token) {
    // Verify token
    char *user_id = jwt_verify(token);
    if (!user_id) {
        log_warn("Invalid token for chat message");
        message_t resp = {0};
        resp.type = MSG_AUTH_FAILED;
        strncpy(resp.payload.auth_fail.reason, "Invalid token", 63);
        ws_send_message(client_sock, &resp);
        return;
    }
    
    log_info("Chat message from user %s (socket %d): %s", 
             user_id, client_sock, chat->message);
    
    // Delegate to game_chat module
    bool success = game_chat_send_message(
        chat->game_id,
        user_id,
        chat->message
    );
    
    if (!success) {
        log_error("Failed to send chat message for user %s in game %s", 
                  user_id, chat->game_id);
    }
    
    free(user_id);
}

int check_token(int client_sock, const char *token, auth_user_t *out_user) {
    char *user_id = jwt_verify(token);
    if (!user_id) {
        log_warn("Client %d gửi token không hợp lệ", client_sock);

        message_t resp;
        resp.type = MSG_AUTH_FAILED;
        strncpy(resp.payload.auth_fail.reason, "Invalid or expired token", 63);
        resp.payload.auth_fail.reason[63] = '\0';
        ws_send_message(client_sock, &resp);

        return 0; // không hợp lệ
    }

    strncpy(out_user->user_id, user_id, 63);
    out_user->user_id[63] = '\0';
    free(user_id);

    return 1; // hợp lệ
}

void handle_join_queue(int client_sock, const char *token) {
    // Verify token
    char *user_id = jwt_verify(token);
    if (!user_id) {
        log_warn("Invalid token for join queue");
        message_t resp = {0};
        resp.type = MSG_AUTH_FAILED;
        strncpy(resp.payload.auth_fail.reason, "Invalid token", 63);
        ws_send_message(client_sock, &resp);
        return;
    }
    
    // Get user info
    user_t *user = user_find_by_id(user_id);
    if (!user) {
        log_error("User not found: %s", user_id);
        free(user_id);
        return;
    }
    
    // Add to matchmaking queue
    bool success = matcher_add_to_queue(
        client_sock, 
        user_id, 
        user->elo_rating, 
        "ranked" // Default game type
    );
    
    if (success) {
        log_info("Player %s joined queue (ELO: %d)", user->username, user->elo_rating);
        
        // Send confirmation (optional)
        // message_t resp = {0};
        // resp.type = MSG_QUEUE_JOINED;
        // ws_send_message(client_sock, &resp);
    } else {
        log_error("Failed to add player %s to queue", user->username);
    }
    
    user_free(user);
    free(user_id);
}

void handle_leave_queue(int client_sock, const char *token) {
    char *user_id = jwt_verify(token);
    if (!user_id) return;
    
    matcher_remove_from_queue(user_id);
    log_info("Player %s left queue", user_id);
    
    free(user_id);
}

void handle_place_ship(int client_sock, place_ship_payload *payload, const char *token) {
    // Verify token
    char *user_id = jwt_verify(token);
    if (!user_id) {
        log_warn("Invalid token for place ship");
        message_t resp = {0};
        resp.type = MSG_AUTH_FAILED;
        strncpy(resp.payload.auth_fail.reason, "Invalid token", 63);
        ws_send_message(client_sock, &resp);
        return;
    }
    
    log_info("Player %s placing ship: type=%d, pos=(%d,%d), horizontal=%d",
             user_id, payload->ship_type, payload->row, payload->col, payload->is_horizontal);
    
    // Find active game for this player
    game_session_t *game = game_find_by_player(user_id);
    if (!game) {
        log_error("No active game found for player: %s", user_id);
        
        message_t resp = {0};
        resp.type = MSG_AUTH_FAILED;
        strncpy(resp.payload.auth_fail.reason, "No active game", 63);
        ws_send_message(client_sock, &resp);
        
        free(user_id);
        return;
    }
    
    // Validate ship type
    ship_type_t ship_type;
    switch (payload->ship_type) {
        case 5: ship_type = SHIP_CARRIER; break;
        case 4: ship_type = SHIP_BATTLESHIP; break;
        case 3: ship_type = SHIP_DESTROYER; break;
        case 2: ship_type = SHIP_SUBMARINE; break;
        case 1: ship_type = SHIP_PATROL; break;
        default:
            log_error("Invalid ship type: %d", payload->ship_type);
            
            message_t resp = {0};
            resp.type = MSG_AUTH_FAILED;
            strncpy(resp.payload.auth_fail.reason, "Invalid ship type", 63);
            ws_send_message(client_sock, &resp);
            
            free(user_id);
            return;
    }
    
    // Place ship
    bool success = game_place_ship(
        game->game_id,
        user_id,
        ship_type,
        payload->row,
        payload->col,
        payload->is_horizontal
    );
    
    message_t resp = {0};
    
    if (success) {
        resp.type = MSG_AUTH_SUCCESS;  // Or create MSG_PLACE_SHIP_SUCCESS
        log_info("Ship placed successfully for player %s", user_id);
        
        // Check if both players ready
        if (game->player1_ready && game->player2_ready) {
            log_info("Both players ready! Game %s starting...", game->game_id);
            
            // Send game start notification to both players
            message_t start_msg = {0};
            start_msg.type = MSG_START_GAME;
            strncpy(start_msg.payload.start_game.opponent, 
                    strcmp(game->player1_id, user_id) == 0 ? game->player2_id : game->player1_id,
                    31);
            
            ws_send_message(client_sock, &start_msg);
            
            // Send to opponent
            int opponent_sock = (strcmp(game->player1_id, user_id) == 0) ?
                                game->player2_socket : game->player1_socket;
            if (opponent_sock > 0) {
                strncpy(start_msg.payload.start_game.opponent, user_id, 31);
                ws_send_message(opponent_sock, &start_msg);
            }
        }
    } else {
        resp.type = MSG_AUTH_FAILED;
        strncpy(resp.payload.auth_fail.reason, "Failed to place ship (overlap/out of bounds)", 63);
        log_warn("Failed to place ship for player %s", user_id);
    }
    
    ws_send_message(client_sock, &resp);
    free(user_id);
}

void handle_player_ready(int client_sock, message_t *msg) {
    // Verify token
    char *user_id = jwt_verify(msg->token);
    if (!user_id) {
        log_error("Invalid token in PLAYER_READY");
        return;
    }

    // Lấy dữ liệu payload
    const char *game_id = msg->payload.ready.game_id;
    const uint8_t *board = msg->payload.ready.board_state;

    log_info("Player %s READY for game %s", user_id, game_id);

    // Gọi game logic nhưng KHÔNG gửi phản hồi
    if (!game_set_player_ready(game_id, user_id, board)) {
        log_error("Failed to set READY for player %s", user_id);
        // Không gửi message lại
    }

    free(user_id);
}
void handle_get_online_players(int client_sock, const char *token) {
    // Verify token
    char *user_id = jwt_verify(token);
    if (!user_id) {
        log_warn("Invalid token for get online players");
        message_t resp = {0};
        resp.type = MSG_AUTH_FAILED;
        strncpy(resp.payload.auth_fail.reason, "Invalid token", 63);
        ws_send_message(client_sock, &resp);
        return;
    }
    
    log_info("User %s requesting online players list", user_id);
    
    // Lấy danh sách online players từ database
    online_players_t *players = user_get_online_players();
    
    if (!players) {
        log_error("Failed to get online players");
        free(user_id);
        return;
    }
    
    // Tạo response message
    message_t resp = {0};
    resp.type = MSG_ONLINE_PLAYERS_LIST;
    resp.payload.online_players.count = players->count;
    
    // Copy player data
    for (int i = 0; i < players->count && i < 50; i++) {
        strncpy(resp.payload.online_players.players[i], 
                players->usernames[i], 63);
        resp.payload.online_players.players[i][63] = '\0';
        
        resp.payload.online_players.elo_ratings[i] = players->elo_ratings[i];
        
        strncpy(resp.payload.online_players.ranks[i], 
                players->ranks[i], 31);
        resp.payload.online_players.ranks[i][31] = '\0';
    }
    
    // Send response
    ssize_t sent = ws_send_message(client_sock, &resp);
    if (sent <= 0) {
        log_error("Failed to send online players list to client %d", client_sock);
    } else {
        log_info("Sent online players list to client %d (%d players)", 
                 client_sock, players->count);
    }
    
    // Cleanup
    online_players_free(players);
    free(user_id);
}
