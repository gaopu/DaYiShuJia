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

int main(int argc, char *argv[])
{
	int param = 0,i = 0,num = 0;
	char path[300];		//path是文件名或者文件夹名
	char opt;

	//录入参数
	opterr = 0;			//不显示参数错误信息
	while ((opt = getopt(argc,argv,"alRtS")) != -1) {
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
		} else {
			printf("对不起，目前只支持参数R,S,a,t和l.\n");
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
