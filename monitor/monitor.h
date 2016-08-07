#if ! defined(__MONITOR_H__)

#define __MONITOR_H__
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <limits.h>
#include <time.h>
#include <sys/stat.h>

#define SLASH '/'

#define MONITOR_NONE 0
#define MONITOR_ADD 1 
#define MONITOR_DEL 2 
#define MONITOR_MOD 3

typedef struct file_t file_t;
struct file_t {
	char *path;
	struct stat stats;
	int changed;
	file_t *next;
};

/* External functions */
int monitor_watch(int poll_interval);
void monitor_watch_add(const char *path);
void monitor_init(bool);
void monitor_process(void);


typedef int (*callback)(void *data);

callback monitor_add_callback;
callback monitor_del_callback;
callback monitor_mod_callback;

void monitor_callback_set(int type, callback func);

/* Internal functions */
	
void file_list_free(file_t *list);
file_t *file_list_add(file_t *list, const char *path, struct stat *st);
file_t *file_list_add_changes(file_t *list, const char *path, struct stat *st, int changes);
file_t *file_exists(file_t *list, const char *filename);
int _check_add_files(file_t *first, file_t *second);
int _check_del_files(file_t *first, file_t *second);
int _check_mod_files(file_t *first, file_t *second);
void file_lists_compare(file_t *first, file_t *second);
const char *directory_next(void);
void _list_append(file_t *one, file_t *two);
file_t *scan_recursive(const char *path);
file_t * monitor_files_get(file_t *list);
file_t *_monitor_compare_lists(file_t *one, file_t *two);

#endif
