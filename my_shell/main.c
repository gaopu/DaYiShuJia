/*
 * =====================================================================================
 *
 *       Filename:  main.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2014年07月22日 15时08分50秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  geekgao, gaopu333@gmail.com
 *        Company:  Class 1302 of Software Engineer
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <linux/limits.h>
#include "myshell.h"
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>

int main(int argc, char *argv[])
{
	char *cmd = NULL;						//存储一整条命令
	char arglist[CMD_COUNT][CMD_LEN] = {0};			//存储按空格分开的一个个元素
	int how = 0;									//存储处理的方式普通，|，>，<这些
	int backgnd = 0;								//标记是否有后台执行
	int cmd_count = 0,i;

	initialize_readline();
	walk_bin_dir();

	while (1) {
		backgnd = 0;
		cmd = getl();
		if (!strcmp(cmd,"exit")) {
			destroy_cmd_list();
			exit(0);
		}
		for (i = 0;i<strlen(cmd);i++)
		{
		  if (cmd[i] == '\"') {
			cmd[i] = ' ';
			
			//通过getl获取命令，只可能是将"变成空格导致最后有空格，否则最后不会有多余空格，
			//这里只能去除去最后一个"导致的空格
			if (i == strlen(cmd) - 1)
				cmd[i] = 0;			
		  }
		}
		cmd_count = split(cmd,arglist);
		if ((how = judge(arglist,&cmd_count,&backgnd)) == -1) {
			printf("命令错误.\n");
			continue;
		}
		do_cmd(how,backgnd,arglist,cmd_count);
	}
	destroy_cmd_list();

	return 0;
}












