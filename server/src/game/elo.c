#include "game/elo.h"
#include "database/mongo_user.h"
#include "utils/logger.h"
#include <math.h>

double elo_expected_score(int rating_a, int rating_b) {
    // E = 1 / (1 + 10^((Rb - Ra) / 400))
    double exponent = (rating_b - rating_a) / 400.0;
    return 1.0 / (1.0 + pow(10.0, exponent));
}

elo_result_t elo_calculate(int winner_current_elo, int loser_current_elo) {
    elo_result_t result;
    
    // Tính xác suất thắng dự kiến
    double winner_expected = elo_expected_score(winner_current_elo, loser_current_elo);
    double loser_expected = elo_expected_score(loser_current_elo, winner_current_elo);
    
    // Actual score: Winner = 1, Loser = 0
    double winner_actual = 1.0;
    double loser_actual = 0.0;
    
    // Tính thay đổi ELO theo công thức: R' = R + K(A - E)
    result.winner_change = (int)round(ELO_K_FACTOR * (winner_actual - winner_expected));
    result.loser_change = (int)round(ELO_K_FACTOR * (loser_actual - loser_expected));
    
    // Tính ELO mới
    result.winner_new_elo = winner_current_elo + result.winner_change;
    result.loser_new_elo = loser_current_elo + result.loser_change;
    
    // Đảm bảo ELO không âm
    if (result.winner_new_elo < 0) result.winner_new_elo = 0;
    if (result.loser_new_elo < 0) result.loser_new_elo = 0;
    
    return result;
}

bool elo_update_after_match(const char *winner_id, const char *loser_id) {
    if (!winner_id || !loser_id) {
        log_error("Invalid user IDs for ELO update");
        return false;
    }

    // Lấy thông tin 2 người chơi từ database
    user_t *winner = user_find_by_id(winner_id);
    user_t *loser = user_find_by_id(loser_id);

    if (!winner || !loser) {
        log_error("Cannot find users for ELO calculation");
        if (winner) user_free(winner);
        if (loser) user_free(loser);
        return false;
    }

    // Tính toán ELO mới
    elo_result_t result = elo_calculate(winner->elo_rating, loser->elo_rating);

    log_info("ELO calculation - Winner: %s (%d -> %d, %+d), Loser: %s (%d -> %d, %d)",
             winner->username, winner->elo_rating, result.winner_new_elo, result.winner_change,
             loser->username, loser->elo_rating, result.loser_new_elo, result.loser_change);

    // Cập nhật ELO vào database cho cả 2 người chơi
    bool winner_updated = user_update_elo(winner_id, result.winner_new_elo);
    bool loser_updated = user_update_elo(loser_id, result.loser_new_elo);

    // Giải phóng bộ nhớ
    user_free(winner);
    user_free(loser);

    return winner_updated && loser_updated;
}