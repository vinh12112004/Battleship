#ifndef GAME_BOARD_H
#define GAME_BOARD_H

#include <stdbool.h>

#define BOARD_SIZE 10
#define MAX_SHIPS 5

// Ship types
typedef enum {
    SHIP_CARRIER = 5,
    SHIP_BATTLESHIP = 4,
    SHIP_CRUISER = 3,
    SHIP_SUBMARINE = 31, 
    SHIP_DESTROYER = 2
} ship_type_t;

// Cell states
typedef enum {
    CELL_WATER = 0,
    CELL_SHIP = 1,
    CELL_HIT = 2,
    CELL_MISS = 3
} cell_state_t;

// Ship structure
typedef struct {
    ship_type_t type;
    int start_row;
    int start_col;
    bool is_horizontal;
    int hits;  // Number of hits on this ship
    bool is_sunk;
} ship_t;

// Player board
typedef struct {
    cell_state_t grid[BOARD_SIZE][BOARD_SIZE];
    ship_t ships[MAX_SHIPS];
    int ship_count;
    int ships_remaining;
} board_t;

// Board operations
void board_init(board_t *board);
bool board_place_ship(board_t *board, ship_type_t type, int row, int col, bool is_horizontal);
bool board_validate_placement(board_t *board, int row, int col, int length, bool is_horizontal);
bool board_is_valid_shot(board_t *board, int row, int col);

// Shot result structure
typedef struct {
    bool is_hit;
    bool is_sunk;
    ship_type_t sunk_ship_type;
    bool game_over;
} shot_result_t;

shot_result_t board_process_shot(board_t *board, int row, int col);
ship_t* board_get_ship_at(board_t *board, int row, int col);
const char* ship_type_to_string(ship_type_t type);

#endif