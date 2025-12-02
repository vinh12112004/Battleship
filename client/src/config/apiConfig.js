/**
 * API Configuration
 * Message types matching C server protocol (ws_protocol.h)
 */

// WebSocket server URL
export const WS_URL = "ws://localhost:9090";

// Message types (must match server's message_type enum)
export const MESSAGE_TYPES = {
    // Auth messages
    MSG_REGISTER: 1,
    MSG_LOGIN: 2,
    MSG_AUTH_SUCCESS: 3,
    MSG_AUTH_FAILED: 4,

    // Matchmaking messages
    MSG_JOIN_QUEUE: 5,
    MSG_LEAVE_QUEUE: 6,
    MSG_MATCH_FOUND: 7,

    // Game messages
    MSG_GAME_STARTED: 8,
    MSG_PLACE_SHIP: 9,
    MSG_FIRE: 10,
    MSG_HIT: 11,
    MSG_MISS: 12,
    MSG_SHIP_SUNK: 13,
    MSG_GAME_OVER: 14,
    MSG_OPPONENT_DISCONNECTED: 15,

    // Chat messages
    MSG_CHAT: 16,

    // Ping/Pong
    MSG_PING: 17,
    MSG_PONG: 18,
};

// Game constants
export const BOARD_SIZE = 10;
export const SHIP_LENGTHS = [5, 4, 3, 3, 2]; // Carrier, Battleship, Cruiser, Submarine, Destroyer
