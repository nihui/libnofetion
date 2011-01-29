#ifndef HTTPPACKET_H
#define HTTPPACKET_H

typedef enum {
    GET         = 0,
    POST        = 1
} http_type_t;

typedef struct __http_packet_t http_packet_t;

http_packet_t* http_packet_create(http_type_t type, const char* subUri);
void http_packet_destroy(http_packet_t* packet);
void http_packet_add_header(http_packet_t* packet, const char* key, const char* value);
const char* http_packet_get_header_value(const http_packet_t* packet, const char* key);
void http_packet_set_content(http_packet_t* packet, const char* content);
const char* http_packet_get_content(const http_packet_t* packet);
char* http_packet_to_string(const http_packet_t* packet, int* buf_len);
http_packet_t* http_packet_from_string(const char* buf);

#endif // HTTPPACKET_H
