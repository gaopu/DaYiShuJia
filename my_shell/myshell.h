#ifndef MYSHELL_H_
#define MYSHELL_H_

#define CMD_COUNT		100		//一条命令中最多100个分开的单词
#define	CMD_LEN			512		//一条命令最大512个字符
#define GENERAL			0		//普通的
#define REDIR_IN		1		//	<
#define REDIR_OUT		2		//	>
#define PIPE			3		//	|
#define READ			4		//管道的模式
#define WRITE			5		//管道的模式
//#define BACKGND			4		//	&

int split(char *cmd,char arglist[][CMD_LEN]);		
void print();							//输出提示语句
int judge(char arglist[][CMD_LEN],int *cmd_count,int *backgnd);
	//命令正确返回有的特殊字符例如><|对应的数字，不正确返回-1，正确时要添加NULL结尾
void do_cmd(int how,int backgnd,char arglist[][CMD_LEN],int cmd_count);	//根据backgnd判断是否后台运行
void excute_pipe(char *arg[],int cmd_count,int mod);

#endif
