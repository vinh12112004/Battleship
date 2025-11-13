#ifndef MONGO_H
#define MONGO_H

#include <mongoc/mongoc.h>
#include <bson/bson.h>
#include <stdbool.h>

typedef struct {
    mongoc_client_pool_t *pool;
    mongoc_uri_t *uri;
    char *db_name;
    bool is_connected;
} mongo_context_t;

extern mongo_context_t *g_mongo_ctx;

// Connection functions
mongo_context_t* mongo_init(const char *uri_string, const char *db_name);
void mongo_cleanup(mongo_context_t *ctx);
mongoc_client_t* mongo_get_client(mongo_context_t *ctx);
void mongo_release_client(mongo_context_t *ctx, mongoc_client_t *client);
mongoc_collection_t* mongo_get_collection(mongoc_client_t *client, const char *collection_name);

// Helper functions
bool mongo_ping(mongo_context_t *ctx);
char* bson_to_json_string(const bson_t *bson);

#endif // MONGO_H