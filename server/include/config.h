#ifndef CONFIG_H
#define CONFIG_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

// ======================= Load .env =======================
// Simple parser for key=value
static inline void load_env_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) return;

    char line[512];
    while (fgets(line, sizeof(line), file)) {
        // Remove trailing newline
        size_t len = strlen(line);
        if (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) line[len-1] = '\0';

        // Ignore comments or empty lines
        if (line[0] == '#' || line[0] == '\0') continue;

        // Split at first '='
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        char *key = line;
        char *value = eq + 1;

        // Trim spaces
        while(isspace((unsigned char)*key)) key++;
        while(isspace((unsigned char)*value)) value++;

        // Set environment variable only if not already set
        if (!getenv(key)) {
#ifdef _WIN32
            _putenv_s(key, value);
#else
            setenv(key, value, 0);
#endif
        }
    }

    fclose(file);
}

// Automatically load .env when this header is included
__attribute__((constructor))
static void auto_load_env() {
    load_env_file(".env");
}

// ======================= MongoDB =======================
static inline const char* get_mongo_uri() {
    const char* uri = getenv("MONGO_URI");
    return uri ? uri : "mongodb://localhost:27019";
}

static inline const char* get_mongo_db() {
    const char* db = getenv("MONGO_DB_NAME");
    return db ? db : "battleship";
}

// ======================= JWT ===========================
static inline const char* get_jwt_secret() {
    const char* secret = getenv("JWT_SECRET");
    return secret ? secret : "default_secret_change_me";
}

static inline int get_jwt_expiry() {
    const char* expiry = getenv("JWT_EXPIRY");
    return expiry ? atoi(expiry) : 604800;
}

// ======================= Validation ===================
#define MIN_USERNAME_LENGTH 3
#define MAX_USERNAME_LENGTH 20
#define MIN_PASSWORD_LENGTH 6
#define MAX_PASSWORD_LENGTH 100

// ======================= Collections ==================
#define COLLECTION_USERS "users"
#define COLLECTION_SESSIONS "sessions"
#define COLLECTION_GAMES "games"
#define COLLECTION_STATISTICS "statistics"

#endif // CONFIG_H
