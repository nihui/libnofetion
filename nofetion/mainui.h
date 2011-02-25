// #include "inputedit.h"
#include "contactlist.h"

#include <ncurses.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../session.h"

int mainui_view(nofetion_session_t* session)
{
    CONTACTLIST* cl = create_contactlist(20,20,0,0);

    const contact_list_t* clt = nofetion_session_contactlist(session);

    const contact_node_t* cltn = clt->contacts;
    while (cltn) {
        const contact_t* c = cltn->contact;
        contactlist_addentry(cl, contact_get_nickname(c));
        cltn = cltn->next_contact;
    }

    int ch;
    while (1) {
        ch = getch();
        if (ch == KEY_F(1))
            break;

        switch (ch) {
            case KEY_DOWN:
                break;
            case KEY_UP:
                break;
            case KEY_ENTER:
                break;
            default:
                break;
        }
    }

    free_contactlist(cl);
    return -1;
}
