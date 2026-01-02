#include <stdio.h>
#include "network/tcp_server.h"
#include "utils/logger.h"
#include "database/mongo.h"
#include "config.h"
#include "matchmaking/matcher.h"

int main() {
    // 1️⃣ Log server start
    log_info("Starting Battleship TCP Server...");

    // 2️⃣ Initialize MongoDB
    const char *mongo_uri = get_mongo_uri();
    const char *mongo_db = get_mongo_db();

    g_mongo_ctx = mongo_init(mongo_uri, mongo_db);
    if (!g_mongo_ctx) {
        log_error("Failed to connect to MongoDB");
        return 1;
    }

    log_info("MongoDB connected successfully.");
    
    // 3️⃣ Initialize matcher
    matcher_init();
    
    // 4️⃣ Start TCP server
    uint16_t port = 9090;
    log_info("Starting TCP server on port %d...", port);
    start_tcp_server(port);  // <- vòng lặp accept client bên trong

    // 5️⃣ Cleanup (chỉ khi server dừng)
    mongo_cleanup(g_mongo_ctx);
    log_info("TCP Server stopped.");

    return 0;
}