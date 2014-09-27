#ifndef _SERVER_H_
#define _SERVER_H_

#define LISTEN_MAX	100
#define BLOCK 8192

void system_log(char msg[]);					//记录系统日志
void perror_log(char err[]);					//记录错误日志
int socket_init();
void do_cmd(int client_fd);
void do_cd(int client_fd,char *cmd);
void do_ls(int client_fd,char *cmd);
void do_put(int client_fd,char *cmd);
void do_get(int client_fd,char *cmd);
void do_mkdir(char *cmd);
int str_cmp(const void *a,const void *b);
int find_filename(char *name,char str[]);
void do_pwd(int client_fd);

#endif
