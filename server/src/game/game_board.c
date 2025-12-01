#include "game/game_board.h"
#include "utils/logger.h"
#include <string.h>
#include <stdlib.h>

#define INDEX(row, col) ((row) * GRID_SIZE + (col))

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
        case SHIP_DESTROYER: return 3;  
        case SHIP_SUBMARINE: return 2;   
        case SHIP_PATROL: return 1;       
        default: return 0;
    }
}

bool board_validate_placement(board_t *board, int row, int col, int length, bool is_horizontal) {
    // Check bounds
    if (row < 0 || row >= GRID_SIZE || col < 0 || col >= GRID_SIZE) {
        log_warn("Ship placement out of bounds: (%d, %d)", row, col);
        return false;
    }
    
    // Check if ship fits
    if (is_horizontal) {
        if (col + length > GRID_SIZE) {
            log_warn("Ship extends beyond board horizontally");
            return false;
        }
    } else {
        if (row + length > GRID_SIZE) {
            log_warn("Ship extends beyond board vertically");
            return false;
        }
    }
    
    // Check for overlapping ships
    for (int i = 0; i < length; i++) {
        int check_row = is_horizontal ? row : row + i;
        int check_col = is_horizontal ? col + i : col;
        
        // Access 1D array using INDEX macro
        if (board->grid[INDEX(check_row, check_col)] == CELL_SHIP) {
            log_warn("Ship overlaps with existing ship at (%d, %d)", check_row, check_col);
            return false;
        }
        
        // Check adjacent cells (optional: enforce 1-cell spacing)
        for (int dr = -1; dr <= 1; dr++) {
            for (int dc = -1; dc <= 1; dc++) {
                int adj_row = check_row + dr;
                int adj_col = check_col + dc;
                
                if (adj_row >= 0 && adj_row < GRID_SIZE && 
                    adj_col >= 0 && adj_col < GRID_SIZE) {
                    if (board->grid[INDEX(adj_row, adj_col)] == CELL_SHIP) {
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
    
    // Check trùng lặp
    for (int i = 0; i < board->ship_count; i++) {
        ship_t *existing = &board->ships[i];
        if (existing->type == type && 
            existing->start_row == row && 
            existing->start_col == col &&
            existing->is_horizontal == is_horizontal) {
            return false;
        }
    }
    
    if (!board_validate_placement(board, row, col, length, is_horizontal)) {
        return false;
    }
    
    ship_t *ship = &board->ships[board->ship_count];
    ship->type = type;
    ship->start_row = row;
    ship->start_col = col;
    ship->is_horizontal = is_horizontal;
    ship->hits = 0;
    ship->is_sunk = false;
    
    for (int i = 0; i < length; i++) {
        int r = is_horizontal ? row : row + i;
        int c = is_horizontal ? col + i : col;
        
        board->grid[INDEX(r, c)] = CELL_SHIP; 
    }
    
    board->ship_count++;
    board->ships_remaining++;
    
    log_info("Ship placed: type=%d, len=%d, pos=(%d,%d)", type, length, row, col);
    return true;
}

bool board_is_valid_shot(board_t *board, int row, int col) {
    if (row < 0 || row >= GRID_SIZE || col < 0 || col >= GRID_SIZE) {
        return false;
    }

    cell_state_t cell = board->grid[INDEX(row, col)];
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
    
    // DEBUG: Log input
    log_info("=== board_process_shot DEBUG ===");
    log_info("Shooting at (%d, %d)", row, col);
    log_info("Board ship_count: %d", board->ship_count);
    log_info("Board ships_remaining: %d", board->ships_remaining);
    
    if (!board_is_valid_shot(board, row, col)) {
        log_warn("Invalid shot at (%d, %d)", row, col);
        return result;
    }
    
    // Check CELL STATE
    cell_state_t cell = board->grid[INDEX(row, col)];
    log_info("Cell value at (%d,%d): %d (0=WATER, 1=SHIP, 2=HIT, 3=MISS)", 
             row, col, cell);
    
    // Nếu ô đó là tàu (CELL_SHIP hoặc ID tàu > 0)
    if (cell == CELL_SHIP) {
        log_info("✅ Cell is SHIP, processing HIT...");
        
        // 1. Đánh dấu ô đó đã bị bắn trúng
        board->grid[INDEX(row, col)] = CELL_HIT;
        result.is_hit = true;
        
        // 2. Tìm xem bắn trúng tàu nào
        ship_t *ship = board_get_ship_at(board, row, col);
        if (ship) {
            log_info("Found ship: type=%d, pos=(%d,%d), hits=%d",
                     ship->type, ship->start_row, ship->start_col, ship->hits);
            
            ship->hits++;
            
            // So sánh với ĐỘ DÀI thật của tàu
            int max_health = get_ship_length(ship->type);
            
            log_info("Ship Hit! Current HP: %d/%d", ship->hits, max_health);

            if (ship->hits >= max_health) {
                ship->is_sunk = true;
                result.is_sunk = true;
                result.sunk_ship_type = (int)ship->type;
                board->ships_remaining--;
                
                log_info("Ship SUNK! Type=%s", ship_type_to_string(ship->type));
                
                if (board->ships_remaining == 0) {
                    result.game_over = true;
                    log_info("GAME OVER! All ships destroyed");
                }
            }
        } else {
            log_error("❌ Ghost Ship detected at (%d, %d)! Grid says SHIP but no ship struct found.", 
                     row, col);
            
            log_info("=== ALL SHIPS DUMP ===");
            for (int i = 0; i < board->ship_count; i++) {
                ship_t *s = &board->ships[i];
                log_info("Ship %d: type=%d, pos=(%d,%d), horizontal=%d, hits=%d, sunk=%d",
                         i, s->type, s->start_row, s->start_col,
                         s->is_horizontal, s->hits, s->is_sunk);
            }
        }
    } else {
        log_info("❌ Cell is NOT SHIP (value=%d), marking as MISS", cell);
        
        board->grid[INDEX(row, col)] = CELL_MISS;
        result.is_hit = false;
        log_info("MISS at (%d, %d)", row, col);
    }
    
    return result;
}

const char* ship_type_to_string(ship_type_t type) {
    switch (type) {
        case SHIP_CARRIER: return "Carrier";
        case SHIP_BATTLESHIP: return "Battleship";
        case SHIP_DESTROYER: return "Destroyer";
        case SHIP_SUBMARINE: return "Submarine";
        case SHIP_PATROL: return "Patrol Boat";
        default: return "Unknown";
    }
}

void board_debug_print(board_t *board, const char *label) {
    log_info("=== BOARD DEBUG: %s ===", label);
    log_info("Ship count: %d, Ships remaining: %d", 
             board->ship_count, board->ships_remaining);
    
    // Print grid
    log_info("Grid (0=WATER, 1=SHIP, 2=HIT, 3=MISS):");
    for (int y = 0; y < 10; y++) {
        char row_str[200] = {0};
        char row_label[10];
        snprintf(row_label, sizeof(row_label), "Row %d: ", y);
        strcat(row_str, row_label);
        
        for (int x = 0; x < 10; x++) {
            char cell[5];
            snprintf(cell, sizeof(cell), "%2d ", board->grid[y * 10 + x]);
            strcat(row_str, cell);
        }
        log_info("%s", row_str);
    }
    
    // Print ships
    log_info("Ships:");
    for (int i = 0; i < board->ship_count; i++) {
        ship_t *ship = &board->ships[i];
        int ship_len = get_ship_length(ship->type);
        log_info("  Ship %d: type=%d (%s), pos=(%d,%d), horizontal=%d, hits=%d/%d, sunk=%d",
                 i, ship->type, ship_type_to_string(ship->type),
                 ship->start_row, ship->start_col, ship->is_horizontal,
                 ship->hits, ship_len, ship->is_sunk);
    }
}