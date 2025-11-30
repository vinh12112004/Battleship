#include "game/game_board.h"
#include "utils/logger.h"
#include <string.h>
#include <stdlib.h>

void board_init(board_t *board) {
    memset(board->grid, CELL_WATER, sizeof(board->grid));
    memset(board->ships, 0, sizeof(board->ships));
    board->ship_count = 0;
    board->ships_remaining = 0;
}

static int get_ship_length(ship_type_t type) {
    switch (type) {
        case SHIP_CARRIER: return 5;
        case SHIP_BATTLESHIP: return 4;
        case SHIP_CRUISER: return 3;
        case SHIP_SUBMARINE: return 3;
        case SHIP_DESTROYER: return 2;
        default: return 0;
    }
}

bool board_validate_placement(board_t *board, int row, int col, int length, bool is_horizontal) {
    // Check bounds
    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) {
        log_warn("Ship placement out of bounds: (%d, %d)", row, col);
        return false;
    }
    
    // Check if ship fits
    if (is_horizontal) {
        if (col + length > BOARD_SIZE) {
            log_warn("Ship extends beyond board horizontally");
            return false;
        }
    } else {
        if (row + length > BOARD_SIZE) {
            log_warn("Ship extends beyond board vertically");
            return false;
        }
    }
    
    // Check for overlapping ships
    for (int i = 0; i < length; i++) {
        int check_row = is_horizontal ? row : row + i;
        int check_col = is_horizontal ? col + i : col;
        
        if (board->grid[check_row][check_col] == CELL_SHIP) {
            log_warn("Ship overlaps with existing ship at (%d, %d)", check_row, check_col);
            return false;
        }
        
        // Check adjacent cells (optional: enforce 1-cell spacing)
        for (int dr = -1; dr <= 1; dr++) {
            for (int dc = -1; dc <= 1; dc++) {
                int adj_row = check_row + dr;
                int adj_col = check_col + dc;
                
                if (adj_row >= 0 && adj_row < BOARD_SIZE && 
                    adj_col >= 0 && adj_col < BOARD_SIZE) {
                    if (board->grid[adj_row][adj_col] == CELL_SHIP) {
                        log_warn("Ship too close to another ship");
                        return false;
                    }
                }
            }
        }
    }
    
    return true;
}

bool board_place_ship(board_t *board, ship_type_t type, int row, int col, bool is_horizontal) {
    if (board->ship_count >= MAX_SHIPS) {
        log_warn("Maximum ships already placed");
        return false;
    }
    
    int length = get_ship_length(type);
    
    if (!board_validate_placement(board, row, col, length, is_horizontal)) {
        return false;
    }
    
    // Place ship
    ship_t *ship = &board->ships[board->ship_count];
    ship->type = type;
    ship->start_row = row;
    ship->start_col = col;
    ship->is_horizontal = is_horizontal;
    ship->hits = 0;
    ship->is_sunk = false;
    
    // Mark cells on grid
    for (int i = 0; i < length; i++) {
        int r = is_horizontal ? row : row + i;
        int c = is_horizontal ? col + i : col;
        board->grid[r][c] = CELL_SHIP;
    }
    
    board->ship_count++;
    board->ships_remaining++;
    
    log_info("Ship placed: type=%d, pos=(%d,%d), horizontal=%d", 
             type, row, col, is_horizontal);
    
    return true;
}

bool board_is_valid_shot(board_t *board, int row, int col) {
    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) {
        return false;
    }
    
    cell_state_t cell = board->grid[row][col];
    return (cell != CELL_HIT && cell != CELL_MISS);
}

ship_t* board_get_ship_at(board_t *board, int row, int col) {
    for (int i = 0; i < board->ship_count; i++) {
        ship_t *ship = &board->ships[i];
        int length = get_ship_length(ship->type);
        
        for (int j = 0; j < length; j++) {
            int ship_row = ship->is_horizontal ? ship->start_row : ship->start_row + j;
            int ship_col = ship->is_horizontal ? ship->start_col + j : ship->start_col;
            
            if (ship_row == row && ship_col == col) {
                return ship;
            }
        }
    }
    
    return NULL;
}

shot_result_t board_process_shot(board_t *board, int row, int col) {
    shot_result_t result = {false, false, 0, false};
    
    if (!board_is_valid_shot(board, row, col)) {
        log_warn("Invalid shot at (%d, %d)", row, col);
        return result;
    }
    
    cell_state_t cell = board->grid[row][col];
    
    if (cell == CELL_SHIP) {
        // HIT!
        board->grid[row][col] = CELL_HIT;
        result.is_hit = true;
        
        // Find which ship was hit
        ship_t *ship = board_get_ship_at(board, row, col);
        if (ship) {
            ship->hits++;
            
            // Check if ship is sunk
            if (ship->hits >= (int)ship->type) {
                ship->is_sunk = true;
                result.is_sunk = true;
                result.sunk_ship_type = ship->type;
                board->ships_remaining--;
                
                log_info("Ship sunk! Type=%s, remaining=%d", 
                         ship_type_to_string(ship->type), 
                         board->ships_remaining);
                
                // Check game over
                if (board->ships_remaining == 0) {
                    result.game_over = true;
                    log_info("All ships destroyed! Game over");
                }
            }
        }
        
        log_info("HIT at (%d, %d)", row, col);
    } else {
        // MISS
        board->grid[row][col] = CELL_MISS;
        result.is_hit = false;
        log_info("MISS at (%d, %d)", row, col);
    }
    
    return result;
}

const char* ship_type_to_string(ship_type_t type) {
    switch (type) {
        case SHIP_CARRIER: return "Carrier";
        case SHIP_BATTLESHIP: return "Battleship";
        case SHIP_CRUISER: return "Cruiser";
        case SHIP_SUBMARINE: return "Submarine";
        case SHIP_DESTROYER: return "Destroyer";
        default: return "Unknown";
    }
}
