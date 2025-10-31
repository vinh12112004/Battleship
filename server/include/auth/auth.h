#ifndef AUTH_H
#define AUTH_H

#include <stdbool.h>
#include "database/mongo_user.h"

typedef struct {
    bool success;
    char *token;
    user_t *user;
    char *error_message;
} auth_result_t;

auth_result_t* auth_register(const char *username, const char *email, const char *password);
auth_result_t* auth_login(const char *username, const char *password);
bool auth_logout(const char *token);
user_t* auth_get_current_user(const char *token);
void auth_result_free(auth_result_t *result);

#endif // AUTH_H