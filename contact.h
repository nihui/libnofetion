#ifndef CONTACT_H
#define CONTACT_H

#include "status.h"

typedef struct __contact_t contact_t;

contact_t* contact_create(int id, const char* uri);
void contact_destroy(contact_t* contact);

int contact_get_sid(const contact_t* contact);

void contact_set_group(contact_t* contact, int groupid);
int contact_get_group(const contact_t* contact);
void contact_set_status(contact_t* contact, status_t status);
status_t contact_get_status(const contact_t* contact);
void contact_set_nickname(contact_t* contact, const char* nickname);
const char* contact_get_nickname(const contact_t* contact);
void contact_set_localname(contact_t* contact, const char* localname);
const char* contact_get_localname(const contact_t* contact);

void contact_retrieve_info(contact_t* contact);
void contact_init_chat(contact_t* contact);
void contact_send_sms(contact_t* contact, const char* text);

typedef struct __contact_list_t contact_list_t;

contact_list_t* contactlist_create();
void contactlist_destroy(contact_list_t* contactlist);

int contactlist_exists(contact_list_t* contactlist, int id);
int contactlist_add_contact(contact_list_t* contactlist, contact_t* contact);
void contactlist_delete(contact_list_t* contactlist, int id);

#endif // CONTACT_H
