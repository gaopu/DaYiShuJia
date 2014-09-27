/*
 * =====================================================================================
 *
 *       Filename:  func.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2014年07月22日 15时32分51秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  geekgao, gaopu333@gmail.com
 *        Company:  Class 1302 of Software Engineer
 *
 * =====================================================================================
 */

#include <stdio.h>
#include "myshell.h"
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

void print()
{
	printf("[gaopu-shell]$ ");
}
int judge(char arglist[][CMD_LEN],int *cmd_count,int *backgnd)
{
	int i = 0,flag = 0,how = 0;

	for (i=0;i < *cmd_count; i++) {
		if (!strcmp(arglist[i],"&")) {
			if (i == *cmd_count - 1) {
				*backgnd = 1;
				(*cmd_count)--;
				break;
			}
			else {
				return -1;
			}
		}
	}

	for (i=0;i < *cmd_count; i++) {
		if (!strcmp(arglist[i], ">")) {
			flag++;
			how = REDIR_OUT;
			if (i + 1 == *cmd_count) 
				flag++;
		}
		if (!strcmp(arglist[i],"<")) {
			flag++;
			how = REDIR_IN;
			if (i + 1 == *cmd_count) {
				flag++;
			}
			if(i == 0) 
				flag++;
		}
		if (!strcmp(arglist[i],"|")) {
			if (i == 1) {
				flag++;
			}
			how = PIPE;
			if(i + 1 == *cmd_count)
				flag++;
			if(i == 0 )
				flag++;
		}
	}

	if (flag > 1) {
		return -1;
	}
	return how;
}
int split(char *cmd,char arglist[][CMD_LEN])
{
	int count = 0,i = 0;
	char *q = cmd;

	while (cmd) {
		//跳过前面的空格
		while (*cmd == ' ') {
			cmd++;
		}

		for (q = cmd,i = 0; *q && *q != ' ' && *q != '\n';q++,i++);
		strncpy(arglist[count],cmd,i);
		arglist[count][i] = 0;
		//printf("arglist[%d]:%s\n",count,arglist[count]);
		count++;
		if (!(*q)) {
			break;
		}
		cmd = q + 1;
	}
	return count;
}
void do_cmd(int how,int backgnd,char arglist[][CMD_LEN],int cmd_count)
{
	pid_t pid;
	int stat = 0;
	char *arg[CMD_COUNT];
	int i = 0,fd = 0,j = 0;

	for (i = 0;i < cmd_count;i++)
	{
		arg[i] = arglist[i];
	}
	arg[i] = NULL;

	if ((pid = vfork()) == -1) {
		printf("进程创建失败.\n");
		exit(1);
	}
	//子进程执行
	if (!pid) {
		switch (how) {
			case GENERAL:
				if (execvp(arg[0],arg) == -1) {
					printf("无此命令.\n");
					exit(1);
				}
				break;

			case REDIR_OUT:
				for (i = 0;i < cmd_count;i++)
				{
					if (!strcmp(arg[i],">")) {
						arg[i] = NULL;
						break;
					}
				}
				if ((fd = open(arg[i + 1],O_RDWR|O_CREAT|O_TRUNC,0644)) == -1) {
					printf("创建文件出错.\n");
					exit(1);
				}
				dup2(fd,1);
				if (execvp(arg[0],arg) == -1) {
					printf("无此命令.\n");
					exit(1);
				}
				break;

			case REDIR_IN:
				for (i = 0;i < cmd_count;i++)
				{
					if (!strcmp(arg[i],"<")) {
						arg[i] = NULL;
						break;
					}
				}
				if ((fd = open(arg[i + 1],O_RDONLY)) == -1) {
					printf("创建文件出错.\n");
					exit(1);
				}
				dup2(fd,0);
				if (execvp(arg[0],arg) == -1) {
					printf("无此命令.\n");
					exit(1);
				}
				break;

			case PIPE:
				excute_pipe(arg,cmd_count,WRITE);
				exit(0);	//子进程在这里结束
				break;

			default:
				break;
		}
	}

	if (backgnd) {
		printf("[%d] 后台运行\n",pid);
		return ;
	}
	if (waitpid(pid,&stat,0) == -1) {
		printf("等待子进程结束出错.\n");
	}
}
void excute_pipe(char *arg[],int cmd_count,int mod)
{
	int pid2 = 0;
	int stat2 = 0;
	int fd2 = 0,fd3 = 0,now_cmd_count = 0,next_cmd_count = 0,i = 0,j = 0;
	char *argnext[CMD_COUNT];


/*	for (i = 0;i<cmd_count;i++)
	{
		printf("arg[%d]:%s\n",i,arg[i]);
	}*/
	for (i = 0;i < cmd_count;i++)
	{
		now_cmd_count++;
		if (!strcmp(arg[i],"|")) {
			arg[i] = NULL;
			for (i = i + 1,j = 0;i < cmd_count;i++,j++)
			{
				argnext[j] = arg[i];
			}
			argnext[j] = NULL;
			break;
		}
	}
	next_cmd_count = cmd_count - now_cmd_count;

/*	printf("cmd_count:%d\n",cmd_count);
	printf("next_cmd_count:%d\n",next_cmd_count);
	printf("now_cmd_count%d\n",now_cmd_count);
	printf("mod:%d\n",mod);*/

	if ((pid2 = vfork()) == -1) {
		printf("创建子进程2失败.\n");
		exit(1);
	}
	if (!pid2) {
		//如果是输出到文件
		//WRITE对应第一次进入excute_pipe函数
		if (mod == WRITE) {
			if ((fd2 = open("/tmp/gaopu-shell-tmp2",O_RDWR|O_CREAT|O_TRUNC,0644)) == -1) {
				printf("创建临时文件出错.\n");
				exit(1);
			}
			dup2(fd2,1);
			if (execvp(arg[0],arg) == -1) {
				printf("无此命令.\n");
				exit(1);
			}
		} else {
			if ((fd2 = open("/tmp/gaopu-shell-tmp",O_RDONLY)) == -1) {
				printf("读取临时文件出错.\n");
				exit(1);
			}
			//后面还有管道
			if (next_cmd_count) {
				if ((fd3 = open("/tmp/gaopu-shell-tmp2",O_RDWR|O_CREAT|O_TRUNC,0644)) == -1) {
					printf("创建临时文件2出错.\n");
					exit(1);
				}
				dup2(fd3,1);
			}
			dup2(fd2,0);
			if (execvp(arg[0],arg) == -1) {
				printf("无此命令.\n");
				exit(1);
			}
		}
	}

	if (waitpid(pid2,&stat2,0) == -1) {
		printf("等待子进程2结束出错.\n");
		exit(1);
	}
	rename("/tmp/gaopu-shell-tmp2","/tmp/gaopu-shell-tmp");
	if (next_cmd_count) {
			excute_pipe(argnext,next_cmd_count,READ);
	}
	return ;
}












