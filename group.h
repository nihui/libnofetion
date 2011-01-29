#ifndef GROUP_H
#define GROUP_H

typedef struct __group_t group_t;

group_t* group_create(int id, const char* name);
void group_destroy(group_t* group);

int group_get_id(const group_t* group);
const char* group_get_name(const group_t* group);

typedef struct __group_list_t group_list_t;

group_list_t* grouplist_create();
void grouplist_destroy(group_list_t* grouplist);

int grouplist_exists(group_list_t* grouplist, int id);
int grouplist_add_group(group_list_t* grouplist, group_t* group);
int grouplist_add_group3(group_list_t* grouplist, int id, const char* name);
void grouplist_delete(group_list_t* grouplist, int id);

#endif // GROUP_H
