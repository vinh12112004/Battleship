#include "auth/jwt.h"
#include "config.h"
#include "utils/logger.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <cjson/cJSON.h>
#include <stdbool.h>

// ===== Base64 URL Encode =====
static char* base64url_encode(const unsigned char *input, size_t len) {
    BIO *b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO *bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);
    BIO_write(b64, input, len);
    BIO_flush(b64);

    BUF_MEM *bptr;
    BIO_get_mem_ptr(b64, &bptr);

    char *buff = malloc(bptr->length + 1);
    memcpy(buff, bptr->data, bptr->length);
    buff[bptr->length] = '\0';

    // URL safe
    for (size_t i = 0; i < bptr->length; i++) {
        if (buff[i] == '+') buff[i] = '-';
        else if (buff[i] == '/') buff[i] = '_';
    }
    // remove padding
    size_t len_url = bptr->length;
    while (len_url > 0 && buff[len_url - 1] == '=') len_url--;
    buff[len_url] = '\0';

    BIO_free_all(b64);
    return buff;
}

// ===== Base64 URL Decode =====
static unsigned char* base64url_decode(const char *input, size_t *out_len) {
    size_t len = strlen(input);
    size_t padded_len = len + (4 - len % 4) % 4;
    char *tmp = malloc(padded_len + 1);
    strcpy(tmp, input);
    for (size_t i = 0; i < len; i++) {
        if (tmp[i] == '-') tmp[i] = '+';
        else if (tmp[i] == '_') tmp[i] = '/';
    }
    for (size_t i = len; i < padded_len; i++) tmp[i] = '=';
    tmp[padded_len] = '\0';

    BIO *b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO *bmem = BIO_new_mem_buf(tmp, padded_len);
    bmem = BIO_push(b64, bmem);

    unsigned char *out = malloc(padded_len);
    int decoded_len = BIO_read(bmem, out, padded_len);
    if (decoded_len < 0) { free(out); out = NULL; }
    else *out_len = decoded_len;

    BIO_free_all(bmem);
    free(tmp);
    return out;
}

// ===== JWT Generate =====
char* jwt_generate(const char *user_id) {
    if (!user_id) return NULL;
    time_t now = time(NULL);
    time_t exp = now + get_jwt_expiry();

    const char *header = "{\"alg\":\"HS256\",\"typ\":\"JWT\"}";
    char payload[512];
    snprintf(payload, sizeof(payload), "{\"user_id\":\"%s\",\"iat\":%ld,\"exp\":%ld}", user_id, (long)now, (long)exp);

    char *enc_header = base64url_encode((unsigned char*)header, strlen(header));
    char *enc_payload = base64url_encode((unsigned char*)payload, strlen(payload));

    size_t sig_input_len = strlen(enc_header) + 1 + strlen(enc_payload);
    char *sig_input = malloc(sig_input_len + 1);
    sprintf(sig_input, "%s.%s", enc_header, enc_payload);

    unsigned char sig[EVP_MAX_MD_SIZE];
    unsigned int sig_len;
    HMAC(EVP_sha256(), get_jwt_secret(), strlen(get_jwt_secret()),
         (unsigned char*)sig_input, strlen(sig_input),
         sig, &sig_len);
    free(sig_input);

    char *enc_sig = base64url_encode(sig, sig_len);

    size_t token_len = strlen(enc_header) + 1 + strlen(enc_payload) + 1 + strlen(enc_sig) + 1;
    char *token = malloc(token_len);
    sprintf(token, "%s.%s.%s", enc_header, enc_payload, enc_sig);

    free(enc_header); free(enc_payload); free(enc_sig);
    log_info("Generated JWT for user: %s", user_id);
    return token;
}

// ===== JWT Verify =====
char* jwt_verify(const char *token) {
    if (!token) return NULL;
    char *tok_copy = _strdup(token);

    char *header = strtok(tok_copy, ".");
    char *payload = strtok(NULL, ".");
    char *sig = strtok(NULL, ".");
    if (!header || !payload || !sig) { free(tok_copy); return NULL; }

    size_t sig_input_len = strlen(header) + 1 + strlen(payload);
    char *sig_input = malloc(sig_input_len + 1);
    sprintf(sig_input, "%s.%s", header, payload);

    unsigned char expected[EVP_MAX_MD_SIZE];
    unsigned int expected_len;
    HMAC(EVP_sha256(), get_jwt_secret(), strlen(get_jwt_secret()),
         (unsigned char*)sig_input, strlen(sig_input),
         expected, &expected_len);
    free(sig_input);

    size_t sig_dec_len;
    unsigned char *sig_dec = base64url_decode(sig, &sig_dec_len);
    if (!sig_dec || sig_dec_len != expected_len || memcmp(sig_dec, expected, expected_len) != 0) {
        free(sig_dec); free(tok_copy); return NULL;
    }
    free(sig_dec);

    size_t payload_dec_len;
    unsigned char *payload_dec = base64url_decode(payload, &payload_dec_len);
    if (!payload_dec) { free(tok_copy); return NULL; }
    payload_dec[payload_dec_len] = '\0';

    cJSON *json = cJSON_Parse((char*)payload_dec);
    free(payload_dec); free(tok_copy);
    if (!json) return NULL;

    cJSON *user_id_json = cJSON_GetObjectItem(json, "user_id");
    if (!user_id_json || !cJSON_IsString(user_id_json)) { cJSON_Delete(json); return NULL; }

    char *user_id = _strdup(user_id_json->valuestring);
    cJSON_Delete(json);
    return user_id;
}
