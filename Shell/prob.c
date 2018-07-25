#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
// #include <paths.h>

#define MAX_ITEMS	10
#define MAX_LEN		256
#define MAX_BACKGROUND 3
extern char **environ;

pid_t bpids[MAX_BACKGROUND] = {-1,-1,-1};
char bcmds[MAX_BACKGROUND][MAX_LEN] = {0, 0, 0};
int bcnt=0;

int parseitems (char* cmdln, char items[][MAX_LEN]) {
	int i = 0;
	char* pch = NULL;

	pch = strtok (cmdln, " \n");
	while (pch != NULL && i < MAX_ITEMS) {
		strcpy (items[i], pch);
		pch = strtok (NULL, " \n");
		i++;
	}

	return i;
}
void savebproc (pid_t pid, char* cmd) {
	int i = 0;
	int find = 0;
	for (i = 0; i < sizeof(bpids)/sizeof(pid_t); ++i) {
		if (bpids[i] == -1) {
			bpids[i] = pid;
			strcpy (bcmds[i], cmd);
			break;
		}
	}
}
int removebproc (pid_t pid) {
	int i = 0;
	int check=0;
	for (i = 0; i < sizeof(bpids)/sizeof(pid_t); i++) {
		if (bpids[i] == pid) {
			bpids[i] = -1;
			check=1;
			break;
		}
	}
	return check;
}
void lsbpid () {
	int i = 0;
	for (i = 0; i < sizeof(bpids)/sizeof(pid_t); ++i) {
		if (bpids[i] != -1) {
			printf ("[%d] %s\n", bpids[i], bcmds[i]);
		}
	}
}

int builtincmd (char items[][MAX_LEN]) {
	if(!strcmp(items[0], "exit")) {
		printf("Goodbye!\n");
		exit(0);
	}
	if(!strcmp(items[0], "status")) {
		lsbpid();
		return 1;
	}
	if (strcmp (items[0], "close") == 0) {
		pid_t rpid = atoi (items[1]);
		if (removebproc (rpid) != 0) {
			int status;
			waitpid(rpid, &status, 0);
			printf ("[%d] closed\n", rpid);
		}
		return 1;
	}
	return 0;
}

int main (int argc, char* argv[]) {

	while(1) {
		int i, bg = 0;
		pid_t pid;
		char cmdln[MAX_LEN];
		char items[MAX_ITEMS][MAX_LEN];
		int nr_items;


		printf("MyCustomShell> ");
		fgets(cmdln, MAX_LEN, stdin);
		nr_items = parseitems (cmdln, items);


		if (builtincmd(items))
			continue;

		bg = strcmp(items[nr_items-1], "&") == 0 ? 1 : 0;
		if (bcnt>=MAX_BACKGROUND && bg) {
			printf ("cannot create more than %d background processes\n", MAX_BACKGROUND);
			continue;
		} else if(bg) ++bcnt;

		if((pid = fork())==0) {
			char* args[MAX_ITEMS];
			for (i = 0; i < nr_items; i++)
				args[i] = items[i];
			if (bg == 1)
				args[i-1] = NULL;
			else
				args[i] = NULL;

				// if (execve (args[0], args, ) < 0) {
			if (execvp (args[0], args) < 0)
				printf ("%s: command not found.\n", items[0]);	
			exit(0);
		} else {
			if(bg) {
				printf ("[%d] %s\n", pid, cmdln);
				savebproc(pid, cmdln);
			} else {
				int status;
				waitpid(pid, &status, 0);
			}
		}
	}
	exit(0);
}
