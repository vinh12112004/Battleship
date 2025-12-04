#ifndef ELO_H
#define ELO_H

#include <stdbool.h>

// Hằng số K-factor cho hệ thống ELO
#define ELO_K_FACTOR 32

// Cấu trúc lưu kết quả tính toán ELO
typedef struct {
    int winner_new_elo;
    int loser_new_elo;
    int winner_change;
    int loser_change;
} elo_result_t;

/**
 * Tính toán ELO mới cho 2 người chơi sau trận đấu
 * @param winner_current_elo ELO hiện tại của người thắng
 * @param loser_current_elo ELO hiện tại của người thua
 * @return Kết quả tính toán ELO mới
 */
elo_result_t elo_calculate(int winner_current_elo, int loser_current_elo);

/**
 * Tính xác suất thắng dự kiến (Expected score)
 * @param rating_a ELO của người chơi A
 * @param rating_b ELO của người chơi B
 * @return Xác suất thắng của người chơi A (0.0 - 1.0)
 */
double elo_expected_score(int rating_a, int rating_b);

/**
 * Cập nhật ELO cho 2 người chơi sau trận đấu
 * @param winner_id ID của người thắng
 * @param loser_id ID của người thua
 * @return true nếu cập nhật thành công, false nếu thất bại
 */
bool elo_update_after_match(const char *winner_id, const char *loser_id);

#endif // ELO_H