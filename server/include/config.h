#ifndef CONFIG_H
#define CONFIG_H

#include <stdlib.h>
#include <string.h>

// MongoDB Configuration
static inline const char* get_mongo_uri() {
    const char* uri = getenv("MONGO_URI");
    return uri ? uri : "mongodb://localhost:27017";
}

static inline const char* get_mongo_db() {
    const char* db = getenv("MONGO_DB_NAME");
    return db ? db : "battleship";
}

// JWT Configuration
static inline const char* get_jwt_secret() {
    const char* secret = getenv("JWT_SECRET");
    return secret ? secret : "default_secret_change_me";
}

static inline int get_jwt_expiry() {
    const char* expiry = getenv("JWT_EXPIRY");
    return expiry ? atoi(expiry) : 604800;
}

// Validation
#define MIN_USERNAME_LENGTH 3
#define MAX_USERNAME_LENGTH 20
#define MIN_PASSWORD_LENGTH 6
#define MAX_PASSWORD_LENGTH 100

// Database Collections
#define COLLECTION_USERS "users"
#define COLLECTION_SESSIONS "sessions"
#define COLLECTION_GAMES "games"
#define COLLECTION_STATISTICS "statistics"

#endif // CONFIG_H