#include "sipcnet.h"

// C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// linux headers
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>


typedef struct __handler_node_t {
    nofetion_session_t* session;
    sipevent_handler_t handler;
    union {
        struct __handler_node_t* next_handler;
        int using;
    };
} handler_node_t;

struct __sipc_net_t {
    int sfd;
    int looping;
    char* pending;
    handler_node_t* handlers;
    handler_node_t cb_array[32];
};

void set_reply_callback(sipc_net_t* net, nofetion_session_t* session, int callid, sipevent_handler_t cb_func)
{
    if (net->cb_array[callid % 32].using) {
        fprintf(stderr, "reply queue out of space\n");
        fprintf(stderr, "please check your network\n");
        return;
    }
    net->cb_array[callid % 32].session = session;
    net->cb_array[callid % 32].handler = cb_func;
    net->cb_array[callid % 32].using = 1;
}

static void invoke_reply_callback(sipc_net_t* net, sipevent_t* sipevent)
{
    int callid = sipevent_get_callid(sipevent);
    sipevent_handler_t cb_func = net->cb_array[callid % 32].handler;
    nofetion_session_t* session = net->cb_array[callid % 32].session;
    net->cb_array[callid % 32].using = 0;
    if (cb_func && session)
        cb_func(session, sipevent);
}

sipc_net_t* sipc_net_create()
{
    sipc_net_t* net = malloc(sizeof(sipc_net_t));
    memset(net, 0, sizeof(sipc_net_t));
    net->sfd = -1;
    return net;
}

int sipc_net_connect(sipc_net_t* net, const char* host, const char* port)
{
    struct addrinfo hints;
    struct addrinfo* result;
    struct addrinfo* rp;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;
    int s = getaddrinfo(host, port, &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        return -1;
    }
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        net->sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (net->sfd == -1)
            continue;
        if (connect(net->sfd, rp->ai_addr, rp->ai_addrlen) == 0)
            break;
        close(net->sfd);
        net->sfd = -1;
    }
    if (rp == NULL) {
        fprintf(stderr, "Could not connect\n");
        return -1;
    }
    freeaddrinfo(result);

    return 0;
}

int sipc_net_connect2(sipc_net_t* net, const char* hostport)
{
    char* host_end = strstr(hostport, ":");
    if (!host_end)
        return -1;
    char* host_str = strndup(hostport, host_end - hostport);
    char* port_str = strdup(host_end + 1);
    int ret = sipc_net_connect(net, host_str, port_str);
    free(host_str);
    free(port_str);
    return ret;
}

void sipc_net_destroy(sipc_net_t* net)
{
    close(net->sfd);
    handler_node_t* handler = net->handlers;
    handler_node_t* handler_next = NULL;
    while (handler) {
        handler_next = handler->next_handler;
        free(handler);
        handler = handler_next;
    }
    free(net);
}

void sipc_net_send_sipevent(sipc_net_t* net, sipevent_t* sipevent)
{
    /// send data
    int len;
    char* buf = sipevent_to_string(sipevent, &len);
    fprintf(stderr, "SEND: %s\n", buf);
    ssize_t nwrite = 0;
    int result_len = 0;
    for (;;) {
        nwrite = send(net->sfd, buf + result_len, len, 0);
        if (nwrite == -1) {
            fprintf(stderr, "write failed\n");
            break;
        }
        result_len += nwrite;
        len -= nwrite;
        if (len == 0)
            break;
    }
    free(buf);
}

void sipc_net_register_sipevent_handler(sipc_net_t* net, nofetion_session_t* session, sipevent_handler_t handler)
{
    handler_node_t* handler_node = malloc(sizeof(handler_node_t));
    handler_node->session = session;
    handler_node->handler = handler;
    handler_node->next_handler = NULL;

    handler_node_t* handler_node_tbp = net->handlers;
    if (!handler_node_tbp) {
        net->handlers = handler_node;
        return;
    }
    // get the handler node to be appended to
    while (handler_node_tbp->next_handler)
        handler_node_tbp = handler_node_tbp->next_handler;
    handler_node_tbp->next_handler = handler_node;
}

void sipc_net_deregister_sipevent_handler(sipc_net_t* net, sipevent_handler_t handler)
{
    handler_node_t* handler_node = net->handlers;
    while (handler_node) {
        if (handler_node->handler == handler) {
            // remove this handler
            if (handler_node == net->handlers)
                net->handlers = handler_node->next_handler;
            else {
                // get the handler node appended to
                handler_node_t* handler_node_tbp = net->handlers;
                while (handler_node_tbp->next_handler != handler_node)
                    handler_node_tbp = handler_node_tbp->next_handler;
                handler_node_tbp->next_handler = handler_node->next_handler;
            }
            free(handler_node);
            return;
        }
        handler_node = handler_node->next_handler;
    }
}

void sipc_net_thread_loop(sipc_net_t* net)
{
    /// FIXME WROKAROUND HERE timeout == 1 and net->looping variable
    /// TODO using a self pipe to indicate whether there exists a quitting loop message
    /// --- nihui
#define TIMEOUT    1
#define TIMEOUT_U  0
#define BUF_LEN    1024
    struct timeval tv;
    fd_set readfds;
    int ret;
    FD_ZERO(&readfds);
    FD_SET(net->sfd, &readfds);
    net->looping = 1;
    for (;;) {
        tv.tv_sec = TIMEOUT;
        tv.tv_usec = TIMEOUT_U;
        ret = select(net->sfd + 1, &readfds, NULL, NULL, &tv);
        if (ret == -1) {
            fprintf(stderr, "select function failed\n");
            return;
        }
        if (!net->looping) {
            /// explicit ending
            break;
        }
        if (!ret) {
            /// timeout
            continue;
        }

        if (FD_ISSET(net->sfd, &readfds)) {
            char buf[BUF_LEN + 1];
            int nread = recv(net->sfd, buf, BUF_LEN, 0);
            if (nread == -1) {
                fprintf(stderr, "read failed\n");
                return;
            }
            if (nread) {
                buf[nread] = '\0';
                /// PARSER ALGORITHM
                /// if pending data is valid
                ///     concat pending data and buf to data
                /// section data by "\r\n\r\n"
                /// if not found "\r\n\r\n"
                ///     pending data
                ///     out
                /// iterator all these sections
                ///     if found "\r\nL: "
                ///         extract content length to clen
                ///         if next section >= clen size
                ///             remove "L: clen\r\n" line
                ///             pick the clen from next section
                ///             generate sipevent
                ///             set sipevent content
                ///         else
                ///             pending data
                ///             out
                ///     else
                ///         generate sipevent
                ///     emit got sipevent
                char* data = NULL;
                if (net->pending) {
                    // concat pending data
                    data = malloc(strlen(net->pending) + nread + 1);
                    data[0] = '\0';
                    strcat(data, net->pending);
                    strcat(data, buf);
                    free(net->pending);
                    net->pending = NULL;
                }
                else
                    data = strdup(buf);

                fprintf(stderr, "@@@ DATA: %s\n", data);

                char* sep = strstr(data, "\r\n\r\n");
                if (!sep) {
                    // not enough, pending data
                    free(net->pending);
                    net->pending = data;
                    continue;
                }

                char* section_begin = data;
                char* section_end = sep;
                while (sep) {
                    section_end = sep;
                    char* sec = strndup(section_begin, section_end - section_begin);
                    char* lline = strstr(sec, "\r\nL: ");
                    if (!lline) {
                        // sipevent without content
                        sipevent_t* sipevent = sipevent_from_string(sec);
                        /// emit callback
                        if (sipevent_get_type(sipevent) == SIP_SIPC_4_0) {
                            invoke_reply_callback(net, sipevent);
                        }
                        else {
                            handler_node_t* handler = net->handlers;
                            while (handler) {
                                handler->handler(handler->session, sipevent);
                                handler = handler->next_handler;
                            }
                        }
                        sipevent_destroy(sipevent);
                        free(sec);
                        section_begin = section_end + 4;
                        sep = strstr(section_begin, "\r\n\r\n");
                        continue;
                    }

                    char* lline_begin = lline + 5;
                    char* lline_end = strstr(lline_begin, "\r\n");
                    char* len_str = NULL;
                    if (!lline_end)
                        lline_end = sec + strlen(sec);
                    len_str = strndup(lline_begin, lline_end - lline_begin);
                    fprintf(stderr, "##### LENSTRING: %s\n", len_str);
                    unsigned int clen = atoi(len_str);
                    free(len_str);

                    char* content_begin = section_end + 4;
                    if (strlen(content_begin) < clen) {
                        // content not enough, pending data
                        net->pending = strdup(section_begin);
                        free(sec);
                        break;
                    }

                    // sipevent with content
                    int newseclen = strlen(sec) - (lline_end - lline);
                    char* newsec = malloc(newseclen + 1);
                    newsec[0] = '\0';
                    strncat(newsec, sec, lline - sec);
                    strncat(newsec, lline_end, section_end - lline_end);
                    char* content_str = strndup(content_begin, clen);
                    sipevent_t* sipevent = sipevent_from_string(newsec);
//                     fprintf(stderr, "TTT: %s\n", newsec);
                    sipevent_set_content(sipevent, content_str);
                    /// emit callback
                    if (sipevent_get_type(sipevent) == SIP_SIPC_4_0) {
                        invoke_reply_callback(net, sipevent);
                    }
                    else {
                        handler_node_t* handler = net->handlers;
                        while (handler) {
                            handler->handler(handler->session, sipevent);
                            handler = handler->next_handler;
                        }
                    }
                    sipevent_destroy(sipevent);
                    free(newsec);
                    free(content_str);
                    free(sec);
                    section_begin = content_begin + clen;
                    if (strlen(section_begin) > 0) {
                        sep = strstr(content_begin, "\r\n\r\n");
                        continue;
                    }
                    break;
                }
                free(data);
            }
        }
    }
}

void sipc_net_thread_quit(sipc_net_t* net)
{
    net->looping = 0;
}
