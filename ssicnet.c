#include "ssicnet.h"

// openssl headers
#include <openssl/ssl.h>
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

struct __ssic_net_t {
    int sfd;
    SSL* ssl;
    SSL_CTX* ssl_ctx;
};

ssic_net_t* ssic_net_create()
{
    ssic_net_t* net = malloc(sizeof(ssic_net_t));
    net->sfd = -1;
    net->ssl = NULL;
    net->ssl_ctx = NULL;
    return net;
}

int ssic_net_connect(ssic_net_t* net, const char* host, const char* port)
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

    if (net->ssl && net->ssl_ctx) {
        int ret;
        ret = SSL_set_fd(net->ssl, net->sfd);
        if (ret == 0) {
            fprintf(stderr, "Set SSL to socket failed\n");
            return -1;
        }

        //     RAND_poll();
    //     while (RAND_status() == 0) {
    //         rand() %
    //     }
        ret = SSL_connect(net->ssl);
        if (ret != 1) {
            fprintf(stderr, "SSL connect failed\n");
            return -1;
        }
    }
    return 0;
}

void ssic_net_destroy(ssic_net_t* net)
{
    close(net->sfd);
    if (net->ssl && net->ssl_ctx) {
        SSL_free(net->ssl);
        SSL_CTX_free(net->ssl_ctx);
    }
    free(net);
}

int ssic_net_init_ssl(ssic_net_t* net)
{
    SSL_library_init();
    SSL_CTX* sslctx = SSL_CTX_new(SSLv23_client_method());
    if (sslctx == NULL) {
        fprintf(stderr, "Init SSL CTX failed\n");
        return -1;
    }
    SSL* ssl = SSL_new(sslctx);
    if (ssl == NULL) {
        fprintf(stderr, "Create SSL structure failed\n");
        SSL_CTX_free(sslctx);
        return -1;
    }
    net->ssl = ssl;
    net->ssl_ctx = sslctx;
    return 0;
}

void ssic_net_send_http_packet(ssic_net_t* net, http_packet_t* packet)
{
    int len;
    char* buf = http_packet_to_string(packet, &len);
    ssize_t nwrite = 0;
    int result_len = 0;
//     fprintf(stderr, "%s\n", buf);
    for (;;) {
        if (net->ssl && net->ssl_ctx)
            nwrite = SSL_write(net->ssl, buf + result_len, len);
        else
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

http_packet_t* ssic_net_receive_http_packet(ssic_net_t* net)
{
    char* buf = NULL;
    ssize_t nread = 0;
    int result_len = 0;
#define BUF_LEN 1024
    for (;;) {
        buf = realloc(buf, result_len + BUF_LEN + 1);
        if (net->ssl && net->ssl_ctx)
            nread = SSL_read(net->ssl, buf + result_len, BUF_LEN);
        else
            nread = recv(net->sfd, buf + result_len, BUF_LEN, 0);
        if (nread == -1) {
            fprintf(stderr, "read failed\n");
            free(buf);
            return NULL;
        }
        buf[result_len + nread] = '\0';
        result_len += nread;
        if (nread == 0)
            break;
    }
#undef BUF_LEN
//     fprintf(stderr, "%s\n", buf);
    http_packet_t* reply = http_packet_from_string(buf);
    free(buf);
    return reply;
}

