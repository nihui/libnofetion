#include <ncurses.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define INPUTEDIT_ECHO_NORMAL 0
#define INPUTEDIT_ECHO_PASSWD 1

#define INPUTEDIT_PATTERN_ANY      0
#define INPUTEDIT_PATTERN_NUM      1
#define INPUTEDIT_PATTERN_ALPHA    2
#define INPUTEDIT_PATTERN_ALPHANUM 3

typedef struct {
    WINDOW* input_window;
    int echo;
    int pattern;
    char buf[256];
    int len;
} INPUTEDIT;

INPUTEDIT* create_inputedit(int h, int w, int y, int x, int echo, int pattern)
{
    INPUTEDIT* ie = malloc(sizeof(INPUTEDIT));
    memset(ie, 0, sizeof(INPUTEDIT));
    ie->echo = echo;
    ie->pattern = pattern;
    ie->input_window = newwin(h, w, y, x);
    box(ie->input_window, 0, 0);
    wborder(ie->input_window, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
    wrefresh(ie->input_window);
    return ie;
}

void free_inputedit(INPUTEDIT* ie)
{
    wborder(ie->input_window, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
    wrefresh(ie->input_window);
    delwin(ie->input_window);
    free(ie);
}

void inputedit_focus(INPUTEDIT* ie)
{
    wmove(ie->input_window, 0, ie->len);
    wrefresh(ie->input_window);
}

void inputedit_addch(INPUTEDIT* ie, int ch)
{
    if (ch == KEY_BACKSPACE) {
        ie->len--;
        if (ie->len < 0) {
            ie->len = 0;
            return;
        }
        ie->buf[ie->len] = '\0';
        mvwaddch(ie->input_window, 0, ie->len, ' ');
        wmove(ie->input_window, 0, ie->len);
    }
    else {
        switch (ie->pattern) {
            case INPUTEDIT_PATTERN_NUM:
                if (!isdigit(ch))
                    return;
                break;
            case INPUTEDIT_PATTERN_ALPHA:
                if (!isalpha(ch))
                    return;
                break;
            case INPUTEDIT_PATTERN_ALPHANUM:
                if (!isalnum(ch))
                    return;
                break;
            case INPUTEDIT_PATTERN_ANY:
                break;
        }
        if (ie->echo == INPUTEDIT_ECHO_NORMAL)
            mvwaddch(ie->input_window, 0, ie->len, ch | A_BOLD);
        else
            mvwaddch(ie->input_window, 0, ie->len, '*' | A_BOLD);
        ie->buf[ie->len] = ch;
        ie->len++;
    }
    wrefresh(ie->input_window);
}
