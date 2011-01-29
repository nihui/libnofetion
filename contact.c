#include "contact.h"

#include <stdlib.h>
#include <string.h>

struct __contact_t {
    int id;
    char* uri;
    int sid;
    int groupid;
    status_t status;
    char* nickname;
    char* localname;
};

contact_t* contact_create(int id, const char* uri)
{
    contact_t* contact = malloc(sizeof(contact_t));
    memset(contact, 0, sizeof(contact_t));
    contact->id = id;
    contact->uri = strdup(uri);
    char* sid_begin = strstr(uri, ":") + 1;
    char* sid_end = strstr(uri, "@");
    char* sid_str = strndup(sid_begin, sid_end - sid_begin);
    contact->sid = atoi(sid_str);
    free(sid_str);
    contact->groupid = 0;
    return contact;
}

void contact_destroy(contact_t* contact)
{
    free(contact->uri);
    free(contact->nickname);
    free(contact->localname);
    free(contact);
}

int contact_get_sid(const contact_t* contact)
{
    return contact->sid;
}

void contact_set_group(contact_t* contact, int groupid)
{
    contact->groupid = groupid;
}

int contact_get_group(const contact_t* contact)
{
    return contact->groupid;
}

void contact_set_status(contact_t* contact, status_t status)
{
    contact->status = status;
}

status_t contact_get_status(const contact_t* contact)
{
    return contact->status;
}

void contact_set_nickname(contact_t* contact, const char* nickname)
{
    free(contact->nickname);
    contact->nickname = strdup(nickname);
}

const char* contact_get_nickname(const contact_t* contact)
{
    return contact->nickname;
}

void contact_set_localname(contact_t* contact, const char* localname)
{
    free(contact->localname);
    contact->localname = strdup(localname);
}

const char* contact_get_localname(const contact_t* contact)
{
    return contact->localname ? contact->localname : contact->nickname;
}

// void contact_retrieve_info(contact_t* contact)
// {
// }

// void contact_init_chat(contact_t* contact)
// {
// }

// void contact_send_sms(contact_t* contact, const char* text)
// {
// }

typedef struct __contact_node_t {
    contact_t* contact;
    struct __contact_node_t* next_contact;
} contact_node_t;

struct __contact_list_t {
    contact_node_t* contacts;
};

contact_list_t* contactlist_create()
{
    contact_list_t* contactlist = malloc(sizeof(contact_list_t));
    contactlist->contacts = NULL;
    return contactlist;
}

void contactlist_destroy(contact_list_t* contactlist)
{
    contact_node_t* contact_node = contactlist->contacts;
    contact_node_t* contact_node_next = NULL;
    while (contact_node) {
        contact_node_next = contact_node->next_contact;
        contact_destroy(contact_node->contact);
        free(contact_node);
        contact_node = contact_node_next;
    }
    free(contactlist);
}

int contactlist_exists(contact_list_t* contactlist, int id)
{
    contact_node_t* contact_node = contactlist->contacts;
    while (contact_node) {
        if (contact_node->contact->id == id)
            return 1;
        contact_node = contact_node->next_contact;
    }
    return 0;
}

int contactlist_add_contact(contact_list_t* contactlist, contact_t* contact)
{
    if (contactlist_exists(contactlist, contact->id)) {
        contact_destroy(contact);
        return -1;
    }
    contact_node_t* contact_node = malloc(sizeof(contact_node_t));
    contact_node->contact = contact;
    contact_node->next_contact = NULL;

    contact_node_t* contact_node_tbp = contactlist->contacts;
    if (!contact_node_tbp) {
        contactlist->contacts = contact_node;
        return 0;
    }
    // get the contact node to be appended to
    while (contact_node_tbp->next_contact)
        contact_node_tbp = contact_node_tbp->next_contact;
    contact_node_tbp->next_contact = contact_node;
    return 0;
}

void contactlist_delete(contact_list_t* contactlist, int id)
{
    contact_node_t* contact_node = contactlist->contacts;
    while (contact_node) {
        if (contact_node->contact->id == id) {
            // remove this contact
            if (contact_node == contactlist->contacts)
                contactlist->contacts = contact_node->next_contact;
            else {
                // get the contact node appended to
                contact_node_t* contact_node_tbp = contactlist->contacts;
                while (contact_node_tbp->next_contact != contact_node)
                    contact_node_tbp = contact_node_tbp->next_contact;
                contact_node_tbp->next_contact = contact_node->next_contact;
            }
            contact_destroy(contact_node->contact);
            free(contact_node);
            return;
        }
        contact_node = contact_node->next_contact;
    }
}

