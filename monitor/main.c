#include "monitor.h"

int do_add(void *data)
{
	file_t *f = data;
	printf("ADD: %s\n", f->path);
	return 1;
}

int do_del(void *data)
{
	file_t *f = data;
	printf("DEL: %s\n", f->path);

	return 1;
}

int do_mod(void *data)
{
	file_t *f = data;
	printf("MOD: %s\n", f->path);

	return 1;
}

int main(void)
{
	time_t interval = 3;
	bool recursive = true;

	monitor_watch_add("/home/netstar");

	monitor_callback_set(MONITOR_ADD, do_add);
	monitor_callback_set(MONITOR_DEL, do_del);
	monitor_callback_set(MONITOR_MOD, do_mod);

	monitor_init(recursive);

	while (monitor_watch(interval)) {
		// after a scan do ...
		puts("done...");
	}
	
	return EXIT_SUCCESS;
}

