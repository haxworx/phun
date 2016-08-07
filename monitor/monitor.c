#include "monitor.h"

#define DIRS_MAX 12

static char *directories[DIRS_MAX];
static int _d_idx = 0;
static int _w_pos = 0;
static file_t *list_prev = NULL, *list_now = NULL;
static file_t *list_changes = NULL;
static bool _was_initialized = false;

static void 
file_list_free(file_t *list)
{
	file_t *c = list;

	while (c) {
		file_t *next = c->next;
		free(c->path);
		free(c);
		c = next;
	}
}

static file_t * 
file_list_add(file_t *list, const char *path, struct stat *st)
{
	file_t *c = list;

	while (c->next)
		c = c->next;

	if (! c->next) {
		c->next = calloc(1, sizeof(file_t));

		c = c->next;
		c->next = NULL;
	
		c->stats = *st;	
		c->path = strdup(path);
		c->changed = 0; 
	}

	return list;
}

static file_t *
file_list_add_changes(file_t *list, const char *path, struct stat *st, int changes)
{
	file_t *c = list;

	while (c->next)
		c = c->next;

	if (! c->next) {
		c->next = calloc(1, sizeof(file_t));

		c = c->next;
		c->next = NULL;
	
		c->stats = *st;	
		c->path = strdup(path);
		c->changed = changes;
	}

	return list;
}

static file_t *
file_exists(file_t *list, const char *filename)
{
	file_t *f = list->next;

	while (f) {
		if (!strcmp(f->path, filename))
			return f;
		f = f->next;
	}
	
	return NULL;
}

static int
_check_add_files(file_t *first, file_t *second)
{
	file_t *f = second->next;
	int changes = 0;

	while (f) {
		file_t *exists = file_exists(first, f->path);
		if (!exists) {
			f->changed = MONITOR_ADD;
			list_changes = file_list_add_changes(list_changes, f->path, &f->stats, MONITOR_ADD);
			monitor_add_callback(f);
#if defined(DEBUG)		
			printf("add file : %s\n", f->path);
#endif
			++changes;
		}
		f = f->next;
	}

	return changes;
}

static int
_check_del_files(file_t *first, file_t *second)
{
	file_t *f = first->next;
	int changes = 0;

	while (f) {
		file_t *exists = file_exists(second, f->path);
		if (!exists) {
			f->changed = MONITOR_DEL;
			list_changes = file_list_add_changes(list_changes, f->path, &f->stats, MONITOR_DEL);
			monitor_del_callback(f);
#if defined(DEBUG)
			printf("del file : %s\n", f->path);
#endif
			changes++;
		}
		f = f->next;
	}

	return changes;
}

static int
_check_mod_files(file_t *first, file_t *second)
{
	file_t *f = second->next;
	int changes = 0;
	
	while (f) {
		file_t *exists = file_exists(first, f->path);
		if (exists) {
			if (f->stats.st_mtime != exists->stats.st_mtime) {
				f->changed = MONITOR_MOD;
				list_changes = file_list_add_changes(list_changes, f->path, &f->stats, MONITOR_MOD);
				monitor_mod_callback(f);
#if defined(DEBUG)
				printf("mod file : %s\n", f->path);
#endif
				++changes;	
			}
		}
		f = f->next;
	}
	
	return changes;
}

void 
file_lists_compare(file_t *first, file_t *second)
{
	int modifications = 0;

	modifications += _check_del_files(first, second);

	modifications += _check_add_files(first, second);

	modifications += _check_mod_files(first, second);

}

static const char *
directory_next(void)
{
	if (directories[_w_pos] == NULL) {
		_w_pos = 0; 
		return NULL;
	}

	return directories[_w_pos++];
}

static void
 _list_append(file_t *one, file_t *two)
{
	file_t *c = one;
	while (c->next)
		c = c->next;

	c->next = two->next;
}

static file_t * 
scan_recursive(const char *path)
{
	DIR *dir = opendir(path);
	struct dirent *dh = NULL;
	char *directories[8192] = { NULL };
	int i = 0;

	file_t *list = calloc(1, sizeof(file_t));
	list->next = NULL;


	while ((dh = readdir(dir)) != NULL) {
		if (dh->d_name[0] == '.') continue;
		
		char buf[PATH_MAX];
		snprintf(buf, sizeof(buf), "%s%c%s", path, SLASH, dh->d_name);
		struct stat fstat;
		if (stat(buf, &fstat) < 0) continue;
		if (S_ISDIR(fstat.st_mode)) {
			directories[i++] = strdup(buf);			
			continue;
		} else {
			list = file_list_add(list, buf, &fstat);
		}
	}

	closedir(dir);


	i = 0;

	return list;

	/* 
	 * We could monitor EVERY file recursively but
	 * that is probably not a good idea!
	*/

	while (directories[i] != NULL) {
		file_t *next = NULL;
		next = scan_recursive(directories[i]);
		_list_append(list, next);
		free(directories[i++]);
	} 

	return list;
}

static file_t *
monitor_files_get(file_t *list)
{
	const char *path;

	while ((path = directory_next()) != NULL) {
		list = scan_recursive(path);
	}

	return list;
}


static file_t *
_monitor_compare_lists(file_t *one, file_t *two)
{
	file_lists_compare(one, two);
	file_list_free(one);
	one = two;

	return one;
}

int 
monitor(int poll)
{
	if (!_was_initialized) return 0;

	list_now = monitor_files_get(list_now);
	list_prev = _monitor_compare_lists(list_prev, list_now);  
	sleep(poll);

	return 1;
}
	
void
monitor_add(const char *path)
{
	if (_d_idx >= DIRS_MAX) 
		exit(1 << 1);

	directories[_d_idx++] = strdup(path);
}

void
monitor_init(void)
{
	if (_d_idx == 0) exit(1 << 0);

	directories[_d_idx] = NULL;
	directories[DIRS_MAX - 1] = NULL;

	list_prev = monitor_files_get(list_prev);	

	list_changes = calloc(1, sizeof(file_t));
	_was_initialized = true;
}

void
monitor_process(void)
{
	file_t *cursor = list_changes->next;
	if (!cursor) return;
 
	while (cursor) {
		printf("cursor->path is: %s\n", cursor->path);
		cursor = cursor->next;
	}

	cursor = list_changes->next;
	
	while (cursor) {
		file_t *next = cursor->next;
		free(cursor->path);
		free(cursor);
		cursor = next;
	}	

	list_changes->next = NULL;
}

