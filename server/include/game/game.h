#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include "game_board.h"
#include <stdint.h> 

// Cấu trúc mô tả tọa độ (cho shots)
typedef struct {
    uint8_t x;
    uint8_t y;
} position_t;

// Cấu trúc mô tả một lần bắn
typedef struct {
    char player_id[64];
    uint8_t x;
    uint8_t y;
    bool hit; // Kết quả của cú bắn
    long long timestamp;
} shot_t;

// Cấu trúc mô tả bảng của một người chơi
// CHỈ LƯU MẢNG 1D VỚI GIÁ TRỊ LÀ KÍCH THƯỚC THUYỀN (hoặc 0)
// typedef struct {
//     // grid: Mảng phẳng 1D (100 ô). 0: nước, 2-5: kích thước thuyền,
//     // Giá trị khác 0 sau khi hit có thể là 1 (miss) hoặc giá trị ban đầu.
//     uint8_t grid[BOARD_SIZE]; 
// } player_board_t;


typedef enum {
    GAME_STATE_PLACING_SHIPS,
    GAME_STATE_PLAYING,
    GAME_STATE_FINISHED
} game_state_t;

// Cấu trúc mô tả toàn bộ phiên chơi
typedef struct {
    char game_id[65];
    char player1_id[64];
    char player2_id[64];
    int player1_socket;
    int player2_socket;
    
    game_state_t state;
    char current_turn[64]; // player1_id hoặc player2_id
    char winner_id[64];    // ID người thắng hoặc chuỗi rỗng ("")

    // THÔNG TIN BẢNG CỦA MỖI NGƯỜI CHƠI (chỉ lưu grid)
    board_t player1_board; 
    board_t player2_board;
    
    // Danh sách các cú bắn (shots)
    shot_t shots[1000]; // Giả định tối đa 1000 cú bắn
    uint16_t num_shots;

    long long created_at;
    
    bool player1_ready;  // Ships placed?
    bool player2_ready;
} game_session_t;

bool game_is_player_turn(const char *game_id, const char *player_id);
void game_switch_turn(game_session_t *game);
game_session_t* game_find_by_player(const char *player_id);

// Game operations
bool game_create(const char *player1_id, const char *player2_id, char *out_game_id);
game_session_t* game_get(const char *game_id);
game_session_t* game_load_from_db(const char *game_id);
bool game_place_ship(const char *game_id, const char *player_id, 
                     ship_type_t type, int row, int col, bool is_horizontal);
shot_result_t game_process_shot(const char *game_id, const char *player_id, int row, int col);
bool game_update_state(const char *game_id, game_state_t new_state);
bool game_end(const char *game_id, const char *winner_id);
void game_free(game_session_t *game);

// HÀM READY: Cập nhật board của người chơi bằng mảng 1D
bool game_set_player_ready(const char *game_id, const char *player_id, const uint8_t board[BOARD_SIZE]);

#endif // GAME_H