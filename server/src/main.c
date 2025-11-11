#include <stdio.h>
#include "network/ws_server.h"
#include "utils/logger.h"
#include "database/mongo.h"
#include "config.h"

int main() {
    // 1️⃣ Log server start
    log_info("Starting Battleship Server...");

    // 2️⃣ Initialize MongoDB
    const char *mongo_uri = get_mongo_uri();
    const char *mongo_db = get_mongo_db();

    g_mongo_ctx = mongo_init(mongo_uri, mongo_db);
    if (!g_mongo_ctx) {
        log_error("Failed to connect to MongoDB");
        return 1;
    }

    log_info("MongoDB connected successfully.");

    // 3️⃣ Start WebSocket / TCP server
    uint16_t port = 8080;
    log_info("Starting WebSocket/TCP server on port %d...", port);
    start_ws_server(port);  // <- vòng lặp accept client bên trong

    // 4️⃣ Cleanup (chỉ khi server dừng)
    mongo_cleanup(g_mongo_ctx);
    log_info("Server stopped.");

    return 0;
}
