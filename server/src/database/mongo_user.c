#include "database/mongo_user.h"
#include "database/mongo.h"
#include "config.h"
#include "utils/logger.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <bson/bson.h>

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
    bson_append_date_time(doc, "created_at", -1, (int64_t)time(NULL) * 1000);
    bson_append_date_time(doc, "updated_at", -1, (int64_t)time(NULL) * 1000);
    
    // Insert document
    bson_error_t error;
    bool success = mongoc_collection_insert_one(
        collection, doc, NULL, NULL, &error
    );
    
    user_t *user = NULL;
    if (success) {
        user = (user_t*)calloc(1, sizeof(user_t)); 
        if (user) {
            char oid_str[25];
            bson_oid_to_string(&oid, oid_str);
            user->id = strdup(oid_str);
            user->username = strdup(username);
            user->email = strdup(email);
            user->password_hash = strdup(password_hash);
            user->display_name = strdup(username);
            user->status = strdup("offline");
            user->rank = strdup("Silver");
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
        user = (user_t*)calloc(1, sizeof(user_t)); // IMPORTANT: Use calloc to zero-initialize
        
        if (!user) {
            log_error("Failed to allocate memory for user");
            goto cleanup;
        }
        
        // Initialize all pointers to NULL (already done by calloc)
        // This is critical for safe freeing later
        
        if (bson_iter_init_find(&iter, doc, "_id")) {
            const bson_oid_t *oid = bson_iter_oid(&iter);
            char oid_str[25];
            bson_oid_to_string(oid, oid_str);
            user->id = strdup(oid_str);
        }
        
        if (bson_iter_init_find(&iter, doc, "username")) {
            user->username = strdup(bson_iter_utf8(&iter, NULL));
        }
        
        if (bson_iter_init_find(&iter, doc, "email")) {
            user->email = strdup(bson_iter_utf8(&iter, NULL));
        }
        
        if (bson_iter_init_find(&iter, doc, "password_hash")) {
            user->password_hash = strdup(bson_iter_utf8(&iter, NULL));
        }
        
        // Initialize optional fields with defaults or NULL
        if (bson_iter_init_find(&iter, doc, "display_name")) {
            user->display_name = strdup(bson_iter_utf8(&iter, NULL));
        } else {
            user->display_name = NULL; // Already NULL from calloc
        }
        
        if (bson_iter_init_find(&iter, doc, "avatar_url")) {
            user->avatar_url = strdup(bson_iter_utf8(&iter, NULL));
        } else {
            user->avatar_url = NULL;
        }
        
        if (bson_iter_init_find(&iter, doc, "status")) {
            user->status = strdup(bson_iter_utf8(&iter, NULL));
        } else {
            user->status = NULL;
        }
        
        if (bson_iter_init_find(&iter, doc, "rank")) {
            user->rank = strdup(bson_iter_utf8(&iter, NULL));
        } else {
            user->rank = NULL;
        }
        
        if (bson_iter_init_find(&iter, doc, "elo_rating")) {
            user->elo_rating = bson_iter_int32(&iter);
        } else {
            user->elo_rating = 1500; // Default
        }
        
        log_info("User found: %s", username);
    }
    
cleanup:
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
    if (!user_id) return NULL;

    // Kiểm tra format ObjectId: phải 24 ký tự hex
    if (strlen(user_id) != 24) {
        log_error("Invalid ObjectId format: %s", user_id);
        return NULL;
    }

    mongoc_client_t *client = mongo_get_client(g_mongo_ctx);
    if (!client) return NULL;

    mongoc_collection_t *collection = mongo_get_collection(client, COLLECTION_USERS);
    if (!collection) {
        mongo_release_client(g_mongo_ctx, client);
        return NULL;
    }

    // Tạo query tìm theo _id
    bson_t *query = bson_new();
    bson_oid_t oid;
    bson_oid_init_from_string(&oid, user_id);  // <-- dùng bson_oid_init_from_string
    BSON_APPEND_OID(query, "_id", &oid);

    mongoc_cursor_t *cursor = mongoc_collection_find_with_opts(collection, query, NULL, NULL);

    user_t *user = NULL;
    const bson_t *doc;

    if (mongoc_cursor_next(cursor, &doc)) {
        bson_iter_t iter;
        user = (user_t*)calloc(1, sizeof(user_t)); // zero-initialize

        if (!user) {
            log_error("Failed to allocate memory for user");
            goto cleanup;
        }

        // Lấy các field cơ bản
        if (bson_iter_init_find(&iter, doc, "_id")) {
            const bson_oid_t *oid_val = bson_iter_oid(&iter);
            char oid_str[25];
            bson_oid_to_string(oid_val, oid_str);
            user->id = strdup(oid_str);
        }

        if (bson_iter_init_find(&iter, doc, "username")) {
            user->username = strdup(bson_iter_utf8(&iter, NULL));
        }

        if (bson_iter_init_find(&iter, doc, "email")) {
            user->email = strdup(bson_iter_utf8(&iter, NULL));
        }

        if (bson_iter_init_find(&iter, doc, "password_hash")) {
            user->password_hash = strdup(bson_iter_utf8(&iter, NULL));
        }

        if (bson_iter_init_find(&iter, doc, "display_name")) {
            user->display_name = strdup(bson_iter_utf8(&iter, NULL));
        }

        if (bson_iter_init_find(&iter, doc, "avatar_url")) {
            user->avatar_url = strdup(bson_iter_utf8(&iter, NULL));
        }

        if (bson_iter_init_find(&iter, doc, "status")) {
            user->status = strdup(bson_iter_utf8(&iter, NULL));
        }

        if (bson_iter_init_find(&iter, doc, "rank")) {
            user->rank = strdup(bson_iter_utf8(&iter, NULL));
        }

        if (bson_iter_init_find(&iter, doc, "elo_rating")) {
            user->elo_rating = bson_iter_int32(&iter);
        } else {
            user->elo_rating = 1500;
        }

        log_info("User found by ID: %s", user_id);
    }

cleanup:
    mongoc_cursor_destroy(cursor);
    bson_destroy(query);
    mongoc_collection_destroy(collection);
    mongo_release_client(g_mongo_ctx, client);

    return user;
}

bool user_update_status(const char *username, const char *status) {
    if (!username || !status) return false;

    mongoc_client_t *client = mongo_get_client(g_mongo_ctx);
    if (!client) return false;

    mongoc_collection_t *collection = mongo_get_collection(client, COLLECTION_USERS);
    if (!collection) {
        mongo_release_client(g_mongo_ctx, client);
        return false;
    }

    bson_t *query = bson_new();
    BSON_APPEND_UTF8(query, "username", username);

    bson_t *update = bson_new();
    bson_t child;

    BSON_APPEND_DOCUMENT_BEGIN(update, "$set", &child);
    BSON_APPEND_UTF8(&child, "status", status);
    bson_append_date_time(&child, "updated_at", -1, (int64_t)time(NULL) * 1000);
    bson_append_document_end(update, &child);

    bson_error_t error;
    bool success = mongoc_collection_update_one(
        collection, query, update, NULL, NULL, &error
    );

    if (!success)
        log_error("Failed to update user status (%s -> %s): %s", username, status, error.message);
    else
        log_info("User %s status updated to %s", username, status);

    bson_destroy(query);
    bson_destroy(update);
    mongoc_collection_destroy(collection);
    mongo_release_client(g_mongo_ctx, client);

    return success;
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

online_players_t* user_get_online_players(void) {
    mongoc_client_t *client = mongo_get_client(g_mongo_ctx);
    if (!client) return NULL;
    
    mongoc_collection_t *collection = mongo_get_collection(client, COLLECTION_USERS);
    if (!collection) {
        mongo_release_client(g_mongo_ctx, client);
        return NULL;
    }
    
    // Query: tìm users có status = "online"
    bson_t *query = bson_new();
    BSON_APPEND_UTF8(query, "status", "online");
    
    // Options: chỉ lấy các field cần thiết
    bson_t *opts = bson_new();
    bson_t projection;
    BSON_APPEND_DOCUMENT_BEGIN(opts, "projection", &projection);
    BSON_APPEND_INT32(&projection, "username", 1);
    BSON_APPEND_INT32(&projection, "elo_rating", 1);
    BSON_APPEND_INT32(&projection, "rank", 1);
    bson_append_document_end(opts, &projection);
    
    // Execute query
    mongoc_cursor_t *cursor = mongoc_collection_find_with_opts(
        collection, query, opts, NULL
    );
    
    // Đếm số lượng kết quả
    int count = 0;
    const bson_t *doc;
    
    // First pass: count
    while (mongoc_cursor_next(cursor, &doc)) {
        count++;
    }
    
    if (count == 0) {
        log_info("No online players found");
        mongoc_cursor_destroy(cursor);
        bson_destroy(query);
        bson_destroy(opts);
        mongoc_collection_destroy(collection);
        mongo_release_client(g_mongo_ctx, client);
        
        // Trả về empty list
        online_players_t *players = calloc(1, sizeof(online_players_t));
        players->count = 0;
        return players;
    }
    
    // Allocate result structure
    online_players_t *players = calloc(1, sizeof(online_players_t));
    if (!players) {
        log_error("Failed to allocate memory for online_players_t");
        mongoc_cursor_destroy(cursor);
        bson_destroy(query);
        bson_destroy(opts);
        mongoc_collection_destroy(collection);
        mongo_release_client(g_mongo_ctx, client);
        return NULL;
    }
    
    players->count = count;
    players->usernames = calloc(count, sizeof(char*));
    players->elo_ratings = calloc(count, sizeof(int));
    players->ranks = calloc(count, sizeof(char*));
    
    if (!players->usernames || !players->elo_ratings || !players->ranks) {
        log_error("Failed to allocate memory for player arrays");
        online_players_free(players);
        mongoc_cursor_destroy(cursor);
        bson_destroy(query);
        bson_destroy(opts);
        mongoc_collection_destroy(collection);
        mongo_release_client(g_mongo_ctx, client);
        return NULL;
    }
    
    // Reset cursor for second pass
    mongoc_cursor_destroy(cursor);
    cursor = mongoc_collection_find_with_opts(collection, query, opts, NULL);
    
    // Second pass: collect data
    int i = 0;
    while (mongoc_cursor_next(cursor, &doc) && i < count) {
        bson_iter_t iter;
        
        if (bson_iter_init_find(&iter, doc, "username")) {
            players->usernames[i] = strdup(bson_iter_utf8(&iter, NULL));
        } else {
            players->usernames[i] = strdup("Unknown");
        }
        
        if (bson_iter_init_find(&iter, doc, "elo_rating")) {
            players->elo_ratings[i] = bson_iter_int32(&iter);
        } else {
            players->elo_ratings[i] = 1500;
        }
        
        if (bson_iter_init_find(&iter, doc, "rank")) {
            players->ranks[i] = strdup(bson_iter_utf8(&iter, NULL));
        } else {
            players->ranks[i] = strdup("Unranked");
        }
        
        i++;
    }
    
    log_info("Found %d online players", count);
    
    // Cleanup
    mongoc_cursor_destroy(cursor);
    bson_destroy(query);
    bson_destroy(opts);
    mongoc_collection_destroy(collection);
    mongo_release_client(g_mongo_ctx, client);
    
    return players;
}

void online_players_free(online_players_t *players) {
    if (!players) return;
    
    if (players->usernames) {
        for (int i = 0; i < players->count; i++) {
            free(players->usernames[i]);
        }
        free(players->usernames);
    }
    
    if (players->elo_ratings) {
        free(players->elo_ratings);
    }
    
    if (players->ranks) {
        for (int i = 0; i < players->count; i++) {
            free(players->ranks[i]);
        }
        free(players->ranks);
    }
    
    free(players);
}

bool user_update_elo(const char *user_id, int new_elo) {
    if (!user_id) return false;
    
    // Kiểm tra format ObjectId
    if (strlen(user_id) != 24) {
        log_error("Invalid ObjectId format: %s", user_id);
        return false;
    }

    mongoc_client_t *client = mongo_get_client(g_mongo_ctx);
    if (!client) return false;

    mongoc_collection_t *collection = mongo_get_collection(client, COLLECTION_USERS);
    if (!collection) {
        mongo_release_client(g_mongo_ctx, client);
        return false;
    }

    // Tạo query
    bson_t *query = bson_new();
    bson_oid_t oid;
    bson_oid_init_from_string(&oid, user_id);
    BSON_APPEND_OID(query, "_id", &oid);

    // Tạo update document
    bson_t *update = bson_new();
    bson_t child;

    BSON_APPEND_DOCUMENT_BEGIN(update, "$set", &child);
    BSON_APPEND_INT32(&child, "elo_rating", new_elo);
    bson_append_date_time(&child, "updated_at", -1, (int64_t)time(NULL) * 1000);
    bson_append_document_end(update, &child);

    // Execute update
    bson_error_t error;
    bool success = mongoc_collection_update_one(
        collection, query, update, NULL, NULL, &error
    );

    if (!success) {
        log_error("Failed to update user ELO (%s -> %d): %s", user_id, new_elo, error.message);
    } else {
        log_info("User %s ELO updated to %d", user_id, new_elo);
    }

    bson_destroy(query);
    bson_destroy(update);
    mongoc_collection_destroy(collection);
    mongo_release_client(g_mongo_ctx, client);

    return success;
}