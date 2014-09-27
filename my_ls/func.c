/*
 * =====================================================================================
 *
 *       Filename:  func.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2014年07月15日 15时39分36秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  geekgao, gaopu333@gmail.com
 *        Company:  Class 1302 of Software Engineer
 *
 * =====================================================================================
 */
#include <stdio.h>
#include "ls.h"
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>

int available_char = 100;	//当前打印行剩余的可打印字符数

void my_err(char *strerr,int line)
{
	fprintf(stderr,"%d行",line);
	perror(strerr);
}
void display(int param,const char *path)
{
	struct stat buf;
	char true_path[MAX_FILE_LEN];

	if (lstat(path,&buf) == -1) {
		my_err("lstat",__LINE__);
		return ;
	}
	if (S_ISDIR(buf.st_mode)) {
		//提供给display()的地址必须是绝对路径
		strcpy(true_path,path);
		//说明这个文件可能是当前工作目录的一个文件
		if (true_path[0] != '/') {
			strcpy(true_path,getcwd(NULL,0));
			strcat(true_path,"/");
			strcat(true_path,path);
		}
		display_dir(param,true_path);
	} else {
		display_file(param,path,20);
	}
}
void display_dir(int param,const char *path)
{
	DIR *dir;
	struct dirent *pdir;
	struct stat buf;
	char now_path[MAX_FILE_LEN],next_path[MAX_FILE_LEN];
	char file_name[MAX_FILE][MAX_FILE_LEN] = {0},tmp[MAX_FILE_LEN];
	int ifile = 0,i = 0,j = 0,max_filename = 0;

	strcpy(now_path,getcwd(NULL,0));
	//修改工作目录到参数的目录
	if (chdir(path) == -1) {
		my_err("chdir",__LINE__);
		return ;
	}
	//打开参数的目录
	if ((dir = opendir(path)) == NULL) {
		my_err("opendir",__LINE__);
		return ;
	}

	//存储名字字符串
	while ((pdir = readdir(dir)) != NULL) {
		if (lstat(pdir->d_name,&buf) == -1) {
			my_err("lstat",__LINE__);
			return ;
		}

		if (ifile >= MAX_FILE) {
			printf("%s文件夹下:\n文件太多,无法全部显示.\n",path);
			break;
		}

		if (strlen(pdir->d_name) > max_filename) {
			max_filename = strlen(pdir->d_name);
		}

		strcpy(file_name[ifile++],pdir->d_name);
	}

	//排序
	if (param & PARAM_t && param & PARAM_S) {
		printf("对不起，不能同时按两种方式排序.\n");
		exit(0);
	}
	//如果没有按什么排序的参数
	if (!(param & PARAM_t) && !(param & PARAM_S)) {
		qsort(file_name,ifile,sizeof(char) * MAX_FILE_LEN,str_cmp);
	}
	//此时已经进入了这些文件所在的文件夹可以在比较函数中用文件名获取文件大小
	else if (param & PARAM_S) {
		qsort(file_name,ifile,sizeof(char) * MAX_FILE_LEN,size_cmp);
	}
	else if (param & PARAM_t) {
		qsort(file_name,ifile,sizeof(char) * MAX_FILE_LEN,time_cmp);
	}

	//如果有R就要显示当前文件夹的名字
	if (param & PARAM_R) {
		printf("%s:\n",path);
	}

	//把当前文件夹下的所有文件都当作文件先显示出来
	for (i = 0;i< ifile;i++)
	{
		if (!(param & PARAM_a) && (file_name[i][0] == '.')) {
			continue;
		}
		display_file(param,file_name[i],max_filename);
	}

	if (param & PARAM_R) {
		for (i = 0;i < ifile;i++)
		{
			if (lstat(file_name[i],&buf) == -1) {
				my_err("lstat",__LINE__);
				return ;
			}
			if (!(S_ISDIR(buf.st_mode))) {
				continue;
			}
			if (!(param & PARAM_a) && (file_name[i][0] == '.')) {
				continue;
			}

			strcpy(next_path,path);
			if (path[strlen(path) - 1] != '/') {
				strcat(next_path,"/");
			}
			strcat(next_path,file_name[i]);
			//如果接下来要访问的目录是..或者.明显不能访问
			if (!strcmp(file_name[i],"..")) {
				continue;
			}
			if (!strcmp(file_name[i],".")) {
				continue;
			}
			available_char = MAX_LINE_LEN;
			printf("\n\n");
			display_dir(param,next_path);
		}
	}
	//更改工作目录到之前的文件夹
	if (chdir(now_path) == -1) {
		my_err("chdir",__LINE__);
		return ;
	}
	closedir(dir);
}
void display_file(int param,const char *filename,int max_filename)
{

	int len = 0,i = 0;

	if (param & PARAM_l) {
		display_single(filename);
	} else {
		len = strlen(filename);
		//打印文件名
		if (available_char >= len) {
			printf("%s",filename);
			available_char -= len;
		} else {
			printf("\n");
			available_char = MAX_LINE_LEN;
			printf("%s",filename);
			available_char -= len;
		}

		//打印空格//如果能容纳下剩余的空格
		if (available_char >= max_filename + SPACE_COUNT - len) {
			for (i = len + 1;i <= max_filename + SPACE_COUNT;i++,available_char--)
			{
				printf(" ");
			}
		} else {
			printf("\n");
			available_char = MAX_LINE_LEN;
		}
	}
}
void display_single(const char *filename)
{
	struct stat buf;
	struct passwd *psd;
	struct group *grp;
	char mtime[30];

	if (lstat(filename,&buf) == -1) {
		my_err("lstat",__LINE__);
		return ;
	}

	//显示文件类型
	if (S_ISDIR(buf.st_mode)) {
		printf("d");
	} else if (S_ISLNK(buf.st_mode)) {
		printf("l");
	} else if (S_ISSOCK(buf.st_mode)) {
		printf("s");
	} else if (S_ISFIFO(buf.st_mode)) {
		printf("p");
	} else if (S_ISREG(buf.st_mode)) {
		printf("-");
	} else if (S_ISCHR(buf.st_mode)) {
		printf("c");
	} else if (S_ISBLK(buf.st_mode)) {
		printf("b");
	}
	
	//所有者权限
	if (S_IRUSR & buf.st_mode) {
		printf("r");
	} else {
		printf("-");
	}
	if (S_IWUSR & buf.st_mode) {
		printf("w");
	} else {
		printf("-");
	}
	if (S_IXUSR & buf.st_mode) {
		printf("x");
	} else {
		printf("-");
	}
	//同组用户权限
	if (S_IRGRP & buf.st_mode) {
		printf("r");
	} else {
		printf("-");
	}
	if (S_IWGRP & buf.st_mode) {
		printf("w");
	} else {
		printf("-");
	}
	if (S_IXGRP & buf.st_mode) {
		printf("x");
	} else {
		printf("-");
	}
	//其他用户权限
	if (S_IROTH & buf.st_mode) {
		printf("r");
	} else {
		printf("-");
	}
	if (S_IWOTH & buf.st_mode) {
		printf("w");
	} else {
		printf("-");
	}
	if (S_IXOTH & buf.st_mode) {
		printf("x");
	} else {
		printf("-");
	}
	printf(". ");

	printf("%3d",buf.st_nlink);
	psd = getpwuid(buf.st_uid);
	grp = getgrgid(buf.st_gid);
	printf("%10s",psd->pw_name);
	printf(" ");
	printf("%10s",grp->gr_name);
	printf(" ");

	printf("%8d",buf.st_size);
	printf(" ");

	strcpy(mtime,ctime(&buf.st_mtime));
	mtime[strlen(mtime) - 1] = 0;
	printf("%s",mtime);
	printf(" ");

	printf("%s\n",filename);
}
int str_cmp(const void *a,const void *b)
{
	return strcmp((char *)a,(char *)b);
}
int size_cmp(const void *a,const void *b)
{
	struct stat bufa,bufb;

	if (lstat((char *)a,&bufa) == -1) {
		my_err("lstat",__LINE__);
		return ;
	}
	if (lstat((char *)b,&bufb) == -1) {
		my_err("lstat",__LINE__);
		return ;
	}

	return (bufa.st_size < bufb.st_size);
}
int time_cmp(const void *a,const void *b)
{
	struct stat bufa,bufb;

	if (lstat((char *)a,&bufa) == -1) {
		my_err("lstat",__LINE__);
		return ;
	}
	if (lstat((char *)b,&bufb) == -1) {
		my_err("lstat",__LINE__);
		return ;
	}

	return (bufa.st_mtime < bufb.st_mtime);
}






















