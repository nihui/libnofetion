#ifndef SSICNET_H
#define SSICNET_H

#include "httppacket.h"

typedef struct __ssic_net_t ssic_net_t;

ssic_net_t* ssic_net_create();
int ssic_net_connect(ssic_net_t* net, const char* host, const char* port);
void ssic_net_destroy(ssic_net_t* net);
int ssic_net_init_ssl(ssic_net_t* net);
void ssic_net_send_http_packet(ssic_net_t* net, http_packet_t* packet);
http_packet_t* ssic_net_receive_http_packet(ssic_net_t* net);

#endif // SSICNET_H
