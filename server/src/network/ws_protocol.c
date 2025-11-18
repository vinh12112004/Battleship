#include "network/ws_protocol.h"
#include "utils/logger.h"
#include <string.h>
#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <openssl/sha.h>

// Base64 encoding for WebSocket key
static const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static void base64_encode(const unsigned char *in, size_t len, char *out) {
    int i = 0, j = 0;
    unsigned char char_array_3[3], char_array_4[4];

    while (len--) {
        char_array_3[i++] = *(in++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for(i = 0; i < 4; i++) out[j++] = base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for(int k = i; k < 3; k++) char_array_3[k] = '\0';
        
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

        for(int k = 0; k < i + 1; k++) out[j++] = base64_chars[char_array_4[k]];
        while(i++ < 3) out[j++] = '=';
    }
    out[j] = '\0';
}

// WebSocket handshake
int ws_handshake(SOCKET sock) {
    char buffer[2048] = {0};
    int n = recv(sock, buffer, sizeof(buffer) - 1, 0);
    
    if (n <= 0) {
        log_error("Failed to receive handshake request");
        return -1;
    }
    
    buffer[n] = '\0';
    log_debug("Received handshake:\n%s", buffer);
    
    // Parse Sec-WebSocket-Key
    char *key_start = strstr(buffer, "Sec-WebSocket-Key: ");
    if (!key_start) {
        log_error("No Sec-WebSocket-Key found in handshake");
        return -1;
    }
    
    key_start += 19; // Length of "Sec-WebSocket-Key: "
    char *key_end = strstr(key_start, "\r\n");
    if (!key_end) {
        log_error("Invalid Sec-WebSocket-Key format");
        return -1;
    }
    
    size_t key_len = key_end - key_start;
    char client_key[256] = {0};
    strncpy(client_key, key_start, key_len);
    client_key[key_len] = '\0';
    
    log_debug("Client key: %s", client_key);
    
    // Append magic string
    const char *magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    char accept_key[512];
    snprintf(accept_key, sizeof(accept_key), "%s%s", client_key, magic);
    
    log_debug("Accept key (before hash): %s", accept_key);
    
    // ⭐ Calculate SHA-1 using OpenSSL
    unsigned char hash[SHA_DIGEST_LENGTH]; // 20 bytes
    SHA1((unsigned char*)accept_key, strlen(accept_key), hash);
    
    // Base64 encode
    char accept_base64[64];
    base64_encode(hash, SHA_DIGEST_LENGTH, accept_base64);
    
    log_debug("Sec-WebSocket-Accept: %s", accept_base64);
    
    // Send handshake response
    char response[1024];
    snprintf(response, sizeof(response),
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: %s\r\n"
        "\r\n",
        accept_base64);
    
    int sent = send(sock, response, strlen(response), 0);
    if (sent <= 0) {
        log_error("Failed to send handshake response");
        return -1;
    }
    
    log_info("WebSocket handshake completed successfully");
    return 0;
}

// Send WebSocket frame
int ws_send_frame(SOCKET sock, uint8_t opcode, const char *payload, size_t len) {
    uint8_t header[10];
    size_t header_len = 2;
    
    // FIN bit + opcode
    header[0] = 0x80 | (opcode & 0x0F);
    
    // Payload length
    if (len < 126) {
        header[1] = len;
    } else if (len < 65536) {
        header[1] = 126;
        header[2] = (len >> 8) & 0xFF;
        header[3] = len & 0xFF;
        header_len = 4;
    } else {
        header[1] = 127;
        for (int i = 0; i < 8; i++) {
            header[9 - i] = (len >> (i * 8)) & 0xFF;
        }
        header_len = 10;
    }
    
    // Send header
    if (send(sock, (char*)header, header_len, 0) != header_len) {
        log_error("Failed to send frame header");
        return -1;
    }
    
    // Send payload
    if (len > 0 && send(sock, payload, len, 0) != len) {
        log_error("Failed to send frame payload");
        return -1;
    }
    
    return 0;
}

// Receive WebSocket frame
int ws_recv_frame(SOCKET sock, ws_frame_t *frame, char **payload) {
    uint8_t header[2];
    
    // Read first 2 bytes
    log_debug("Attempting to read WebSocket frame header...");
    int bytes_received = recv(sock, (char*)header, 2, 0);
    
    if (bytes_received != 2) {
        if (bytes_received == 0) {
            log_info("Client đóng kết nối một cách bình thường");
        } else if (bytes_received < 0) {
            int error = WSAGetLastError();
            log_error("recv() lỗi: %d (WSAError: %d)", bytes_received, error);
        } else {
            log_error("Chỉ nhận được %d/2 bytes header", bytes_received);
        }
        return -1;
    }
    
    frame->fin = (header[0] & 0x80) >> 7;
    frame->opcode = header[0] & 0x0F;
    frame->mask = (header[1] & 0x80) >> 7;
    frame->payload_len = header[1] & 0x7F;
    
    log_debug("Frame: fin=%d, opcode=0x%X, mask=%d, len=%llu", 
            frame->fin, frame->opcode, frame->mask, frame->payload_len);

    // Extended payload length
    if (frame->payload_len == 126) {
        uint8_t len_bytes[2];
        if (recv(sock, (char*)len_bytes, 2, 0) != 2) return -1;
        frame->payload_len = (len_bytes[0] << 8) | len_bytes[1];
    } else if (frame->payload_len == 127) {
        uint8_t len_bytes[8];
        if (recv(sock, (char*)len_bytes, 8, 0) != 8) return -1;
        frame->payload_len = 0;
        for (int i = 0; i < 8; i++) {
            frame->payload_len = (frame->payload_len << 8) | len_bytes[i];
        }
    }
    
    // Masking key
    if (frame->mask) {
        if (recv(sock, (char*)frame->masking_key, 4, 0) != 4) return -1;
    }
    
    // Read payload
    if (frame->payload_len > 0) {
        *payload = (char*)malloc(frame->payload_len + 1);
        if (!*payload) return -1;
        
        size_t received = 0;
        while (received < frame->payload_len) {
            int n = recv(sock, *payload + received, frame->payload_len - received, 0);
            if (n <= 0) {
                free(*payload);
                return -1;
            }
            received += n;
        }
        
        // Unmask if needed
        if (frame->mask) {
            for (size_t i = 0; i < frame->payload_len; i++) {
                (*payload)[i] ^= frame->masking_key[i % 4];
            }
        }
        
        (*payload)[frame->payload_len] = '\0';
    } else {
        *payload = NULL;
    }
    
    return 0;
}

// Send message as WebSocket binary frame
ssize_t ws_send_message(SOCKET sock, message_t *msg) {
    log_debug("Sending WebSocket message type=%d", msg->type);
    
    // Convert integers to network byte order
    message_t tmp;
    memcpy(&tmp, msg, sizeof(message_t));
    
    if (msg->type == MSG_PLAYER_MOVE || msg->type == MSG_MOVE_RESULT) {
        tmp.payload.move.x = htonl(msg->payload.move.x);
        tmp.payload.move.y = htonl(msg->payload.move.y);
        if (msg->type == MSG_MOVE_RESULT) {
            tmp.payload.move_res.hit = htonl(msg->payload.move_res.hit);
        }
    }
    
    if (ws_send_frame(sock, WS_OPCODE_BINARY, (char*)&tmp, sizeof(message_t)) < 0) {
        log_error("Failed to send WebSocket message");
        return -1;
    }
    
    log_debug("WebSocket message sent successfully");
    return sizeof(message_t);
}

// Receive message from WebSocket binary frame
ssize_t ws_recv_message(SOCKET sock, message_t *msg) {
    ws_frame_t frame;
    char *payload = NULL;
    
    log_debug("Receiving WebSocket message...");
    
    if (ws_recv_frame(sock, &frame, &payload) < 0) {
        log_error("Failed to receive WebSocket frame");
        return -1;
    }
    
    // Handle control frames
    if (frame.opcode == WS_OPCODE_CLOSE) {
        log_info("Received close frame");
        if (payload) free(payload);
        return 0;
    }
    
    if (frame.opcode == WS_OPCODE_PING) {
        log_debug("Received ping, sending pong");
        ws_send_frame(sock, WS_OPCODE_PONG, payload, frame.payload_len);
        if (payload) free(payload);
        return ws_recv_message(sock, msg); // Try again
    }
    
    if (frame.opcode != WS_OPCODE_BINARY) {
        log_warn("Unexpected opcode: %d", frame.opcode);
        if (payload) free(payload);
        return -1;
    }
    
    if (frame.payload_len != sizeof(message_t)) {
        log_error("Invalid message size: %llu (expected %zu)", frame.payload_len, sizeof(message_t));
        if (payload) free(payload);
        return -1;
    }
    
    memcpy(msg, payload, sizeof(message_t));
    free(payload);
    
    // Convert from network byte order
    if (msg->type == MSG_PLAYER_MOVE || msg->type == MSG_MOVE_RESULT) {
        msg->payload.move.x = ntohl(msg->payload.move.x);
        msg->payload.move.y = ntohl(msg->payload.move.y);
        if (msg->type == MSG_MOVE_RESULT) {
            msg->payload.move_res.hit = ntohl(msg->payload.move_res.hit);
        }
    }
    
    log_debug("WebSocket message received: type=%d", msg->type);
    return sizeof(message_t);
}

// Close WebSocket connection
void ws_close(SOCKET sock, uint16_t code) {
    uint8_t payload[2];
    payload[0] = (code >> 8) & 0xFF;
    payload[1] = code & 0xFF;
    
    ws_send_frame(sock, WS_OPCODE_CLOSE, (char*)payload, 2);
    log_info("WebSocket close frame sent");
}