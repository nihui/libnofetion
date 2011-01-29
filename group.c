#include "group.h"

#include <stdlib.h>
#include <string.h>

struct __group_t {
    int id;
    char* name;
};

group_t* group_create(int id, const char* name)
{
    group_t* group = malloc(sizeof(group_t));
    group->id = id;
    group->name = strdup(name);
    return group;
}

void group_destroy(group_t* group)
{
    free(group->name);
    free(group);
}

int group_get_id(const group_t* group)
{
    return group->id;
}

const char* group_get_name(const group_t* group)
{
    return group->name;
}

typedef struct __group_node_t {
    group_t* group;
    struct __group_node_t* next_group;
} group_node_t;

struct __group_list_t {
    group_node_t* groups;
};

group_list_t* grouplist_create()
{
    group_list_t* grouplist = malloc(sizeof(group_list_t));
    grouplist->groups = NULL;
    return grouplist;
}

void grouplist_destroy(group_list_t* grouplist)
{
    group_node_t* group_node = grouplist->groups;
    group_node_t* group_node_next = NULL;
    while (group_node) {
        group_node_next = group_node->next_group;
        group_destroy(group_node->group);
        free(group_node);
        group_node = group_node_next;
    }
    free(grouplist);
}

int grouplist_exists(group_list_t* grouplist, int id)
{
    group_node_t* group_node = grouplist->groups;
    while (group_node) {
        if (group_node->group->id == id)
            return 1;
        group_node = group_node->next_group;
    }
    return 0;
}

int grouplist_add_group(group_list_t* grouplist, group_t* group)
{
    if (grouplist_exists(grouplist, group->id)) {
        group_destroy(group);
        return -1;
    }
    group_node_t* group_node = malloc(sizeof(group_node_t));
    group_node->group = group;
    group_node->next_group = NULL;

    group_node_t* group_node_tbp = grouplist->groups;
    if (!group_node_tbp) {
        grouplist->groups = group_node;
        return 0;
    }
    // get the group node to be appended to
    while (group_node_tbp->next_group)
        group_node_tbp = group_node_tbp->next_group;
    group_node_tbp->next_group = group_node;
    return 0;
}

int grouplist_add_group3(group_list_t* grouplist, int id, const char* name)
{
    return grouplist_add_group(grouplist, group_create(id, name));
}

void grouplist_delete(group_list_t* grouplist, int id)
{
    group_node_t* group_node = grouplist->groups;
    while (group_node) {
        if (group_node->group->id == id) {
            // remove this group
            if (group_node == grouplist->groups)
                grouplist->groups = group_node->next_group;
            else {
                // get the group node appended to
                group_node_t* group_node_tbp = grouplist->groups;
                while (group_node_tbp->next_group != group_node)
                    group_node_tbp = group_node_tbp->next_group;
                group_node_tbp->next_group = group_node->next_group;
            }
            group_destroy(group_node->group);
            free(group_node);
            return;
        }
        group_node = group_node->next_group;
    }
}
