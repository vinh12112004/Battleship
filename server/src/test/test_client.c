#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include "network/ws_protocol.h"
char current_token[MAX_JWT_LEN] = "";
void print_menu() {
    printf("\n========== TEST CLIENT MENU ==========\n");
    printf("1. MSG_REGISTER - Đăng ký tài khoản\n");
    printf("2. MSG_LOGIN - Đăng nhập\n");
    printf("3. MSG_JOIN_QUEUE - Tham gia hàng đợi\n");
    printf("4. MSG_LEAVE_QUEUE - Rời hàng đợi\n");
    printf("5. MSG_PLAYER_MOVE - Thực hiện nước đi\n");
    printf("6. MSG_CHAT - Gửi tin nhắn chat\n");
    printf("7. MSG_LOGOUT - Đăng xuất\n");
    printf("0. Thoát\n");
    printf("======================================\n");
    printf("Chọn loại message: ");
}

void send_register(SOCKET sock) {
    message_t msg;
    msg.type = MSG_REGISTER;
    
    printf("Nhập username: ");
    scanf("%31s", msg.payload.auth.username);
    printf("Nhập password: ");
    scanf("%31s", msg.payload.auth.password);
    
    printf("Đang gửi MSG_REGISTER...\n");
    send_message(sock, &msg);
    
    // Nhận phản hồi
    message_t resp;
    if (recv_message(sock, &resp) > 0) {
        if (resp.type == MSG_AUTH_SUCCESS) {
            printf("✓ Đăng ký thành công!\n");
            printf("  Token: %s\n", resp.payload.auth_suc.token);
            printf("  Username: %s\n", resp.payload.auth_suc.username);
        } else if (resp.type == MSG_AUTH_FAILED) {
            printf("✗ Đăng ký thất bại: %s\n", resp.payload.auth_fail.reason);
        }
    }
}

void send_login(SOCKET sock) {
    message_t msg;
    msg.type = MSG_LOGIN;
    
    printf("Nhập username: ");
    scanf("%31s", msg.payload.auth.username);
    printf("Nhập password: ");
    scanf("%31s", msg.payload.auth.password);
    
    printf("Đang gửi MSG_LOGIN...\n");
    send_message(sock, &msg);
    
    // Nhận phản hồi
    message_t resp;
    if (recv_message(sock, &resp) > 0) {
        if (resp.type == MSG_AUTH_SUCCESS) {
            printf("✓ Đăng nhập thành công!\n");
            printf("  Token: %s\n", resp.payload.auth_suc.token);
            printf("  Username: %s\n", resp.payload.auth_suc.username);
            strncpy(current_token, resp.payload.auth_suc.token, MAX_JWT_LEN - 1);
            current_token[MAX_JWT_LEN - 1] = '\0';
        } else if (resp.type == MSG_AUTH_FAILED) {
            printf("✗ Đăng nhập thất bại: %s\n", resp.payload.auth_fail.reason);
        }
    }
}

void send_logout(SOCKET sock) {
    if (strlen(current_token) == 0) {
        printf("✗ Bạn chưa đăng nhập!\n");
        return;
    }

    message_t msg;
    msg.type = MSG_LOGOUT;
    strncpy(msg.payload.logout.token, current_token, MAX_JWT_LEN - 1);
    msg.payload.logout.token[MAX_JWT_LEN - 1] = '\0';

    printf("Đang gửi MSG_LOGOUT...\n");
    send_message(sock, &msg);

    message_t resp;
    if (recv_message(sock, &resp) > 0) {
        if (resp.type == MSG_AUTH_SUCCESS) {
            printf("✓ Đăng xuất thành công!\n");
            current_token[0] = '\0'; // clear token
        } else if (resp.type == MSG_AUTH_FAILED) {
            printf("✗ Đăng xuất thất bại: %s\n", resp.payload.auth_fail.reason);
        }
    }
}


void send_join_queue(SOCKET sock) {
    message_t msg;
    msg.type = MSG_JOIN_QUEUE;
    
    printf("Đang gửi MSG_JOIN_QUEUE...\n");
    send_message(sock, &msg);
    
    printf("✓ Đã gửi yêu cầu tham gia hàng đợi\n");
    
    // Chờ phản hồi START_GAME
    message_t resp;
    printf("Đang chờ tìm đối thủ...\n");
    if (recv_message(sock, &resp) > 0) {
        if (resp.type == MSG_START_GAME) {
            printf("✓ Tìm thấy trận đấu!\n");
            printf("  Đối thủ: %s\n", resp.payload.start_game.opponent);
        }
    }
}

void send_leave_queue(SOCKET sock) {
    message_t msg;
    msg.type = MSG_LEAVE_QUEUE;
    
    printf("Đang gửi MSG_LEAVE_QUEUE...\n");
    send_message(sock, &msg);
    
    printf("✓ Đã rời khỏi hàng đợi\n");
}

void send_player_move(SOCKET sock) {
    message_t msg;
    msg.type = MSG_PLAYER_MOVE;
    
    printf("Nhập tọa độ X (0-9): ");
    scanf("%d", &msg.payload.move.x);
    printf("Nhập tọa độ Y (0-9): ");
    scanf("%d", &msg.payload.move.y);
    
    printf("Đang gửi nước đi (%d, %d)...\n", msg.payload.move.x, msg.payload.move.y);
    send_message(sock, &msg);
    
    // Nhận kết quả
    message_t resp;
    if (recv_message(sock, &resp) > 0) {
        if (resp.type == MSG_MOVE_RESULT) {
            printf("✓ Kết quả: ");
            if (resp.payload.move_res.hit) {
                printf("TRÚNG! ");
                if (strlen(resp.payload.move_res.sunk) > 0) {
                    printf("Đã đánh chìm %s", resp.payload.move_res.sunk);
                }
            } else {
                printf("TRƯỢT!");
            }
            printf("\n");
        } else if (resp.type == MSG_GAME_OVER) {
            printf("✓ Trò chơi kết thúc!\n");
        }
    }
}

void send_chat(SOCKET sock) {
    message_t msg;
    msg.type = MSG_CHAT;
    
    printf("Nhập tin nhắn: ");
    getchar(); // Clear buffer
    fgets(msg.payload.chat.message, 128, stdin);
    msg.payload.chat.message[strcspn(msg.payload.chat.message, "\n")] = 0; // Remove newline
    
    printf("Đang gửi tin nhắn...\n");
    send_message(sock, &msg);
    
    printf("✓ Tin nhắn đã được gửi\n");
}

int main() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        printf("Socket creation failed\n");
        WSACleanup();
        return 1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    printf("Đang kết nối đến server 127.0.0.1:8080...\n");
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Không thể kết nối đến server!\n");
        closesocket(sock);
        WSACleanup();
        return 1;
    }
    printf("✓ Đã kết nối thành công!\n");

    int choice;
    while (1) {
        print_menu();
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                send_register(sock);
                break;
            case 2:
                send_login(sock);
                break;
            case 3:
                send_join_queue(sock);
                break;
            case 4:
                send_leave_queue(sock);
                break;
            case 5:
                send_player_move(sock);
                break;
            case 6:
                send_chat(sock);
                break;
            case 7:
                send_logout(sock);
                break;
            case 0:
                printf("Đang thoát...\n");
                closesocket(sock);
                WSACleanup();
                return 0;
            default:
                printf("Lựa chọn không hợp lệ!\n");
                break;
        }
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}