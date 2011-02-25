#include "../session.h"

#include "login.h"
#include "mainui.h"
#include <ncurses.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#include <sys/select.h>

nofetion_session_t* session;

void* keepalive(void* args)
{
    nofetion_session_keepalive(session);
    struct timeval tv;
    while (1) {
        tv.tv_sec = 40;
        tv.tv_usec = 0;
        select(0, NULL, NULL, NULL, &tv);
        nofetion_session_keepalive(session);
    }
    pthread_exit(NULL);
}

void* console_loop(void* args)
{
    char line[256] = {'\0'};
    while (1) {
        char* line = NULL;
        size_t len = 0;
        getline(&line, &len, stdin);
//         puts(line);
        if (strncmp(line, "quit", 4) == 0) {
            nofetion_session_mainloop_quit(session);
            free(line);
            break;
        }
        if (strncmp(line, "m", 1) == 0) {
            char sid[20] = {0};
            char* sid_begin = line + 2;
            char* sid_end = strchr(sid_begin, ' ');
            strncpy(sid, sid_begin, sid_end - sid_begin);
            char* msg = sid_end + 1;
//             puts(sid);
//             puts(msg);
            nofetion_session_send_message(session, sid, msg);
        }
//         free(line);
    }
    pthread_exit(NULL);
}

void login_cb()
{
    fprintf(stderr, "login over\n");

    pthread_t console_thread;
    pthread_create(&console_thread, NULL, console_loop, NULL);
    pthread_t keepalive_thread;
    pthread_create(&keepalive_thread, NULL, keepalive, NULL);
}

int main(int argc, char* argv[])
{
    int ret;
    char no[256] = {0};
    char password[256] = {0};

    if (argc == 3) {
        strncpy(no, argv[1], 255);
        strncpy(password, argv[2], 255);
    }
    else {
        initscr();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);

        ret = login_view(no, password);
        endwin();
        if (ret != 0) {
            fprintf(stderr, "error inputting login\n");
            goto OUT;
        }
    }


OUT:
    session = nofetion_session_create(no);
    nofetion_session_set_login_callback(session, login_cb);

    fprintf(stderr, "login...\n");
    nofetion_session_login(session, password);

    nofetion_session_mainloop_start(session);

    nofetion_session_logout(session);
    nofetion_session_destroy(session);


    return 0;
}
