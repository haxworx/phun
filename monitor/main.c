#include "monitor.h"
#include <signal.h>
#include <unistd.h>

#define CMD_LEN 1024

int do_add(void *data)
{
	file_t *f = data;
	char cmd[CMD_LEN];
	snprintf(cmd, sizeof(cmd), "./add.sh %s", f->path);
	return system(cmd);
}

int do_del(void *data)
{
	file_t *f = data;
	char cmd[CMD_LEN];
	snprintf(cmd, sizeof(cmd), "./del.sh %s", f->path);
	return system(cmd);
}

int do_mod(void *data)
{
	file_t *f = data;
	char cmd[CMD_LEN];
	snprintf(cmd, sizeof(cmd), "./mod.sh %s", f->path);
	return system(cmd);
}

bool quit = false;

void exit_safe(int sig)
{
	if (sig != SIGINT && sig != SIGTERM) return;
	quit = true;
}

int main(int argc, char **argv)
{
	time_t interval = 3;
	bool recursive = true;
	
	if (argc != 2) exit (1 << 4);

	char *directory = argv[1];
	if (directory[strlen(directory) - 1] == '/'
		&& strlen(directory) > 1) {
		directory[strlen(directory) - 1] = '\0';
	}

	monitor_watch_add(directory);
	monitor_callback_set(MONITOR_ADD, do_add);
	monitor_callback_set(MONITOR_DEL, do_del);
	monitor_callback_set(MONITOR_MOD, do_mod);
	
	monitor_init(recursive);

	signal(SIGINT, exit_safe);
	signal(SIGTERM, exit_safe);

	// daemon(1, 0);
	
	while (monitor_watch(interval) && !quit) {
		// after a scan do ...
		puts("done...");
	}

	printf("Bye!\n");
	
	return EXIT_SUCCESS;
}

