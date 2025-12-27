#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/socket.h>

#define MAX_JWT_LEN 512
#define MESSAGE_SIZE 5520  // 4 (type) + 512 (token) + 5004 (payload union)
#define MAX_PAYLOAD_SIZE 5004

typedef enum {
    MSG_REGISTER = 1,
    MSG_LOGIN = 2,
    MSG_AUTH_SUCCESS = 3,
    MSG_AUTH_FAILED = 4,
    MSG_JOIN_QUEUE = 5,
    MSG_LEAVE_QUEUE = 6,
    MSG_START_GAME = 7,
    MSG_PLAYER_MOVE = 8,
    MSG_MOVE_RESULT = 9,
    MSG_GAME_OVER = 10,
    MSG_CHAT = 11,
    MSG_LOGOUT = 12,
    MSG_PING = 13,
    MSG_PONG = 14,
    MSG_PLACE_SHIP = 15,
    MSG_PLAYER_READY = 16,
    MSG_GET_ONLINE_PLAYERS = 17,
    MSG_ONLINE_PLAYERS_LIST = 18,
    MSG_CHALLENGE_PLAYER = 19,
    MSG_CHALLENGE_RECEIVED = 20,
    MSG_CHALLENGE_ACCEPT = 21,
    MSG_CHALLENGE_DECLINE = 22,
    MSG_CHALLENGE_DECLINED = 23,
    MSG_CHALLENGE_EXPIRED = 24,
    MSG_CHALLENGE_CANCEL = 25,
    MSG_CHALLENGE_CANCELLED = 26,
    MSG_AUTH_TOKEN = 27,
    MSG_TURN_WARNING = 28,
    MSG_GAME_TIMEOUT = 29,
    MSG_CHAT_MESSAGE = 30
} msg_type_t;

typedef struct __attribute__((packed)) {
    int32_t type;                 // msg_type (4 bytes)
    char token[MAX_JWT_LEN];      // Token xác thực (512 bytes)
    uint8_t payload[MAX_PAYLOAD_SIZE]; // Union payload (dạng byte thô)
} tcp_message_t;

// TCP Client API
int tcp_connect(const char *host, uint16_t port);
int tcp_disconnect(int sockfd);
int tcp_send(int sockfd, tcp_message_t *msg);
int tcp_recv(int sockfd, tcp_message_t *msg);
int tcp_set_nonblocking(int sockfd);

// Helper functions
void tcp_message_init(tcp_message_t *msg, int32_t type);
void tcp_message_set_payload(tcp_message_t *msg, const void *data, uint16_t len);

#endif // TCP_CLIENT_H