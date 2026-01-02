#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <stdint.h>

// Public functions
int setup_tcp_server(uint16_t port);
void start_tcp_server(uint16_t port);
void client_register(int client_sock, const char *user_id);
int get_socket_by_user_id(const char *user_id);

#endif