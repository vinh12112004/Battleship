#ifndef JWT_H
#define JWT_H

#include <stdbool.h>
#include <time.h>

char* jwt_generate(const char *user_id);
char* jwt_verify(const char *token);
bool jwt_is_expired(const char *token);

#endif // JWT_H