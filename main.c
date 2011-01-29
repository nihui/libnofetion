#include "session.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

void* console_loop(void* session)
{
    char line[256] = {'\0'};
    while (1) {
        scanf("%s", line);
        puts(line);
        if (strncmp(line, "quit", 4) == 0) {
            nofetion_session_mainloop_quit(session);
            break;
        }
    }
    pthread_exit(NULL);
}

int main(int argc, char* argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s [mobile-no] [password]\n", argv[0]);
        return -1;
    }

    nofetion_session_t* session;
    session = nofetion_session_create(argv[1]);
    nofetion_session_login(session, argv[2]);

    pthread_t console_thread;
    pthread_create(&console_thread, NULL, console_loop, (void*)session);

    nofetion_session_mainloop_start(session);

    nofetion_session_logout(session);
    nofetion_session_destroy(session);

    return 0;
}
