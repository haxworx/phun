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
int monitor(int poll);
void monitor_add(const char *path);
void monitor_init(void);
void monitor_process(void);

typedef int (*func)(void *data);
func monitor_add_callback;
func monitor_del_callback;
func monitor_mod_callback;


/* Internal functions */
	
static void file_list_free(file_t *list);
static file_t *file_list_add(file_t *list, const char *path, struct stat *st);
static file_t *file_list_add_changes(file_t *list, const char *path, struct stat *st, int changes);
static file_t *file_exists(file_t *list, const char *filename);
static int _check_add_files(file_t *first, file_t *second);
static int _check_del_files(file_t *first, file_t *second);
static int _check_mod_files(file_t *first, file_t *second);
static void file_lists_compare(file_t *first, file_t *second);
static const char *directory_next(void);
static void _list_append(file_t *one, file_t *two);
static file_t *scan_recursive(const char *path);
static file_t * monitor_files_get(file_t *list);
static file_t *_monitor_compare_lists(file_t *one, file_t *two);

#endif
