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

extern const int MAX_LINE_LEN;
extern int available_char;

void my_err(char *strerr,int line) {
	fprintf(stderr,"%d行出现异常:",line);
	perror(strerr);
}

void display(int param,const char *path) {
	struct stat buf;
	char true_path[MAX_FILE_LEN];

	//通过文件路径获取文件属性信息
	if (lstat(path,&buf) == -1) {
		my_err("lstat",__LINE__);
		return ;
	}
	if (S_ISDIR(buf.st_mode)) {
		//不是绝对路径就要转化成绝对路径s
		if (path[0] != '/') {
			strcpy(true_path,getcwd(NULL,0));
			strcat(true_path,"/");
			strcat(true_path,path);
		} else {
			strcpy(true_path,path);
		}
		display_dir(param,true_path);
	} else {
		display_file(param,path,20);
	}
}

void display_dir(int param,const char *path) {
	struct stat buf;
	char now_path[MAX_FILE_LEN],next_path[MAX_FILE_LEN];
	char file_name[MAX_FILE][MAX_FILE_LEN] = {0},tmp[MAX_FILE_LEN];
	int fileCounts = 0,hideFileCounts = 0,i = 0,j = 0,max_filename = 0;

	strcpy(now_path,getcwd(NULL,0));
	//修改工作目录到参数的目录
	if (chdir(path) == -1) {
		my_err("chdir",__LINE__);
		return ;
	}
	
	DIR *dir;
	//打开参数的目录
	if ((dir = opendir(path)) == NULL) {
		my_err("opendir",__LINE__);
		return ;
	}

	struct dirent *pdir;
	//存储名字字符串
	while ((pdir = readdir(dir)) != NULL) {
		//根据文件名获取文件属性
		if (lstat(pdir->d_name,&buf) == -1) {
			my_err("lstat",__LINE__);
			return ;
		}

		if (fileCounts >= MAX_FILE) {
			printf("%s文件夹下:\n文件太多,无法全部显示.\n",path);
			break;
		}

		if (strlen(pdir->d_name) > max_filename) {
			max_filename = strlen(pdir->d_name);
		}

		strcpy(file_name[fileCounts++],pdir->d_name);
		if (pdir->d_name[0] == '.') {
			hideFileCounts++;
		}
	}

	//排序
	if (param & PARAM_t && param & PARAM_S) {
		printf("对不起，不能同时按两种方式排序.\n");
		exit(0);
	}
	//如果没有按什么排序的参数
	if (!(param & PARAM_t) && !(param & PARAM_S)) {
		qsort(file_name,fileCounts,sizeof(char) * MAX_FILE_LEN,str_cmp);
	}
	//此时已经进入了这些文件所在的文件夹可以在比较函数中用文件名获取文件大小
	else if (param & PARAM_S) {
		qsort(file_name,fileCounts,sizeof(char) * MAX_FILE_LEN,size_cmp);
	}
	else if (param & PARAM_t) {
		qsort(file_name,fileCounts,sizeof(char) * MAX_FILE_LEN,time_cmp);
	}

	//如果有R就要显示当前文件夹的名字
	if ((param & PARAM_R) && (param & PARAM_a)) {
		printf("%s:\n",path);
	} else if ((param & PARAM_R) && !(param & PARAM_a) && (hideFileCounts < fileCounts)) {
		printf("%s:\n",path);
	}

	//把当前文件夹下的所有文件都当作文件先显示出来
	int flag = 0;//标记有没有已经显示了一行
	for (i = 0;i< fileCounts;i++) {
		if (!(param & PARAM_a) && (file_name[i][0] == '.')) {
			continue;
		}
		
		//有l参数就要先显示当前文件夹下面的文件总数
		if ((param & PARAM_l) && !flag) {
			if (!(param & PARAM_a)) {
				printf("文件总数 %d\n",fileCounts - hideFileCounts);
			} else {
				printf("文件总数 %d\n",fileCounts);
			}
			flag = 1;
		}
		
		display_file(param,file_name[i],max_filename);
	}

	if (param & PARAM_R) {
		for (i = 0;i < fileCounts;i++) {
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

			//如果接下来要访问的目录是..或者.明显不能访问，会引起无限循环
			if (!strcmp(file_name[i],"..")) {
				continue;
			}
			if (!strcmp(file_name[i],".")) {
				continue;
			}

			strcpy(next_path,path);
			if (path[strlen(path) - 1] != '/') {
				strcat(next_path,"/");
			}
			available_char = MAX_LINE_LEN;			
			strcat(next_path,file_name[i]);
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

void display_file(int param,const char *filename,int max_filename) {

	int len = 0,i = 0;

	if (param & PARAM_l) {
		display_single(param,filename);
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

		//每列的宽度都是“待打印的文件名中最长字符数+SPACE_COUNT个空格”
		//这个循环是打印空格，使这一列的宽度够上面计算出的这么宽
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

void display_single(int param,const char *filename) {
	struct stat buf;
	struct passwd *psd;
	struct group *grp;

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
	//SUID
	if (S_ISUID & buf.st_mode) {
		printf("s");
	} else {
		if (S_IXUSR & buf.st_mode) {
			printf("x");
		} else {
			printf("-");
		}
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
	//SGID
	if (S_ISGID & buf.st_mode) {
		printf("s");
	} else {
		if (S_IXGRP & buf.st_mode) {
			printf("x");
		} else {
			printf("-");
		}
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
	//sticky
	if (S_ISVTX & buf.st_mode) {
		printf("t");
	} else {
		if (S_IXOTH & buf.st_mode) {
			printf("x");
		} else {
			printf("-");
		}
	}

	printf("%3d",buf.st_nlink);
	psd = getpwuid(buf.st_uid);
	grp = getgrgid(buf.st_gid);
	printf("%10s",psd->pw_name);
	printf(" ");
	printf("%10s",grp->gr_name);
	printf(" ");

	printf("%8d",buf.st_size);
	printf(" ");

	//显示最近访问时间
	if (param & PARAM_u) {
		//最后访问时间
		char atime[30];
		strcpy(atime,ctime(&buf.st_atime));
		atime[strlen(atime) - 1] = 0;
		printf("%s",atime);
		printf(" ");
	} else {
		//最近修改时间
		char mtime[30];
		strcpy(mtime,ctime(&buf.st_mtime));
		mtime[strlen(mtime) - 1] = 0;
		printf("%s",mtime);
		printf(" ");
	}

	printf("%s\n",filename);
}

int str_cmp(const void *a,const void *b) {
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

int time_cmp(const void *a,const void *b) {
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