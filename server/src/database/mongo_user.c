#include "database/mongo_user.h"
#include "database/mongo.h"
#include "config.h"
#include "utils/logger.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

user_t* user_create(const char *username, const char *email, const char *password_hash) {
    if (!username || !email || !password_hash) {
        log_error("Invalid user creation parameters");
        return NULL;
    }
    
    mongoc_client_t *client = mongo_get_client(g_mongo_ctx);
    if (!client) return NULL;
    
    mongoc_collection_t *collection = mongo_get_collection(client, COLLECTION_USERS);
    if (!collection) {
        mongo_release_client(g_mongo_ctx, client);
        return NULL;
    }
    
    // Create BSON document
    bson_t *doc = bson_new();
    bson_oid_t oid;
    bson_oid_init(&oid, NULL);
    
    BSON_APPEND_OID(doc, "_id", &oid);
    BSON_APPEND_UTF8(doc, "username", username);
    BSON_APPEND_UTF8(doc, "email", email);
    BSON_APPEND_UTF8(doc, "password_hash", password_hash);
    BSON_APPEND_UTF8(doc, "display_name", username);
    BSON_APPEND_UTF8(doc, "status", "offline");
    BSON_APPEND_INT32(doc, "elo_rating", 1500);
    BSON_APPEND_UTF8(doc, "rank", "Silver");
    
    // Add stats subdocument
    bson_t stats;
    BSON_APPEND_DOCUMENT_BEGIN(doc, "stats", &stats);
    BSON_APPEND_INT32(&stats, "total_games", 0);
    BSON_APPEND_INT32(&stats, "wins", 0);
    BSON_APPEND_INT32(&stats, "losses", 0);
    BSON_APPEND_INT32(&stats, "draws", 0);
    BSON_APPEND_DOUBLE(&stats, "win_rate", 0.0);
    BSON_APPEND_INT32(&stats, "current_streak", 0);
    BSON_APPEND_INT32(&stats, "best_streak", 0);
    BSON_APPEND_INT32(&stats, "total_ships_sunk", 0);
    BSON_APPEND_DOUBLE(&stats, "accuracy", 0.0);
    bson_append_document_end(doc, &stats);
    
    // Add timestamps
    bson_t *now_time = bson_new();
    BSON_APPEND_NOW_UTC(doc, "created_at");
    BSON_APPEND_NOW_UTC(doc, "updated_at");
    
    // Insert document
    bson_error_t error;
    bool success = mongoc_collection_insert_one(
        collection, doc, NULL, NULL, &error
    );
    
    user_t *user = NULL;
    if (success) {
        user = (user_t*)malloc(sizeof(user_t));
        if (user) {
            char oid_str[25];
            bson_oid_to_string(&oid, oid_str);
            user->id = _strdup(oid_str);
            user->username = _strdup(username);
            user->email = _strdup(email);
            user->password_hash = _strdup(password_hash);
            user->display_name = _strdup(username);
            user->status = _strdup("offline");
            user->elo_rating = 1500;
            user->rank = _strdup("Silver");
        }
        log_info("User created: %s", username);
    } else {
        log_error("Failed to create user: %s", error.message);
    }
    
    bson_destroy(doc);
    mongoc_collection_destroy(collection);
    mongo_release_client(g_mongo_ctx, client);
    
    return user;
}

user_t* user_find_by_username(const char *username) {
    if (!username) return NULL;
    
    mongoc_client_t *client = mongo_get_client(g_mongo_ctx);
    if (!client) return NULL;
    
    mongoc_collection_t *collection = mongo_get_collection(client, COLLECTION_USERS);
    if (!collection) {
        mongo_release_client(g_mongo_ctx, client);
        return NULL;
    }
    
    // Create query
    bson_t *query = bson_new();
    BSON_APPEND_UTF8(query, "username", username);
    
    // Execute query
    mongoc_cursor_t *cursor = mongoc_collection_find_with_opts(
        collection, query, NULL, NULL
    );
    
    user_t *user = NULL;
    const bson_t *doc;
    
    if (mongoc_cursor_next(cursor, &doc)) {
        bson_iter_t iter;
        user = (user_t*)malloc(sizeof(user_t));
        
        if (bson_iter_init_find(&iter, doc, "_id")) {
            const bson_oid_t *oid = bson_iter_oid(&iter);
            char oid_str[25];
            bson_oid_to_string(oid, oid_str);
            user->id = _strdup(oid_str);
        }
        
        if (bson_iter_init_find(&iter, doc, "username")) {
            user->username = _strdup(bson_iter_utf8(&iter, NULL));
        }
        
        if (bson_iter_init_find(&iter, doc, "email")) {
            user->email = _strdup(bson_iter_utf8(&iter, NULL));
        }
        
        if (bson_iter_init_find(&iter, doc, "password_hash")) {
            user->password_hash = _strdup(bson_iter_utf8(&iter, NULL));
        }
        
        if (bson_iter_init_find(&iter, doc, "elo_rating")) {
            user->elo_rating = bson_iter_int32(&iter);
        }
        
        log_info("User found: %s", username);
    }
    
    mongoc_cursor_destroy(cursor);
    bson_destroy(query);
    mongoc_collection_destroy(collection);
    mongo_release_client(g_mongo_ctx, client);
    
    return user;
}

user_t* user_find_by_email(const char *email) {
    // Similar to user_find_by_username
    // ... (implement similarly)
    return NULL;
}

user_t* user_find_by_id(const char *user_id) {
    // ... (implement similarly)
    return NULL;
}

bool user_update_status(const char *user_id, const char *status) {
    // ... (implement update operation)
    return false;
}

void user_free(user_t *user) {
    if (!user) return;
    
    free(user->id);
    free(user->username);
    free(user->email);
    free(user->password_hash);
    free(user->display_name);
    free(user->avatar_url);
    free(user->status);
    free(user->rank);
    free(user);
}