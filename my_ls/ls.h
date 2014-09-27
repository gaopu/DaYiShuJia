/*
 * =====================================================================================
 *
 *       Filename:  ls.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2014年07月15日 15时31分16秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  geekgao, gaopu333@gmail.com
 *        Company:  Class 1302 of Software Engineer
 *
 * =====================================================================================
 */
#ifndef LS_H_
#define LS_H_

//宏
#define PARAM_NONE			0
#define PARAM_a 			1
#define PARAM_l				2
#define PARAM_R				4
#define PARAM_S				8
#define PARAM_t				16

#define MAX_FILE			500
#define MAX_FILE_LEN		300
#define MAX_LINE_LEN		100
#define SPACE_COUNT			2		//打印最长文件名后面跟的空格数

//函数
void display(int param,const char *path);
		//提供给display()的地址必须是绝对路径
void display_dir(int param,const char *path);
void display_file(int param,const char *filename,int max_filename);
void display_single(const char *filename);
void my_err(char *strerr,int line);
int str_cmp(const void *a,const void *b);
int size_cmp(const void *a,const void *b);
int time_cmp(const void *a,const void *b);
#endif
