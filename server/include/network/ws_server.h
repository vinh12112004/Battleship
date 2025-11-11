#ifndef WS_SERVER_H
#define WS_SERVER_H

#include <stdint.h>

// Khai báo hàm public
int setup_ws_server(uint16_t port);       // Khởi tạo server, trả socket
void start_ws_server(uint16_t port);      // Bắt đầu vòng lặp accept client

#endif
