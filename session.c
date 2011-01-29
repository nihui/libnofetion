#include "session.h"

#include "sipevent.h"
#include "httppacket.h"
#include "sipcnet.h"
#include "ssicnet.h"

#include "user.h"
#include "group.h"
#include "contact.h"

// minixml headers
#include <mxml.h>
// openssl headers
#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>
// C headers
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
// linux headers
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

struct __nofetion_session_t {
    unsigned char* password_hash;
    char* ssic_cookie;
    char* sipc_proxy;
    user_t* user;
    group_list_t* grouplist;
    contact_list_t* contactlist;
};

static int nofetion_sipevent_handler(nofetion_session_t* session, const sipevent_t* sipevent);

static void nofetion_hash_password(int32_t userid, const char* password, unsigned char* out)
{
    /// construct hidden
    static const char const_str[] = "fetion.com.cn:";
    char* buf = malloc(strlen(const_str) + strlen(password) + 1);
    buf[0] = '\0';
    strcat(buf, const_str);
    strcat(buf, password);

    /// sha1 hash
    SHA1((unsigned char*)buf, strlen(buf), out);
    out[SHA_DIGEST_LENGTH] = '\0';

    if (userid) {
        char* newbuf = malloc(sizeof(int32_t) + SHA_DIGEST_LENGTH + 1);
        memcpy(newbuf, &userid, sizeof(int32_t));
        memcpy(newbuf + sizeof(int32_t), out, SHA_DIGEST_LENGTH + 1);
        SHA1((unsigned char*)newbuf, strlen(newbuf), out);
        out[SHA_DIGEST_LENGTH] = '\0';
        free(newbuf);
    }

    free(buf);
}

static void nofetion_binary_to_hex(const unsigned char* binary, char* out)
{
    int binary_len = strlen((const char*)binary);
    int i = 0;
    for (; i < binary_len; ++i) {
        sprintf(out, "%02X", binary[i]);
        out += 2;
    }
}

static void nofetion_generate_aeskey(unsigned char* out, int len)
{
    int rfd = open("/dev/urandom", O_RDONLY);
    int nread = read(rfd, out, len);
    close(rfd);
    fprintf(stderr,"NREAD: %d\n", nread);
}

static void nofetion_get_system_config(nofetion_session_t* session)
{
    /// get system config
    ssic_net_t* net = ssic_net_create();
    ssic_net_connect(net, "nav.fetion.com.cn", "80");

    /// send http packet
    char content[170] = {'\0'};
    sprintf(content, "<config><user mobile-no=\"%s\"/>"
                     "<client type=\"PC\" version=\"4.0.2510\" platform=\"W5.1\"/>"
                     "<servers version=\"0\"/><parameters version=\"0\"/>"
                     "<hints version=\"0\"/></config>", user_get_mobileno(session->user));
    http_packet_t* packet = http_packet_create(POST, "/nav/getsystemconfig.aspx HTTP/1.1");
    http_packet_add_header(packet, "User-Agent", "IIC2.0/PC 4.0.2510");
    http_packet_add_header(packet, "Host", "nav.fetion.com.cn");
    http_packet_add_header(packet, "Connection", "Close");
    http_packet_set_content(packet, content);

    ssic_net_send_http_packet(net, packet);
    http_packet_destroy(packet);

    /// receive http packet
    http_packet_t* reply = ssic_net_receive_http_packet(net);
    ssic_net_destroy(net);

    const char* ccc = http_packet_get_content(reply);
//     fprintf(stderr,"%s\n",ccc);
    mxml_node_t* tree = mxmlLoadString(NULL, ccc, MXML_TEXT_CALLBACK);
    mxml_node_t* servers_node = mxmlFindElement(tree, tree, "servers", NULL, NULL, MXML_DESCEND);
    mxml_node_t* ssi_node = mxmlFindElement(servers_node, tree, "ssi-app-sign-in-v2", NULL, NULL, MXML_DESCEND_FIRST);
    mxml_node_t* sipc_node = mxmlFindElement(servers_node, tree, "sipc-proxy", NULL, NULL, MXML_DESCEND_FIRST);

    char* ssi_app_sign_in = ssi_node->child->value.text.string;
    char* sipc_proxy = sipc_node->child->value.text.string;
    fprintf(stderr, "ssi_app_sign_in %s\n", ssi_app_sign_in);
    fprintf(stderr, "sipc_proxy      %s\n", sipc_proxy);

    session->sipc_proxy = strdup(sipc_proxy);

    mxmlDelete(ssi_node);
    mxmlDelete(sipc_node);
    mxmlDelete(servers_node);
    mxmlDelete(tree);

    http_packet_destroy(reply);
}

static int nofetion_ssic_auth(nofetion_session_t* session)
{
    /// encrypt hidden to hex
    char ohex[SHA_DIGEST_LENGTH * 2 + 1] = {'\0'};
    nofetion_binary_to_hex(session->password_hash, ohex);
    fprintf(stderr, "O: %s\n", ohex);

    /// ssic auth
    ssic_net_t* net = ssic_net_create();
    ssic_net_init_ssl(net);
    ssic_net_connect(net, "uid.fetion.com.cn", "443");

    /// send http packet
    char subUri[140] = {'\0'};
    sprintf(subUri, "/ssiportal/SSIAppSignInV4.aspx"
                    "?mobileno=%s"
                    "&domains=fetion.com.cn"
                    "&v4digest-type=1"
                    "&v4digest=%s", user_get_mobileno(session->user), ohex);
    http_packet_t* packet = http_packet_create(GET, subUri);
    http_packet_add_header(packet, "User-Agent", "IIC2.0/PC 4.0.2510");
    http_packet_add_header(packet, "Host", "uid.fetion.com.cn");
    http_packet_add_header(packet, "Cache-Control", "private");
    http_packet_add_header(packet, "Connection", "Keep-Alive");

    ssic_net_send_http_packet(net, packet);
    http_packet_destroy(packet);

    /// receive http packet
    http_packet_t* reply = ssic_net_receive_http_packet(net);
    ssic_net_destroy(net);

    const char* ccc = http_packet_get_content(reply);
    mxml_node_t* tree = mxmlLoadString(NULL, ccc, MXML_TEXT_CALLBACK);
    mxml_node_t* results_node = mxmlFindElement(tree, tree, "results", NULL, NULL, MXML_DESCEND);

    const char* status_code = mxmlElementGetAttr(results_node, "status-code");
    if (strcmp(status_code, "200") == 0) {
        /// ssic auth success
        mxml_node_t* user_node = mxmlFindElement(results_node, tree, "user", NULL, NULL, MXML_DESCEND_FIRST);
        const char* uri = mxmlElementGetAttr(user_node, "uri");
        const char* mobileno = mxmlElementGetAttr(user_node, "mobile-no");
        const char* userstatus = mxmlElementGetAttr(user_node, "user-status");
        const char* userid = mxmlElementGetAttr(user_node, "user-id");
        fprintf(stderr, "uri        %s\n", uri);
        fprintf(stderr, "mobileno   %s\n", mobileno);
        fprintf(stderr, "userstatus %s\n", userstatus);
        fprintf(stderr, "userid     %s\n", userid);
        user_set_id(session->user, atoi(userid));
        user_set_uri(session->user, uri);

        /// TODO: extract ssic cookie from http header

        mxmlDelete(user_node);
    }

    mxmlDelete(results_node);
    mxmlDelete(tree);

    http_packet_destroy(reply);

    return 0;
}

static int nofetion_sipc_register(nofetion_session_t* session)
{
    /// generate nouce
    srand(time(NULL));
    char nouce[33] = {'\0'};
    sprintf(nouce, "%04X%04X%04X%04X%04X%04X%04X%04X",
            rand() & 0xFFFF, rand() & 0xFFFF,
            rand() & 0xFFFF, rand() & 0xFFFF,
            rand() & 0xFFFF, rand() & 0xFFFF,
            rand() & 0xFFFF, rand() & 0xFFFF);

    sipc_net_t* sipc = sipc_net_create();
    sipc_net_connect2(sipc, session->sipc_proxy);
    user_set_sipcnet(session->user, sipc);

    /// sipc register
    sipevent_t* ev = sipevent_create(SIP_REGISTER, "fetion.com.cn SIP-C/4.0", user_get_sid(session->user));
    sipevent_add_header(ev, "CN", nouce);
    sipevent_add_header(ev, "CL", "type=\"pc\" ,version=\"4.0.2510\"");

    /// set up callback
    set_reply_callback(sipc, session, sipevent_get_callid(ev), nofetion_sipevent_handler);
    sipc_net_send_sipevent(sipc, ev);
    sipevent_destroy(ev);

    return 0;
}

int nofetion_sipevent_handler_main(nofetion_session_t* session, const sipevent_t* sipevent)
{
    session = NULL;
    switch (sipevent_get_type(sipevent)) {
        case SIP_BENOTIFY: {
            sipevent_dump(stderr, sipevent);
            const char* n_str = sipevent_get_header_value(sipevent, "N");
            if (strcmp(n_str, "PresenceV4") == 0) {
                ///
                const char* content = sipevent_get_content(sipevent);
                fprintf(stderr, "PC: %s\n", content);
            }
            else if (strcmp(n_str, "Conversation") == 0) {
                ///
            }
            else if (strcmp(n_str, "contact") == 0) {
                ///
            }
            else if (strcmp(n_str, "registration") == 0) {
                ///
            }
            else if (strcmp(n_str, "SyncUserInfoV4") == 0) {
                ///
            }
            else if (strcmp(n_str, "PGGroup") == 0) {
                ///
            }
            break;
        }
        case SIP_INVITE: {
            sipevent_dump(stderr, sipevent);
            break;
        }
        default: {
            sipevent_dump(stderr, sipevent);
            break;
        }
    }

    return 0;
}

int nofetion_sipevent_handler2(nofetion_session_t* session, const sipevent_t* sipevent)
{
    fprintf(stderr, "nofetion_sipevent_handler2\n");
    sipevent_dump(stderr, sipevent);
    const char* add_str = sipevent_get_type_addition(sipevent);
    if (strcmp(add_str, "200 OK") == 0) {
        /// login success!
        const char* sipcontent = sipevent_get_content(sipevent);
        mxml_node_t* tree = mxmlLoadString(NULL, sipcontent, MXML_TEXT_CALLBACK);
        /// extract buddy lists
        mxml_node_t* buddylist_node = mxmlFindElement(tree, tree, "buddy-list", NULL, NULL, MXML_DESCEND);
        mxml_node_t* buddylist_node_next = NULL;
        while (buddylist_node) {
            const char* buddylist_id = mxmlElementGetAttr(buddylist_node, "id");
            const char* buddylist_name = mxmlElementGetAttr(buddylist_node, "name");
            fprintf(stderr, "buddylist ID:   %s\n", buddylist_id);
            fprintf(stderr, "buddylist NAME: %s\n", buddylist_name);
            grouplist_add_group3(session->grouplist, atoi(buddylist_id), buddylist_name);
            buddylist_node_next = mxmlFindElement(buddylist_node, tree, "buddy-list", NULL, NULL, MXML_DESCEND);
            mxmlDelete(buddylist_node);
            buddylist_node = buddylist_node_next;
        }
        /// extract buddies
        mxml_node_t* buddies_node = mxmlFindElement(tree, tree, "buddies", NULL, NULL, MXML_DESCEND);
        mxml_node_t* buddy_node = mxmlFindElement(buddies_node, tree, "b", NULL, NULL, MXML_DESCEND_FIRST);
        mxml_node_t* buddy_node_next = NULL;
        while (buddy_node) {
            const char* buddy_id = mxmlElementGetAttr(buddy_node, "i");
            const char* buddy_uri = mxmlElementGetAttr(buddy_node, "u");
            const char* buddy_l = mxmlElementGetAttr(buddy_node, "l");
            fprintf(stderr, "buddy ID:   %s\n", buddy_id);
            fprintf(stderr, "buddy URI:  %s\n", buddy_uri);
            fprintf(stderr, "buddy L:    %s\n", buddy_l);
            contact_t* contact = contact_create(atoi(buddy_id), buddy_uri);
            if (buddy_l[0])
                contact_set_group(contact, atoi(buddy_l));
            contactlist_add_contact(session->contactlist, contact);
            buddy_node_next = mxmlFindElement(buddy_node, tree, "b", NULL, NULL, MXML_DESCEND);
            mxmlDelete(buddy_node);
            buddy_node = buddy_node_next;
        }
        mxmlDelete(buddies_node);
        mxmlDelete(tree);

        /// send contact subscribe
        sipevent_t* ev = sipevent_create(SIP_SUBSCRIBE, "fetion.com.cn SIP-C/4.0", user_get_sid(session->user));
        sipevent_add_header(ev, "N", "PresenceV4");
        char body[] = "<args><subscription self=\"v4default;mail-count\" buddy=\"v4default\" version=\"0\"/></args>";
        sipevent_set_content(ev, body);
        set_reply_callback(user_get_sipcnet(session->user), session, sipevent_get_callid(ev), nofetion_sipevent_handler_main);
        sipc_net_send_sipevent(user_get_sipcnet(session->user), ev);
        sipevent_destroy(ev);
        return 0;
    }
    else {
        /// login failed, again
        nofetion_sipc_register(session);
        return 1;
    }
}

static int nofetion_sipevent_handler(nofetion_session_t* session, const sipevent_t* sipevent)
{
    sipevent_dump(stderr, sipevent);
    const char* w_str = sipevent_get_header_value(sipevent, "W");

    char* nonce_begin = strstr(w_str, "nonce=\"") + 7;
    char* nonce_end = strstr(nonce_begin, "\"");
    char* nonce = strndup(nonce_begin, nonce_end - nonce_begin);

    char* key_begin = strstr(nonce_end, "key=\"") + 5;
    char* key_end = strstr(key_begin, "\"");
    char* key = strndup(key_begin, key_end - key_begin);

    /// generate aeskey
    unsigned char aeskey[33] = {'\0'};
    nofetion_generate_aeskey(aeskey, 32);

    int hidden_len = strlen(nonce) + SHA_DIGEST_LENGTH + 32;
    unsigned char* hidden = malloc(hidden_len + 1);
    memset(hidden, 0, hidden_len + 1);
    memcpy(hidden, nonce, strlen(nonce));
    memcpy(hidden + strlen(nonce), session->password_hash, SHA_DIGEST_LENGTH);
    memcpy(hidden + strlen(nonce) + SHA_DIGEST_LENGTH, aeskey, 32);
    free(nonce);

    /// rsa encrypt
    char modulus[257] = {'\0'};
    char exponent[7] = {'\0'};
    memcpy(modulus, key, 256);
    memcpy(exponent, key + 256, 6);
    free(key);

    BIGNUM* bnn = BN_new();
    BIGNUM* bne = BN_new();
    BN_hex2bn(&bnn, modulus);
    BN_hex2bn(&bne, exponent);
    RSA* r = RSA_new();
    r->n = bnn;
    r->e = bne;
    r->d = NULL;
    int flen = RSA_size(r);
    unsigned char* out = malloc(flen + 1);
    memset(out, 0, flen + 1);
    int ret = RSA_public_encrypt(hidden_len, hidden, out, r, RSA_PKCS1_PADDING);
    RSA_free(r);
//     BN_clear_free(bnn);
//     BN_clear_free(bne);
    free(hidden);

    char* outhex = malloc(flen * 2 + 1);
    memset(outhex, 0, flen * 2 + 1);
    nofetion_binary_to_hex(out, outhex);
    free(out);
    fprintf(stderr, "rsa public encrypt %d: %s\n", ret, outhex);

    /// sipc auth
    sipevent_t* ev = sipevent_create(SIP_REGISTER, "fetion.com.cn SIP-C/4.0", user_get_sid(session->user));
    char a_str[1000] = {'\0'};
    sprintf(a_str, "Digest response=\"%s\",algorithm=\"SHA1-sess-v4\"", outhex);
    free(outhex);
    sipevent_add_header(ev, "A", a_str);
    sipevent_add_header(ev, "AK", "ak-value");
    char authContent[1000] = {'\0'};
    sprintf(authContent, "<args><device machine-code=\"001676C0E351\"/>"
                         "<caps value=\"1ff\"/><events value=\"7f\"/>"
                         "<user-info mobile-no=\"%s\" user-id=\"%d\">"
                         "<personal version=\"\" attributes=\"v4default\"/>"
                         "<custom-config version=\"\"/>"
                         "<contact-list version=\"\" buddy-attributes=\"v4default\"/></user-info>"
                         "<credentials domains=\"fetion.com.cn\"/>"
                         "<presence><basic value=\"0\" desc=\"\"/></presence></args>",
                         user_get_mobileno(session->user), user_get_id(session->user));
    sipevent_set_content(ev, authContent);

    set_reply_callback(user_get_sipcnet(session->user), session, sipevent_get_callid(ev), nofetion_sipevent_handler2);
    sipc_net_send_sipevent(user_get_sipcnet(session->user), ev);
    sipevent_destroy(ev);

    return 0;
}

nofetion_session_t* nofetion_session_create(const char* no)
{
    nofetion_session_t* session = malloc(sizeof(nofetion_session_t));
    session->password_hash = malloc(SHA_DIGEST_LENGTH + 1);
    session->sipc_proxy = NULL;
    session->user = user_create(no);
    session->grouplist = grouplist_create();
    session->contactlist = contactlist_create();
    return session;
}

void nofetion_session_destroy(nofetion_session_t* session)
{
    free(session->password_hash);
    free(session->sipc_proxy);
    user_destroy(session->user);
    grouplist_destroy(session->grouplist);
    contactlist_destroy(session->contactlist);
    free(session);
}

void nofetion_session_login(nofetion_session_t* session, const char* password)
{
    nofetion_hash_password(0, password, session->password_hash);
    nofetion_get_system_config(session);
    nofetion_ssic_auth(session);

    nofetion_hash_password(user_get_id(session->user), password, session->password_hash);
    nofetion_sipc_register(session);
}

void nofetion_session_mainloop_start(nofetion_session_t* session)
{
    sipc_net_register_sipevent_handler(user_get_sipcnet(session->user), session, nofetion_sipevent_handler_main);
    sipc_net_thread_loop(user_get_sipcnet(session->user));
}

void nofetion_session_mainloop_quit(nofetion_session_t* session)
{
    /// deregister all the event handlers
    sipc_net_thread_quit(user_get_sipcnet(session->user));
}

void nofetion_session_logout(nofetion_session_t* session)
{
    fprintf(stderr, "calling nofetion_session_logout...\n");
    sipc_net_destroy(user_get_sipcnet(session->user));
}
