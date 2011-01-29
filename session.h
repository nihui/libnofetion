#ifndef SESSION_H
#define SESSION_H

typedef struct __nofetion_session_t nofetion_session_t;

nofetion_session_t* nofetion_session_create(const char* no);
void nofetion_session_destroy(nofetion_session_t* session);

void nofetion_session_login(nofetion_session_t* session, const char* password);
void nofetion_session_mainloop_start(nofetion_session_t* session);
void nofetion_session_mainloop_quit(nofetion_session_t* session);
void nofetion_session_logout(nofetion_session_t* session);

#endif // SESSION_H
