#include "auth/jwt.h"
#include "config.h"
#include "utils/logger.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>

// Base64 URL encoding helper
static char* base64url_encode(const unsigned char *input, size_t length) {
    static const char base64_chars[] = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    
    size_t output_length = 4 * ((length + 2) / 3);
    char *output = (char*)malloc(output_length + 1);
    if (!output) return NULL;
    
    size_t i, j;
    for (i = 0, j = 0; i < length;) {
        uint32_t octet_a = i < length ? input[i++] : 0;
        uint32_t octet_b = i < length ? input[i++] : 0;
        uint32_t octet_c = i < length ? input[i++] : 0;
        uint32_t triple = (octet_a << 16) + (octet_b << 8) + octet_c;
        
        output[j++] = base64_chars[(triple >> 18) & 0x3F];
        output[j++] = base64_chars[(triple >> 12) & 0x3F];
        output[j++] = base64_chars[(triple >> 6) & 0x3F];
        output[j++] = base64_chars[triple & 0x3F];
    }
    
    // Remove padding
    while (j > 0 && output[j - 1] == 'A') j--;
    output[j] = '\0';
    
    return output;
}

char* jwt_generate(const char *user_id) {
    if (!user_id) {
        log_error("Cannot generate token for NULL user_id");
        return NULL;
    }
    
    time_t now = time(NULL);
    time_t exp = now + get_jwt_expiry();
    
    // Create header
    char header[] = "{\"alg\":\"HS256\",\"typ\":\"JWT\"}";
    
    // Create payload
    char payload[512];
    snprintf(payload, sizeof(payload),
        "{\"user_id\":\"%s\",\"iat\":%ld,\"exp\":%ld}",
        user_id, (long)now, (long)exp);
    
    // Base64 encode header and payload
    char *encoded_header = base64url_encode(
        (unsigned char*)header, strlen(header));
    char *encoded_payload = base64url_encode(
        (unsigned char*)payload, strlen(payload));
    
    if (!encoded_header || !encoded_payload) {
        free(encoded_header);
        free(encoded_payload);
        return NULL;
    }
    
    // Create signature input
    char signature_input[1024];
    snprintf(signature_input, sizeof(signature_input),
        "%s.%s", encoded_header, encoded_payload);
    
    // Generate HMAC SHA256 signature
    unsigned char signature[EVP_MAX_MD_SIZE];
    unsigned int signature_len;
    
    HMAC(EVP_sha256(), get_jwt_secret(), strlen(get_jwt_secret()),
         (unsigned char*)signature_input, strlen(signature_input),
         signature, &signature_len);
    
    // Base64 encode signature
    char *encoded_signature = base64url_encode(signature, signature_len);
    
    // Combine all parts
    char *token = (char*)malloc(2048);
    snprintf(token, 2048, "%s.%s.%s",
        encoded_header, encoded_payload, encoded_signature);
    
    free(encoded_header);
    free(encoded_payload);
    free(encoded_signature);
    
    log_info("Generated JWT token for user: %s", user_id);
    return token;
}

char* jwt_verify(const char *token) {
    if (!token) {
        log_error("Cannot verify NULL token");
        return NULL;
    }
    
    // Parse token parts (simplified - needs proper implementation)
    char *token_copy = _strdup(token);
    char *header = strtok(token_copy, ".");
    char *payload = strtok(NULL, ".");
    char *signature = strtok(NULL, ".");
    
    if (!header || !payload || !signature) {
        free(token_copy);
        return NULL;
    }
    
    // TODO: Verify signature
    // TODO: Decode payload
    // TODO: Check expiration
    // TODO: Extract user_id
    
    // Simplified: just return a dummy user_id for now
    char *user_id = _strdup("dummy_user_id");
    
    free(token_copy);
    return user_id;
}

bool jwt_is_expired(const char *token) {
    // TODO: Implement proper expiration check
    return false;
}