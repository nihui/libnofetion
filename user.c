#include "user.h"

#include "sipevent.h"

#include <stdlib.h>
#include <string.h>

struct __user_t {
    char* no;
    int id;
    char* uri;
    int sid;
    sipc_net_t* sipc_net;
};

user_t* user_create(const char* no)
{
    user_t* user = malloc(sizeof(user_t));
    memset(user, 0, sizeof(user_t));
    user->no = strdup(no);
    return user;
}

void user_destroy(user_t* user)
{
    free(user->no);
    free(user->uri);
    free(user);
}

const char* user_get_mobileno(const user_t* user)
{
    return user->no;
}

void user_set_id(user_t* user, int id)
{
    user->id = id;
}

int user_get_id(const user_t* user)
{
    return user->id;
}

void user_set_uri(user_t* user, const char* uri)
{
    free(user->uri);
    user->uri = strdup(uri);
    char* sid_begin = strstr(uri, ":") + 1;
    char* sid_end = strstr(uri, "@");
    char* sid_str = strndup(sid_begin, sid_end - sid_begin);
    user->sid = atoi(sid_str);
    free(sid_str);
}

const char* user_get_uri(const user_t* user)
{
    return user->uri;
}

int user_get_sid(const user_t* user)
{
    return user->sid;
}

void user_set_sipcnet(user_t* user, sipc_net_t* sipc)
{
    user->sipc_net = sipc;
}

sipc_net_t* user_get_sipcnet(const user_t* user)
{
    return user->sipc_net;
}

void user_set_moodphrase(user_t* user, const char* moodphrase)
{
}
