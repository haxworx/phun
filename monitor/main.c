#include "monitor.h"

int do_add(void *data)
{
	file_t *f = data;
	printf("it is %s\n", f->path);
}

int do_del(void *data)
{
	file_t *f = data;
	printf("it is %s\n", f->path);
}

int do_mod(void *data)
{
	file_t *f = data;
	printf("it is %s\n", f->path);
}


void setup_callbacks(void)
{
	monitor_add_callback = &do_add;
	monitor_del_callback = &do_del;
	monitor_mod_callback = &do_mod;
}

int main(void)
{
	time_t interval = 3;

	monitor_add("/home/netstar");

	setup_callbacks();

	monitor_init();

	while (monitor(interval)) {
		// after a scan do ...
		puts("done...");
	}
	
	return EXIT_SUCCESS;
}

