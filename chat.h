#ifndef CHAT_H
#define CHAT_H

void chat_invite_contact(chat_t* chat, const char* buddyid);
void chat_send_message(chat_t* chat, const char* content);
void chat_send_nudge(chat_t* chat);

#endif // CHAT_H
