#ifndef SIPEVENT_H
#define SIPEVENT_H

#include <stdio.h>

typedef enum {
    SIP_UNKNOWN                  = 0,
    SIP_ACKNOWLEDGE              = 1,
    SIP_BENOTIFY                 = 2,
    SIP_BYE                      = 3,
    SIP_CANCEL                   = 4,
    SIP_INFO                     = 5,
    SIP_INVITE                   = 6,
    SIP_MESSAGE                  = 7,
    SIP_NEGOTIATE                = 8,
    SIP_NOTIFY                   = 9,
    SIP_OPTION                   = 10,
    SIP_REFER                    = 11,
    SIP_REGISTER                 = 12,
    SIP_SERVICE                  = 13,
    SIP_SUBSCRIBE                = 14,
    SIP_SIPC_4_0                 = 15
} sipevent_type_t;

typedef struct __sipevent_t sipevent_t;

extern sipevent_t* sipevent_create(sipevent_type_t type, const char* addition, int from);
extern void sipevent_destroy(sipevent_t* sipevent);
extern void sipevent_add_header(sipevent_t* sipevent, const char* key, const char* value);
extern const char* sipevent_get_header_value(const sipevent_t* sipevent, const char* key);
extern sipevent_type_t sipevent_get_type(const sipevent_t* sipevent);
extern const char* sipevent_get_type_addition(const sipevent_t* sipevent);
extern int sipevent_get_callid(const sipevent_t* sipevent);
extern void sipevent_set_content(sipevent_t* sipevent, const char* content);
extern const char* sipevent_get_content(const sipevent_t* sipevent);
extern char* sipevent_to_string(const sipevent_t* sipevent, int* buf_len);
extern sipevent_t* sipevent_from_string(const char* buf);
extern void sipevent_dump(FILE* out, const sipevent_t* sipevent);

#endif // SIPEVENT_H
