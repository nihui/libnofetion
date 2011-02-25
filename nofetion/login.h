
#include "inputedit.h"
#include <ncurses.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


int login_view(char* no, char* password)
{
    mvprintw(1, 1, "Welcome to nofetion!");
    mvprintw(3, 1, "MobileNo:");
    mvprintw(4, 1, "Password:");
    mvprintw(6, 1, "Press F1 to exit");
    mvprintw(7, 1, "Press Shift+Enter to login");
    refresh();

    INPUTEDIT* ie1 = create_inputedit(1,20,3,10, INPUTEDIT_ECHO_NORMAL, INPUTEDIT_PATTERN_NUM);
    INPUTEDIT* ie2 = create_inputedit(1,20,4,10, INPUTEDIT_ECHO_PASSWD, INPUTEDIT_PATTERN_ALPHANUM);
    INPUTEDIT* ie_cur = ie1;

    inputedit_focus(ie_cur);

    int ch;
    while (1) {
        ch = getch();
        if (ch == KEY_F(1))
            break;

        switch (ch) {
            case KEY_DOWN:
                ie_cur = ie2;
                inputedit_focus(ie_cur);
                break;
            case KEY_UP:
                ie_cur = ie1;
                inputedit_focus(ie_cur);
                break;
            case KEY_ENTER:
                strncpy(no, ie1->buf, 255);
                strncpy(password, ie2->buf, 255);
                mvprintw(1, 1, "                    ");
                mvprintw(3, 1, "         ");
                mvprintw(4, 1, "         ");
                mvprintw(6, 1, "                ");
                mvprintw(7, 1, "                          ");
                refresh();
                free_inputedit(ie1);
                free_inputedit(ie2);
                return 0;
            default:
                inputedit_addch(ie_cur, ch);
                break;
        }
    }
    return -1;
}
