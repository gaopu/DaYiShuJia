#ifndef _CLIENT_H_
#define _CLIENT_H_

#define BLOCK	8192
#define ALIAS_COUNT 100

struct alias
{
	char left[50];
	char right[50];
};

int socket_init();
void cmd_init(struct alias cmd[],int count);
void do_cmd(int sock_fd);
void do_cd(int sock_fd,char *cmd);
void do_ls(int sock_fd,char *cmd);
void do_put(int sock_fd,char *cmd);
void do_get(int sock_fd,char *cmd);
void do_mkdir(int sock_fd,char *cmd);
void do_pwd(int sock_fd,char *cmd);
int do_alias(char *cmd);	//返回值标记是否成功
int str_cmp(const void *a,const void *b);
int find_filename(char *name,char str[]);

#endif
