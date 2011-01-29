#include "sipevent.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>

#include "session.h"

typedef struct __sipheader_t {
    char* key;
    char* value;
    struct __sipheader_t* next_header;
} sipheader_t;

struct __sipevent_t {
    sipevent_type_t type;
    char* addition;
    int from;
    int callid;
    int sequence;
    sipheader_t* headers;
    char* content;
};

static volatile int callid = 0;
static const int sequence = 2;

sipevent_t* sipevent_create(sipevent_type_t type, const char* addition, int from)
{
    sipevent_t* sipevent = malloc(sizeof(sipevent_t));
    sipevent->type = type;
    sipevent->addition = strdup(addition);
    sipevent->from = from;
    sipevent->callid = ++callid;
    sipevent->sequence = sequence;
    sipevent->headers = NULL;
    sipevent->content = NULL;
    return sipevent;
}

void sipevent_destroy(sipevent_t* sipevent)
{
    free(sipevent->addition);
    sipheader_t* header = sipevent->headers;
    sipheader_t* header_next = NULL;
    while (header) {
        header_next = header->next_header;
        free(header->key);
        free(header->value);
        free(header);
        header = header_next;
    }
    free(sipevent->content);
    free(sipevent);
}

void sipevent_add_header(sipevent_t* sipevent, const char* key, const char* value)
{
    sipheader_t* header = malloc(sizeof(sipheader_t));
    header->key = strdup(key);
    header->value = strdup(value);
    header->next_header = NULL;

    sipheader_t* header_tbp = sipevent->headers;
    if (!header_tbp) {
        sipevent->headers = header;
        return;
    }
    // get the header node to be appended to
    while (header_tbp->next_header)
        header_tbp = header_tbp->next_header;
    header_tbp->next_header = header;
}

const char* sipevent_get_header_value(const sipevent_t* sipevent, const char* key)
{
    sipheader_t* header = sipevent->headers;
    while (header) {
        if (strcmp(header->key, key) == 0)
            return header->value;
        header = header->next_header;
    }
    return NULL;
}

sipevent_type_t sipevent_get_type(const sipevent_t* sipevent)
{
    return sipevent->type;
}

const char* sipevent_get_type_addition(const sipevent_t* sipevent)
{
    return sipevent->addition;
}

int sipevent_get_callid(const sipevent_t* sipevent)
{
    return sipevent->callid;
}

void sipevent_set_content(sipevent_t* sipevent, const char* content)
{
    free(sipevent->content);
    sipevent->content = strdup(content);
}

const char* sipevent_get_content(const sipevent_t* sipevent)
{
    return sipevent->content;
}

static const char* sipevent_type_to_string(sipevent_type_t type)
{
    static const char* sipevent_type_string[] = {
        "",             // SipUnknown
        "A",            // SipAcknowledge
        "BN",           // SipBENotify
        "B",            // SipBye
        "",             // SipCancel TODO
        "IN",           // SipInfo
        "I",            // SipInvite
        "M",            // SipMessage
        "",             // SipNegotiate TODO
        "NB",           // SipNotify
        "O",            // SipOption
        "",             // SipRefer TODO
        "R",            // SipRegister
        "S",            // SipService
        "SUB",          // SipSubscribe
        "SIP-C/4.0",    // SipSipc_4_0
    };
    if (type > 15) {
        fprintf(stderr, "unexpected sip event type %d\n", (int)type);
        return sipevent_type_string[0];
    }
    return sipevent_type_string[(int)type];
}

static sipevent_type_t sipevent_string_to_type(const char* string)
{
    if (strcmp(string, "SIP-C/4.0") == 0)
        return SIP_SIPC_4_0;
    if (strcmp(string, "BN") == 0)
        return SIP_BENOTIFY;
    if (strcmp(string, "M") == 0)
        return SIP_MESSAGE;
    if (strcmp(string, "S") == 0)
        return SIP_SERVICE;
    if (strcmp(string, "I") == 0)
        return SIP_INVITE;
    if (strcmp(string, "IN") == 0)
        return SIP_INFO;
    if (strcmp(string, "O") == 0)
        return SIP_OPTION;
    if (strcmp(string, "SUB") == 0)
        return SIP_SUBSCRIBE;
    if (strcmp(string, "R") == 0)
        return SIP_REGISTER;
    if (strcmp(string, "NB") == 0)
        return SIP_NOTIFY;
    if (strcmp(string, "B") == 0)
        return SIP_BYE;
    if (strcmp(string, "A") == 0)
        return SIP_ACKNOWLEDGE;
    return SIP_UNKNOWN;
}

char* sipevent_to_string(const sipevent_t* sipevent, int* buf_len)
{
    const char* type_str = sipevent_type_to_string(sipevent->type);
    char from_str[16] = {'\0'};
    sprintf(from_str, "%d", sipevent->from);
    char callid_str[16] = {'\0'};
    sprintf(callid_str, "%d", sipevent->callid);
    char seq_str[16] = {'\0'};
    sprintf(seq_str, "%d", sipevent->sequence);
    char clen_str[16] = {'\0'};
    if (sipevent->content)
        sprintf(clen_str, "%d", strlen(sipevent->content));

    /// calculate the string length
    int len = 0;
    // "TYPESTR ADDITION\r\n"
    len += strlen(type_str) + 1 + strlen(sipevent->addition) + 2;
    // "F: FROM\r\n"
    len += 3 + strlen(from_str) + 2;
    // "I: CALLID\r\n"
    len += 3 + strlen(callid_str) + 2;
    // "Q: SEQUENCE TYPESTR\r\n"
    len += 3 + strlen(seq_str) + 1 + strlen(type_str) + 2;
    sipheader_t* header = sipevent->headers;
    while (header) {
        // "KEY: VALUE\r\n"
        len += strlen(header->key) + 2 + strlen(header->value) + 2;
        header = header->next_header;
    }
    if (sipevent->content) {
        // "L: CONTENT-LENGTH\r\n"
        len += 3 + strlen(clen_str) + 2;
    }
    // "\r\n"
    len += 2;
    if (sipevent->content) {
        // "CONTENT\r\n"
        len += strlen(sipevent->content) + 2;
    }

    /// construct the string
    char* buf = malloc(len + 1);
    buf[0] = '\0';
    // "TYPESTR ADDITION\r\n"
    strcat(buf, type_str);
    strcat(buf, " ");
    strcat(buf, sipevent->addition);
    strcat(buf, "\r\n");
    // "F: FROM\r\n"
    strcat(buf, "F: ");
    strcat(buf, from_str);
    strcat(buf, "\r\n");
    // "I: CALLID\r\n"
    strcat(buf, "I: ");
    strcat(buf, callid_str);
    strcat(buf, "\r\n");
    // "Q: SEQUENCE TYPESTR\r\n"
    strcat(buf, "Q: ");
    strcat(buf, seq_str);
    strcat(buf, " ");
    strcat(buf, type_str);
    strcat(buf, "\r\n");
    header = sipevent->headers;
    while (header) {
        // "KEY: VALUE\r\n"
        strcat(buf, header->key);
        strcat(buf, ": ");
        strcat(buf, header->value);
        strcat(buf, "\r\n");
        header = header->next_header;
    }
    if (sipevent->content) {
        // "L: CONTENT-LENGTH\r\n"
        strcat(buf, "L: ");
        strcat(buf, clen_str);
        strcat(buf, "\r\n");
    }
    // "\r\n"
    strcat(buf, "\r\n");
    if (sipevent->content) {
        // "CONTENT\r\n"
        strcat(buf, sipevent->content);
        strcat(buf, "\r\n");
    }
    *buf_len = len;
    return buf;
}

sipevent_t* sipevent_from_string(const char* buf)
{
    sipevent_t* sipevent = malloc(sizeof(sipevent_t));
    memset(sipevent, 0, sizeof(sipevent_t));

    /// extract type
    char* pos = strstr(buf, " ");
    char* pos_next = pos;
    char* type_str = strndup(buf, pos - buf);
    sipevent->type = sipevent_string_to_type(type_str);
    free(type_str);

    /// extract addition
    pos = pos_next + 1;
    pos_next = strstr(pos, "\r\n");
    sipevent->addition = strndup(pos,  pos_next - pos);

    /// extract key and value pairs
    pos = pos_next + 2;
    pos_next = strstr(pos, "\r\n");
    char* header_str = NULL;
    char* key_end = NULL;
    sipheader_t* header_tbp = sipevent->headers;
    while (pos != pos_next) {
        if (pos_next)
            header_str = strndup(pos, pos_next - pos);
        else
            header_str = strdup(pos);
        key_end = strstr(header_str, ": ");
        char* key = strndup(header_str, key_end - header_str);
        char* value = strndup(key_end + 2, pos_next - key_end - 2);
        free(header_str);
        if (strcmp(key, "F") == 0) {
            /// extract from F
            free(key);
            sipevent->from = atoi(value);
            free(value);
        }
        else if (strcmp(key, "I") == 0) {
            /// extract callid I
            free(key);
            sipevent->callid = atoi(value);
            free(value);
        }
        else if (strcmp(key, "Q") == 0) {
            /// extract sequence Q
            free(key);
            char* seq_end = strstr(value, " ");
            char* seq_str = strndup(value, seq_end - value);
            free(value);
            sipevent->sequence = atoi(seq_str);
            free(seq_str);
        }
        else {
            /// extract general key value pair
            sipheader_t* header = malloc(sizeof(sipheader_t));
            header->key = key;
            header->value = value;
            header->next_header = NULL;
            if (header_tbp) {
                header_tbp->next_header = header;
                header_tbp = header;
            }
            else {
                sipevent->headers = header;
                header_tbp = header;
            }
        }
        if (!pos_next)
            return sipevent;
        pos = pos_next + 2;
        pos_next = strstr(pos, "\r\n");
    }

    /// extract content
    char* content_begin = pos + 2;
    char* content_end = strstr(content_begin, "\r\n");
    if (content_end)
        sipevent->content = strndup(content_begin, content_end - content_begin);
    else
        sipevent->content = strdup(content_begin);
    return sipevent;
}

void sipevent_dump(FILE* out, const sipevent_t* sipevent)
{
    fprintf(out,"DUMP: %p ===============================\n",sipevent);
    fprintf(out,"TYPE: %d\n",sipevent->type);
    fprintf(out,"ADDI: %s\n",sipevent->addition);
    fprintf(out,"FROM: %d\n",sipevent->from);
    fprintf(out,"CALL: %d\n",sipevent->callid);
    fprintf(out,"SEQU: %d\n",sipevent->sequence);
    sipheader_t* header = sipevent->headers;
    while (header) {
        fprintf(out,"%s: %s\n", header->key, header->value);
        header = header->next_header;
    }
    fprintf(out,"CONTENT: %s\n",sipevent->content);
}

