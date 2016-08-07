#include "monitor.h"

#define DIRS_MAX 12

char *directories[DIRS_MAX];
int _d_idx = 0;
int _w_pos = 0;
file_t *list_prev = NULL, *list_now = NULL;
bool _was_initialized = false;
bool _is_recursive = true;

void monitor_callback_set(int type, callback func)
{
        switch (type) {
        case MONITOR_ADD:
                monitor_add_callback = func;
                break;
        case MONITOR_DEL:
                monitor_del_callback = func;
                break;
        case MONITOR_MOD:
                monitor_mod_callback = func;
        };
}

void 
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

file_t * 
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

file_t *
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

int
_check_add_files(file_t *first, file_t *second)
{
	file_t *f = second->next;
	int changes = 0;

	while (f) {
		file_t *exists = file_exists(first, f->path);
		if (!exists) {
			f->changed = MONITOR_ADD;
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

int
_check_del_files(file_t *first, file_t *second)
{
	file_t *f = first->next;
	int changes = 0;

	while (f) {
		file_t *exists = file_exists(second, f->path);
		if (!exists) {
			f->changed = MONITOR_DEL;
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

int
_check_mod_files(file_t *first, file_t *second)
{
	file_t *f = second->next;
	int changes = 0;
	
	while (f) {
		file_t *exists = file_exists(first, f->path);
		if (exists) {
			if (f->stats.st_mtime != exists->stats.st_mtime) {
				f->changed = MONITOR_MOD;
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

const char *
directory_next(void)
{
	if (directories[_w_pos] == NULL) {
		_w_pos = 0; 
		return NULL;
	}

	return directories[_w_pos++];
}

void
 _list_append(file_t *one, file_t *two)
{
	file_t *c = one;
	while (c->next)
		c = c->next;

	if (two->next)
		c->next = two->next;
	else
		c->next = NULL;
}

file_t * 
scan_recursive(const char *path)
{
	DIR *dir = opendir(path);
	if (!dir) return NULL;
	struct dirent *dh = NULL;
	char *directories[8192] = { NULL };
	int i; 

	file_t *list = calloc(1, sizeof(file_t));
	list->next = NULL;

	for (i = 0; i < sizeof(directories) / sizeof(char *); i++) {
		directories[i] = NULL;
	}

	i = 0;

	while ((dh = readdir(dir)) != NULL) {
		if (dh->d_name[0] == '.') continue;
		
		char buf[PATH_MAX];
		snprintf(buf, sizeof(buf), "%s%c%s", path, SLASH, dh->d_name);
		struct stat fstat;
		if (stat(buf, &fstat) < 0) continue;

		if (S_ISLNK(fstat.st_mode)) continue;

		if (S_ISDIR(fstat.st_mode)) {
			directories[i++] = strdup(buf);			
			continue;
		} else {
			list = file_list_add(list, buf, &fstat);
		}
	}

	closedir(dir);

	i = 0;

	if (!_is_recursive) return list;
	
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

file_t *
monitor_files_get(file_t *list)
{
	const char *path;

	while ((path = directory_next()) != NULL) {
		list = scan_recursive(path);
	}

	return list;
}


file_t *
_monitor_compare_lists(file_t *one, file_t *two)
{
	file_lists_compare(one, two);
	file_list_free(one);
	one = two;

	return one;
}

int 
monitor_watch(int poll)
{
	if (!_was_initialized) return 0;

	list_now = monitor_files_get(list_now);
	list_prev = _monitor_compare_lists(list_prev, list_now);  
	sleep(poll);

	return 1;
}
	
void
monitor_watch_add(const char *path)
{
	if (_d_idx >= DIRS_MAX) 
		exit(1 << 1);

	directories[_d_idx++] = strdup(path);
}

void
monitor_init(bool recursive)
{
	if (_d_idx == 0) exit(1 << 0);
	if (!recursive) 
		_is_recursive = false;

	directories[_d_idx] = NULL;
	directories[DIRS_MAX - 1] = NULL;

	list_prev = monitor_files_get(list_prev);	

	_was_initialized = true;
}


