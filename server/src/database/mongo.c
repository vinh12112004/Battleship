#include "database/mongo.h"
#include "config.h"
#include "utils/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

mongo_context_t *g_mongo_ctx = NULL;

mongo_context_t* mongo_init(const char *uri_string, const char *db_name) {
    mongo_context_t *ctx = (mongo_context_t*)malloc(sizeof(mongo_context_t));
    if (!ctx) {
        log_error("Failed to allocate mongo context");
        return NULL;
    }
    
    mongoc_init();
    
    bson_error_t error;
    ctx->uri = mongoc_uri_new_with_error(uri_string, &error);
    if (!ctx->uri) {
        log_error("Failed to parse MongoDB URI: %s", error.message);
        free(ctx);
        return NULL;
    }
    
    ctx->pool = mongoc_client_pool_new(ctx->uri);
    if (!ctx->pool) {
        log_error("Failed to create MongoDB connection pool");
        mongoc_uri_destroy(ctx->uri);
        free(ctx);
        return NULL;
    }
    
    mongoc_client_pool_max_size(ctx->pool, 10);
    ctx->db_name = strdup(db_name);
    ctx->is_connected = false;
    
    if (mongo_ping(ctx)) {
        ctx->is_connected = true;
        log_info("MongoDB connected successfully to %s", db_name);
    } else {
        log_error("MongoDB connection test failed");
        mongoc_client_pool_destroy(ctx->pool);
        mongoc_uri_destroy(ctx->uri);
        free(ctx->db_name);
        free(ctx);
        return NULL;
    }
    
    return ctx;
}

void mongo_cleanup(mongo_context_t *ctx) {
    if (!ctx) return;
    
    if (ctx->pool) mongoc_client_pool_destroy(ctx->pool);
    if (ctx->uri) mongoc_uri_destroy(ctx->uri);
    if (ctx->db_name) free(ctx->db_name);
    
    free(ctx);
    mongoc_cleanup();
    log_info("MongoDB connection closed");
}

mongoc_client_t* mongo_get_client(mongo_context_t *ctx) {
    if (!ctx || !ctx->pool) {
        log_error("Invalid mongo context");
        return NULL;
    }
    return mongoc_client_pool_pop(ctx->pool);
}

void mongo_release_client(mongo_context_t *ctx, mongoc_client_t *client) {
    if (ctx && ctx->pool && client) {
        mongoc_client_pool_push(ctx->pool, client);
    }
}

mongoc_collection_t* mongo_get_collection(mongoc_client_t *client, const char *collection_name) {
    if (!client || !collection_name) return NULL;
    
    mongoc_database_t *db = mongoc_client_get_database(client, g_mongo_ctx->db_name);
    mongoc_collection_t *collection = mongoc_database_get_collection(db, collection_name);
    mongoc_database_destroy(db);
    
    return collection;
}

bool mongo_ping(mongo_context_t *ctx) {
    if (!ctx || !ctx->pool) return false;
    
    mongoc_client_t *client = mongo_get_client(ctx);
    if (!client) return false;
    
    bson_t *command = BCON_NEW("ping", BCON_INT32(1));
    bson_t reply;
    bson_error_t error;
    
    bool success = mongoc_client_command_simple(
        client, "admin", command, NULL, &reply, &error
    );
    
    if (!success) {
        log_error("MongoDB ping failed: %s", error.message);
    }
    
    bson_destroy(command);
    bson_destroy(&reply);
    mongo_release_client(ctx, client);
    
    return success;
}

char* bson_to_json_string(const bson_t *bson) {
    if (!bson) return NULL;
    
    size_t length;
    char *str = bson_as_canonical_extended_json(bson, &length);
    return str;
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