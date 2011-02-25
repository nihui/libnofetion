#include <ncurses.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
    WINDOW* window;
    int count;
} CONTACTLIST;

CONTACTLIST* create_contactlist(int h, int w, int y, int x)
{
    CONTACTLIST* cl = malloc(sizeof(CONTACTLIST));
    memset(cl, 0, sizeof(CONTACTLIST));
    cl->window = newwin(h, w, y, x);
    box(cl->window, 0, 0);
    wrefresh(cl->window);
    return cl;
}

void free_contactlist(CONTACTLIST* cl)
{
    wborder(cl->window, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
    wrefresh(cl->window);
    delwin(cl->window);
    free(cl);
}

void contactlist_addentry(CONTACTLIST* cl, const char* entry)
{
    cl->count++;
    mvwprintw(cl->window, cl->count, 1, entry);
    wrefresh(cl->window);
}
