#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "database/mongo.h"
#include "auth/auth.h"
#include "utils/logger.h"

int main(int argc, char *argv[]) {
    log_info("Starting Battleship Server...");
    
    // Initialize MongoDB
    const char *mongo_uri = get_mongo_uri();
    const char *mongo_db = get_mongo_db();
    
    g_mongo_ctx = mongo_init(mongo_uri, mongo_db);
    if (!g_mongo_ctx) {
        log_error("Failed to connect to MongoDB");
        return 1;
    }
    
    // Test auth functions
    log_info("Testing authentication...");
    
    // Register test user
    auth_result_t *reg_result = auth_register("testuser", "test@example.com", "password123");
    if (reg_result->success) {
        log_info("Registration successful!");
        log_info("Token: %s", reg_result->token);
        log_info("User ID: %s", reg_result->user->id);
    } else {
        log_error("Registration failed: %s", reg_result->error_message);
    }
    auth_result_free(reg_result);
    
    // Login test user
    auth_result_t *login_result = auth_login("testuser", "password123");
    if (login_result->success) {
        log_info("Login successful!");
        log_info("Token: %s", login_result->token);
    } else {
        log_error("Login failed: %s", login_result->error_message);
    }
    auth_result_free(login_result);
    
    // Cleanup
    mongo_cleanup(g_mongo_ctx);
    
    log_info("Server stopped");
    return 0;
}