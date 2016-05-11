/*
 * =====================================================================================
 *
 *       Filename:  my_ls.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2014年07月14日 20时37分58秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  geekgao, gaopu333@gmail.com
 *        Company:  Class 1302 of Software Engineer
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <string.h>
#include "ls.h"
#include <stdlib.h>
#include <unistd.h>
#include<sys/ioctl.h>

int MAX_LINE_LEN;	//终端宽度
int available_char;	//当前打印行剩余的可打印字符数

int main(int argc, char *argv[])
{
	//获取当前终端的宽度，并设置
	struct winsize size;
    ioctl(STDIN_FILENO,TIOCGWINSZ,&size);
	MAX_LINE_LEN = size.ws_col;
	available_char = MAX_LINE_LEN;
	
	int param = 0,i = 0,num = 0;
	char opt;

	//录入参数
	while ((opt = getopt(argc,argv,"alRtSu")) != -1) {
		if (opt == 'a') {
			param |= PARAM_a;
		} else if (opt == 'l') {
			param |= PARAM_l;
		} else if (opt == 'R') {
			param |= PARAM_R;
		} else if (opt == 'S') {
			param |= PARAM_S;
		} else if (opt == 't') {
			param |= PARAM_t;
		} else if (opt == 'u') {
			param |= PARAM_u;
		} else {
			printf("对不起，目前只支持参数R,S,a,t,u和l.\n");
			exit(0);
		}
	}
	
	for (i = 0;i < argc;i++)
	{
		if (argv[i][0] == '-') {
			num++;		//用来计算是否输入了文件名或者文件夹名
		}
	}

	//没有输入文件名或者文件夹名
	if (num + 1 == argc) {
		display(param,getcwd(NULL,0));
		argc = 1;	//不让进入下面的循环
	}

	//path是文件名或者文件夹名
	char path[MAX_FILE_LEN];
	for (i = 1;i < argc;i++)
	{
		if (argv[i][0] != '-')
		{
			strcpy(path,argv[i]);
			display(param,path);
		}
	}
	if (!(param & PARAM_l)) {
		printf("\n");
	}

	return 0;
}
