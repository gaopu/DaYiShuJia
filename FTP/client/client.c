#include <sys/types.h>
#include <signal.h>

int main(int argc, char *argv[])
{
	int sock_fd = 0;
	pid_t pid;

	signal(SIGCHLD,SIG_IGN);	//避免僵尸进程
	signal(SIGINT,SIG_IGN);		//忽略ctlr+c
	sock_fd = socket_init();
	do_cmd(sock_fd);

	return 0;
}
