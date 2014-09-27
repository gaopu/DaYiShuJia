#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <netinet/in.h>
#include "server.h"
#include <signal.h>
#include <pwd.h>

char ROOT_DIR[50] = "/home/";			//FTP根目录//等会而会改成/home/用户名/ftp
const char LS[] = "ls";
const char CD[] = "cd";
const char GET[] = "get";
const char PUT[] = "put";
const char MKDIR[] = "mkdir";
const char PWD[] = "pwd";

int main(int argc, char *argv[])
{
	struct sockaddr_in client_addr;
	struct passwd *user;
	int sock_fd = 0,client_fd = 0;
	int client_addr_len = sizeof(client_addr);
	int stat = 0;
	char login_msg[30];
	pid_t pid;

	user = getpwuid(getuid());
	strcat(ROOT_DIR,user->pw_name);
	strcat(ROOT_DIR,"/ftp");		//找到了FTP根目录的地址
	system_log("<<服务器启动>>");
	sock_fd = socket_init();

	//修改工作目录到FTP根目录
	if (chdir(ROOT_DIR) == -1) {
		my_err("修改工作目录有误",__LINE__);
		perror_log("<<服务器退出>>");
		close(sock_fd);
		exit(1);
	}

	signal(SIGCHLD,SIG_IGN);	//忽略子进程退出的消息，避免僵尸进程
	while (1) {
		//接受连接，没有请求会一直阻塞
		printf("等待客户机接入...\n");
		client_fd = accept(sock_fd,(struct sockaddr *)&client_addr,&client_addr_len);
		printf("有客户机接入.\t");
		printf("IP是：%s\n",inet_ntoa(client_addr.sin_addr));
		strcpy(login_msg,inet_ntoa(client_addr.sin_addr));
		strcat(login_msg,"\t客户机接入");
		system_log(login_msg);

		pid = fork();
		if (!pid) {
			do_cmd(client_fd);
		}
	}

	return 0;
}
