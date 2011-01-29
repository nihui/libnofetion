#ifndef SIPCNET_H
#define SIPCNET_H

#include "sipevent.h"
#include "session.h"

typedef struct __sipc_net_t sipc_net_t;
typedef int (*sipevent_handler_t)(nofetion_session_t*, const sipevent_t*);

void set_reply_callback(sipc_net_t* net, nofetion_session_t* session, int callid, sipevent_handler_t cb_func);
// void invoke_reply_callback(sipc_net_t* net, sipevent_t* sipevent);

sipc_net_t* sipc_net_create();
int sipc_net_connect(sipc_net_t* net, const char* host, const char* port);
int sipc_net_connect2(sipc_net_t* net, const char* hostport);
void sipc_net_destroy(sipc_net_t* net);
void sipc_net_send_sipevent(sipc_net_t* net, sipevent_t* sipevent);
void sipc_net_register_sipevent_handler(sipc_net_t* net, nofetion_session_t* session, sipevent_handler_t handler);
void sipc_net_deregister_sipevent_handler(sipc_net_t* net, sipevent_handler_t handler);
void sipc_net_thread_loop(sipc_net_t* net);
void sipc_net_thread_quit(sipc_net_t* net);

#endif // SIPCNET_H
