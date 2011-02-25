#include "chat.h"

#include "contact.h"
#include "sipcnet.h"
#include "sipevent.h"
#include "user.h"

#include <stdlib.h>
#include <string.h>

struct __chat_t {
    sipc_net_t* sipcnet;
    user_t* user;
    contact_list_t* contacts;
};

chat_t* chat_create()
{
    chat_t* chat = malloc(sizeof(chat_t));
    memset(chat, 0, sizeof(chat_t));
    return chat;
}

void chat_destroy(chat_t* chat)
{
    free(chat);
}

void chat_set_sipcnet(chat_t* chat, sipc_net_t* sipc)
{
    chat->sipcnet = sipc;
}

sipc_net_t* chat_get_sipcnet(const chat_t* chat)
{
    return chat->sipcnet;
}

void chat_set_user(chat_t* chat, user_t* user)
{
    chat->user = user;
}

user_t* chat_get_user(const chat_t* chat)
{
    return chat->user;
}

void chat_invite_contact(chat_t* chat, const char* buddyid)
{
}

void chat_send_message(chat_t* chat, const char* content)
{
}

void chat_send_nudge(chat_t* chat)
{
    sipevent_t* ev = sipevent_create(SIP_INFO, "fetion.com.cn SIP-C/4.0", user_get_sid(chat->user));
    char body[] = "<is-composing><state>nudge</state></is-composing>";
    sipevent_set_content(ev, body);
    sipc_net_send_sipevent(chat->sipcnet, ev);
    sipevent_destroy(ev);
}


