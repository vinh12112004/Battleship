#ifndef PASSWORD_H
#define PASSWORD_H

#include <stdbool.h>

#define PASSWORD_HASH_LENGTH 61

char* password_hash(const char *password);
bool password_verify(const char *password, const char *hash);
bool password_validate(const char *password, char *error_msg, size_t error_len);

#endif // PASSWORD_H