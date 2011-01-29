#ifndef USER_H
#define USER_H

#include "sipcnet.h"

#include "status.h"

typedef enum {
    NO
} field_t;

typedef struct __user_t user_t;

user_t* user_create(const char* no);
void user_destroy(user_t* user);

const char* user_get_mobileno(const user_t* user);
void user_set_id(user_t* user, int id);
int user_get_id(const user_t* user);
void user_set_uri(user_t* user, const char* uri);
const char* user_get_uri(const user_t* user);
int user_get_sid(const user_t* user);
void user_set_sipcnet(user_t* user, sipc_net_t* sipc);
sipc_net_t* user_get_sipcnet(const user_t* user);

void user_retrieve_info(user_t* user);
void user_set_authority(user_t* user, field_t field, const char* text, int authority);
void user_set_status(user_t* user, status_t type);
void user_set_moodphrase(user_t* user, const char* moodphrase);
void user_send_sms(user_t* user, const char* text);
void user_add_buddy(user_t* user, const char* buddyno);
void user_delete_buddy(user_t* user, const char* buddyid);

#endif // USER_H
