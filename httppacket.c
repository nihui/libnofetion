#include "httppacket.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct __http_header_t {
    char* key;
    char* value;
    struct __http_header_t* next_header;
} http_header_t;

struct __http_packet_t{
    char* type;
    char* subUri;
    http_header_t* headers;
    char* content;
};

http_packet_t* http_packet_create(http_type_t type, const char* subUri)
{
    char* type_str = NULL;
    if (type == GET)
        type_str = strdup("GET");
    else if (type == POST)
        type_str = strdup("POST");
    else {
        fprintf(stderr, "unknown http packet type\n");
        return NULL;
    }
    http_packet_t* packet = malloc(sizeof(http_packet_t));
    packet->type = type_str;
    packet->subUri = strdup(subUri);
    packet->headers = NULL;
    packet->content = NULL;
    return packet;
}

void http_packet_destroy(http_packet_t* packet)
{
    free(packet->type);
    free(packet->subUri);
    http_header_t* header = packet->headers;
    http_header_t* header_next = NULL;
    while (header) {
        header_next = header->next_header;
        free(header->key);
        free(header->value);
        free(header);
        header = header_next;
    }
    free(packet->content);
    free(packet);
}

void http_packet_add_header(http_packet_t* packet, const char* key, const char* value)
{
    http_header_t* header = malloc(sizeof(http_header_t));
    header->key = strdup(key);
    header->value = strdup(value);
    header->next_header = NULL;

    http_header_t* header_tbp = packet->headers;
    if (!header_tbp) {
        packet->headers = header;
        return;
    }
    // get the header node to be appended to
    while (header_tbp->next_header)
        header_tbp = header_tbp->next_header;
    header_tbp->next_header = header;
}

const char* http_packet_get_header_value(const http_packet_t* packet, const char* key)
{
    http_header_t* header = packet->headers;
    while (header) {
        if (strcmp(header->key, key) == 0)
            return header->value;
        header = header->next_header;
    }
    return NULL;
}

void http_packet_set_content(http_packet_t* packet, const char* content)
{
    free(packet->content);
    packet->content = strdup(content);
}

const char* http_packet_get_content(const http_packet_t* packet)
{
    return packet->content;
}

char* http_packet_to_string(const http_packet_t* packet, int* buf_len)
{
    char clen_str[16] = {'\0'};
    if (packet->content) {
        sprintf(clen_str, "%d", strlen(packet->content));
    }
    /// calculate the string length
    int len = 0;
    if (packet->type && packet->subUri) {
        // "TYPE SUBURI\r\n"
        len += strlen(packet->type) + 1 + strlen(packet->subUri) + 2;
    }
    http_header_t* header = packet->headers;
    while (header) {
        // "KEY: VALUE\r\n"
        len += strlen(header->key) + 2 + strlen(header->value) + 2;
        header = header->next_header;
    }
    if (packet->content) {
        // "Content-Length: CONTENT-LENGTH\r\n"
        len += 16 + strlen(clen_str) + 2;
    }
    // "\r\n"
    len += 2;
    if (packet->content) {
        // "CONTENT\r\n"
        len += strlen(packet->content) + 2;
    }

    /// construct the string
    char* buf = malloc(len + 1);
    buf[0] = '\0';
    if (packet->type && packet->subUri) {
        // "TYPE SUBURI\r\n"
        strcat(buf, packet->type);
        strcat(buf, " ");
        strcat(buf, packet->subUri);
        strcat(buf, "\r\n");
    }
    header = packet->headers;
    while (header) {
        // "KEY: VALUE\r\n"
        strcat(buf, header->key);
        strcat(buf, ": ");
        strcat(buf, header->value);
        strcat(buf, "\r\n");
        header = header->next_header;
    }
    if (packet->content) {
        // "Content-Length: CONTENT-LENGTH\r\n"
        strcat(buf, "Content-Length: ");
        strcat(buf, clen_str);
        strcat(buf, "\r\n");
    }
    // "\r\n"
    strcat(buf, "\r\n");
    if (packet->content) {
        // "CONTENT\r\n"
        strcat(buf, packet->content);
        strcat(buf, "\r\n");
    }
    *buf_len = len;
    return buf;
}

http_packet_t* http_packet_from_string(const char* buf)
{
    char* pos = strstr(buf, "HTTP/1.1 200 OK");
    if (pos != buf) {
        fprintf(stderr, "received an unsuccessful reply, ignore it: %s\n", buf);
        return NULL;
    }
    http_packet_t* packet = malloc(sizeof(http_packet_t));
    memset(packet, 0, sizeof(http_packet_t));

    /// extract header key and value pairs
    pos = strstr(buf, "\r\n") + 2;
    char* pos_next = strstr(pos, "\r\n");
    char* header_str = NULL;
    char* key_end = NULL;
    http_header_t* header_tbp = NULL;
    while (pos != pos_next) {
        if (pos_next)
            header_str = strndup(pos, pos_next - pos);
        else
            header_str = strdup(pos);
        key_end = strstr(header_str, ": ");
        http_header_t* header = malloc(sizeof(http_header_t));
        header->key = strndup(header_str, key_end - header_str);
        header->value = strndup(key_end + 2, pos_next - key_end - 2);
        header->next_header = NULL;
        if (header_tbp) {
            header_tbp->next_header = header;
            header_tbp = header;
        }
        else {
            packet->headers = header;
            header_tbp = header;
        }
        free(header_str);
        if (!pos_next)
            return packet;
        pos = pos_next + 2;
        pos_next = strstr(pos, "\r\n");
    }

    /// extract content
    char* content_begin = pos + 2;
    char* content_end = strstr(content_begin, "\r\n");
    if (content_end)
        packet->content = strndup(content_begin, content_end - content_begin);
    else
        packet->content = strdup(content_begin);
    return packet;
}

