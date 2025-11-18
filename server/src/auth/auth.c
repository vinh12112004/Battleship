#include "auth/auth.h"
#include "auth/password.h"
#include "auth/jwt.h"
#include "database/mongo_user.h"
#include "utils/logger.h"
#include <stdlib.h>
#include <string.h>

auth_result_t* auth_register(const char *username, const char *email, const char *password) {
    auth_result_t *result = (auth_result_t*)calloc(1, sizeof(auth_result_t));
    if (!result) return NULL;
    
    // Validate input
    char error_msg[256];
    if (!password_validate(password, error_msg, sizeof(error_msg))) {
        result->success = false;
        result->error_message = _strdup(error_msg);
        log_warn("Registration failed: %s", error_msg);
        return result;
    }
    
    // Check if username exists
    user_t *existing = user_find_by_username(username);
    if (existing) {
        result->success = false;
        result->error_message = _strdup("Username already exists");
        user_free(existing);
        log_warn("Registration failed: username exists");
        return result;
    }
    
    // Hash password
    char *password_hash_str = password_hash(password);
    if (!password_hash_str) {
        result->success = false;
        result->error_message = _strdup("Failed to hash password");
        return result;
    }
    
    // Create user
    user_t *user = user_create(username, email, password_hash_str);
    free(password_hash_str);
    
    if (!user) {
        result->success = false;
        result->error_message = _strdup("Failed to create user");
        return result;
    }
    
    // Generate token
    char *token = jwt_generate(user->id);
    if (!token) {
        result->success = false;
        result->error_message = _strdup("Failed to generate token");
        user_free(user);
        return result;
    }
    
    result->success = true;
    result->token = token;
    result->user = user;
    
    log_info("User registered successfully: %s", username);
    return result;
}

auth_result_t* auth_login(const char *username, const char *password) {
    auth_result_t *result = (auth_result_t*)calloc(1, sizeof(auth_result_t));
    if (!result) return NULL;
    
    // Find user
    user_t *user = user_find_by_username(username);
    if (!user) {
        result->success = false;
        result->error_message = _strdup("Invalid username or password");
        log_warn("Login failed: user not found");
        return result;
    }
    
    // Verify password
    if (!password_verify(password, user->password_hash)) {
        result->success = false;
        result->error_message = _strdup("Invalid username or password");
        user_free(user);
        log_warn("Login failed: invalid password");
        return result;
    }
    
    // Generate token
    char *token = jwt_generate(user->id);
    if (!token) {
        result->success = false;
        result->error_message = _strdup("Failed to generate token");
        user_free(user);
        return result;
    }
    
    result->success = true;
    result->token = token;
    result->user = user;

    user_update_status(username, "online");
    
    log_info("User logged in successfully: %s", username);
    return result;
}

bool auth_logout(const char *token) {
    char *user_id = jwt_verify(token);
    if (!user_id) return false;

    user_t *user = user_find_by_id(user_id);
    free(user_id);

    if (user) {
        user_update_status(user->username, "offline");
        user_free(user);
        log_debug("User status set to offline on logout");
    }

    log_info("User logged out");
    return true;
}

user_t* auth_get_current_user(const char *token) {
    char *user_id = jwt_verify(token);
    if (!user_id) return NULL;
    
    user_t *user = user_find_by_id(user_id);
    free(user_id);
    
    return user;
}

void auth_result_free(auth_result_t *result) {
    if (!result) return;
    
    if (result->token) free(result->token);
    if (result->error_message) free(result->error_message);
    if (result->user) user_free(result->user);
    free(result);
}