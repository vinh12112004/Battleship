#include "auth/password.h"
#include "config.h"
#include "utils/logger.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <openssl/sha.h>
#include <openssl/evp.h>

// Simple bcrypt-like implementation using OpenSSL
// Note: For production, use a proper bcrypt library

char* password_hash(const char *password) {
    if (!password) {
        log_error("Cannot hash NULL password");
        return NULL;
    }
    
    // Generate salt (simplified - use random bytes in production)
    unsigned char salt[16];
    unsigned char hash[SHA256_DIGEST_LENGTH];
    
    // Simple salt generation (use proper random in production)
    for (int i = 0; i < 16; i++) {
        salt[i] = (unsigned char)(rand() % 256);
    }
    
    // Hash password with salt
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
    EVP_DigestUpdate(ctx, salt, 16);
    EVP_DigestUpdate(ctx, password, strlen(password));
    EVP_DigestFinal_ex(ctx, hash, NULL);
    EVP_MD_CTX_free(ctx);
    
    // Combine salt + hash into result
    char *result = (char*)malloc(PASSWORD_HASH_LENGTH);
    if (!result) return NULL;
    
    // Format: $2b$12$[22 char salt][31 char hash]
    snprintf(result, PASSWORD_HASH_LENGTH, "$2b$12$");
    
    // Base64-like encoding of salt and hash (simplified)
    for (int i = 0; i < 16; i++) {
        sprintf(result + 7 + (i * 2), "%02x", salt[i]);
    }
    for (int i = 0; i < SHA256_DIGEST_LENGTH && i < 15; i++) {
        sprintf(result + 39 + (i * 2), "%02x", hash[i]);
    }
    
    return result;
}

bool password_verify(const char *password, const char *hash) {
    if (!password || !hash) return false;
    
    // Extract salt from hash (chars 7-38)
    if (strlen(hash) < 60) return false;
    
    unsigned char salt[16];
    unsigned char computed_hash[SHA256_DIGEST_LENGTH];
    
    // Parse salt from hex
    for (int i = 0; i < 16; i++) {
        sscanf(hash + 7 + (i * 2), "%2hhx", &salt[i]);
    }
    
    // Compute hash with same salt
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
    EVP_DigestUpdate(ctx, salt, 16);
    EVP_DigestUpdate(ctx, password, strlen(password));
    EVP_DigestFinal_ex(ctx, computed_hash, NULL);
    EVP_MD_CTX_free(ctx);
    
    // Compare hash portion
    for (int i = 0; i < 15; i++) {
        char hash_byte[3];
        snprintf(hash_byte, 3, "%02x", computed_hash[i]);
        if (strncmp(hash + 39 + (i * 2), hash_byte, 2) != 0) {
            return false;
        }
    }
    
    return true;
}

bool password_validate(const char *password, char *error_msg, size_t error_len) {
    if (!password) {
        if (error_msg) {
            snprintf(error_msg, error_len, "Password cannot be empty");
        }
        return false;
    }
    
    size_t len = strlen(password);
    
    if (len < MIN_PASSWORD_LENGTH) {
        if (error_msg) {
            snprintf(error_msg, error_len, 
                "Password must be at least %d characters", MIN_PASSWORD_LENGTH);
        }
        return false;
    }
    
    if (len > MAX_PASSWORD_LENGTH) {
        if (error_msg) {
            snprintf(error_msg, error_len, 
                "Password must be at most %d characters", MAX_PASSWORD_LENGTH);
        }
        return false;
    }
    
    bool has_letter = false;
    bool has_digit = false;
    
    for (size_t i = 0; i < len; i++) {
        if (isalpha(password[i])) has_letter = true;
        if (isdigit(password[i])) has_digit = true;
    }
    
    if (!has_letter || !has_digit) {
        if (error_msg) {
            snprintf(error_msg, error_len, 
                "Password must contain at least one letter and one number");
        }
        return false;
    }
    
    return true;
}