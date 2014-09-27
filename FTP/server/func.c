#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "server.h"
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <dirent.h>

extern char ROOT_DIR[];
extern char LS[];
extern char CD[];
extern char GET[];
extern char PUT[];
extern char MKDIR[];
extern char PWD[];
char QUIT[] = "quit";

void system_log(char *msg)
{
	struct flock lock;
	int fd = 0;
	char filename[50] = {0};
	time_t time_now;
	char time_str[30] = {0};

	strcpy(filename,ROOT_DIR);
	strcat(filename,"/Log/system_log.txt");
	fd = open(filename,O_WRONLY|O_CREAT|O_APPEND,0664);

	memset(&lock,0,sizeof(struct flock));
	lock.l_len = 0;
	lock.l_start = SEEK_SET;
	lock.l_whence = 0;
	lock.l_type = F_WRLCK;

	fcntl(fd,F_SETLKW,&lock);
	time(&time_now);
	strcpy(time_str,ctime(&time_now));
	time_str[strlen(time_str) - 1] = 0;
	strcat(time_str,":");
	write(fd,time_str,strlen(time_str));
	write(fd,msg,strlen(msg));
	write(fd,"\n",strlen("\n"));
	lock.l_type = F_UNLCK;
	fcntl(fd,F_SETLKW,&lock);

	close(fd);
}

void perror_log(char *err)
{
	struct flock lock;
	int fd = 0;
	char filename[50] = {0};
	time_t time_now;
	char time_str[30] = {0};

	strcpy(filename,ROOT_DIR);
	strcat(filename,"/Log/error_log.txt");
	fd = open(filename,O_WRONLY|O_CREAT|O_APPEND,0664);

	memset(&lock,0,sizeof(struct flock));
	lock.l_len = 0;
	lock.l_start = 0;
	lock.l_whence = SEEK_SET;
	lock.l_type = F_WRLCK;

	fcntl(fd,F_SETLKW,&lock);
	time(&time_now);
	strcpy(time_str,ctime(&time_now));
	time_str[strlen(time_str) - 1] = 0;
	strcat(time_str,":\t");
	write(fd,time_str,strlen(time_str));
	write(fd,err,strlen(err));
	write(fd,"\n",strlen("\n"));
	lock.l_type = F_UNLCK;
	fcntl(fd,F_SETLKW,&lock);

	close(fd);
}

void my_err(char *strerr,int line)
{
	char str_line[100];

	sprintf(str_line,"%d",line);
	strcat(str_line,"行,");
	strcat(str_line,strerr);
	strcat(str_line,strerror(errno));

	perror(strerr);
	perror_log(str_line);
}

int socket_init()
{
	int sock_fd = 0,option_value = 1;
	struct sockaddr_in serv_addr;
	char log_dir[50] = {0};

	strcpy(log_dir,ROOT_DIR);
	strcat(log_dir,"/Log");
	mkdir(ROOT_DIR,0777);
	mkdir(log_dir,0777);

	system_log("创建套接字");
	printf("创建套接字...\n");
	sock_fd = socket(AF_INET,SOCK_STREAM,0);	//创建套接字
	if (sock_fd == -1) {
		my_err("创建套接字出错",__LINE__);
		exit(1);
	}
	setsockopt(sock_fd,SOL_SOCKET,SO_REUSEADDR,&option_value,sizeof(option_value));
	//一定时间没有数据传输就关闭这个连接
	setsockopt(sock_fd,SOL_SOCKET,SO_KEEPALIVE,&option_value,sizeof(option_value));
	printf("创建套接字完成.\n");
	system_log("创建套接字完成");

	//绑定端口
	system_log("绑定端口");
	printf("绑定端口...\n");
	memset(&serv_addr,0,sizeof(struct sockaddr_in));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(4500);
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(sock_fd,(struct sockaddr *)&serv_addr,sizeof(struct sockaddr_in)) == -1) {
		my_err("绑定端口出错",__LINE__);
		close(sock_fd);
		exit(1);
	}
	printf("绑定端口完成.\n");
	system_log("绑定端口完成");

	//监听端口
	system_log("监听端口");
	printf("监听端口...\n");
	if (listen(sock_fd,LISTEN_MAX) == -1) {
		my_err("监听端口出错",__LINE__);
		close(sock_fd);
		exit(1);
	}
	printf("监听端口完成.\n");
	system_log("监听端口完成");

	return sock_fd;
}

void do_cd(int client_fd,char *cmd)
{
	char origin_path[300];
	char tmp[300] = {0};
	char err_msg[] = "目录非法";
	char sys_err[] = "服务器已退出.";
	char success_msg[] = "转换工作目录成功！";
	int flag = 1;
	int send_length = 0;

	if (getcwd(origin_path,sizeof(origin_path)) == NULL) {
		send_length = strlen(sys_err) + 1;
		send(client_fd,&send_length,sizeof(send_length),0);
		send(client_fd,sys_err,send_length,0);
		my_err("获取当前目录出错",__LINE__);
		close(client_fd);
		exit(1);
	}

	//只有cd就回到ftp根目录
	if (strlen(cmd) == strlen(CD)) {
		if (chdir(ROOT_DIR) == -1) {
			my_err("切换工作目录出错",__LINE__);
			send_length = strlen(sys_err) + 1;
			send(client_fd,&send_length,sizeof(send_length),0);
			send(client_fd,sys_err,send_length,0);
			close(client_fd);
			exit(1);
		}	else {
			send_length = strlen(success_msg) + 1;
			send(client_fd,&send_length,sizeof(send_length),0);
			send(client_fd,success_msg,send_length,0);
		}
		return ;
	}

	//先转到用户指定的目录
	if (chdir(cmd + strlen(CD) + 1) == -1) {
		flag = 0;
		my_err("切换工作目录出错",__LINE__);
	}

	//在前面转换成功的条件下，如果此目录超出ftp目录范围，则要转换回原来的文件夹
	if (flag && strncmp(getcwd(NULL,0),ROOT_DIR,strlen(ROOT_DIR))) {
		if (chdir(origin_path) == -1) {
			my_err("切换工作目录出错",__LINE__);
		}
		flag = 0;
	}

	if (flag) {
		send_length = strlen(success_msg) + 1;
		send(client_fd,&send_length,sizeof(send_length),0);
		send(client_fd,success_msg,send_length,0);
		system_log("切换工作目录");
	}	else {
		send_length = strlen(err_msg) + 1;
		send(client_fd,&send_length,sizeof(send_length),0);
		send(client_fd,err_msg,send_length,0);
	}
}

void do_ls(int client_fd,char *cmd)
{
	char send_file_name[8192];
	char origin_path[300];
	char next_path[300];
	char err_msg[] = "目录非法";
	char sys_err[] = "服务器已退出.";
	struct dirent *pdr;
	struct stat buf;
	DIR *dir;
	int flag = 1;
	int send_length = 0;

	if (getcwd(origin_path,sizeof(origin_path)) == NULL) {
		my_err("获取当前目录出错",__LINE__);
		send_length = strlen(sys_err) + 1;
		send(client_fd,&send_length,sizeof(send_length),0);
		send(client_fd,sys_err,send_length,0);
		close(client_fd);
		exit(1);
	}

	//只有ls就显示当前目录
	if (strlen(cmd) == strlen(LS)) {
		strcpy(next_path,".");
	}	else {	//不是只有ls就要去显示哪个文件夹的内容
		strcpy(next_path,cmd + strlen(LS) + 1);
	}

	if (flag && (dir = opendir(next_path)) == NULL) {
		my_err("打开文件夹错误",__LINE__);
		flag = 0;
	}

	if (flag && chdir(next_path) == -1) {
		my_err("切换工作目录出错",__LINE__);
		flag = 0;
	}

	//在前面转换成功的条件下，如果此目录超出ftp目录范围，则要转换回原来的文件夹
	if (flag && strncmp(getcwd(NULL,0),ROOT_DIR,strlen(ROOT_DIR))) {
		if (chdir(origin_path) == -1) {
			my_err("切换工作目录出错",__LINE__);
		}
		flag = 0;
	}

	memset(send_file_name,0,sizeof(send_file_name));
	//当发现了此文夹下的文件后
	while (flag && (pdr = readdir(dir))) {
		if (lstat(pdr->d_name,&buf) == -1) {
			my_err("获取文件属性出错",__LINE__);
			send_length = strlen(sys_err) + 1;
			send(client_fd,&send_length,sizeof(send_length),0);
			send(client_fd,sys_err,send_length,0);
			close(client_fd);
			exit(1);
		}

		if (S_ISDIR(buf.st_mode)) {
			strcat(send_file_name,"<");
			strcat(send_file_name,pdr->d_name);
			strcat(send_file_name,"> ");
		}	else {
			strcat(send_file_name,pdr->d_name);
			strcat(send_file_name," ");
		}
	}

	if (chdir(origin_path) == -1) {
		my_err("切换工作目录出错",__LINE__);
		send_length = strlen(sys_err) + 1;
		send(client_fd,&send_length,sizeof(send_length),0);
		send(client_fd,sys_err,send_length,0);
		close(client_fd);
		exit(1);
	}

	if (flag) {
		send_length = strlen(send_file_name) + 1;
		send(client_fd,&send_length,sizeof(send_length),0);
		send(client_fd,send_file_name,send_length,0);
		system_log("ls查看文件");
	}	else {
		send_length = strlen(err_msg) + 1;
		send(client_fd,&send_length,sizeof(send_length),0);
		send(client_fd,err_msg,send_length,0);
	}
}

void do_put(int client_fd,char *cmd)
{
	char file_name[300] = {0};
	char file_block[BLOCK] = {0};
	struct stat buf;
	int flag = 0;
	int file_length = 0;
	int times = 0;
	int fd = 0;
	int should_recv_length = 0;
	int true_recv_length = 0;

	strcpy(file_name,cmd + strlen(PUT) + 1);
	if (lstat(file_name,&buf) != -1) {
		flag = 0;
		send(client_fd,&flag,sizeof(flag),0);
		return ;
	}	else {
		flag = 1;
		send(client_fd,&flag,sizeof(flag),0);
	}

	recv(client_fd,&file_length,sizeof(file_length),MSG_WAITALL);
	times = (file_length + BLOCK - 1) / BLOCK;

	if ((fd = open(file_name,O_WRONLY|O_CREAT,0664)) == -1) {
		my_err("创建用户上传文件出错",__LINE__);
		return ;
	}

	while (times--) {
		recv(client_fd,&should_recv_length,sizeof(should_recv_length),MSG_WAITALL);	//接收此次要接收的长度
		true_recv_length = recv(client_fd,file_block,should_recv_length,MSG_WAITALL);		//接受具体文件
		write(fd,file_block,true_recv_length);
	}

	close(fd);
	system_log("上传文件");
}

void do_get(int client_fd,char *cmd)
{
	struct stat buf;
	char file_name[300] = {0};	//下载的文件的名字
	char file_block[BLOCK] = {0};	//每次获取的文夹大小
	char all_filename[300][300];	//当前目录下的所有文件的名字
	char name_str[8192];
	char *tmp = NULL;
	int times = 0,j = 0;
	int filename_length = 0;
	int file_length = 0;
	int count = 0;		//此次一共要发送的文件个数
	int fd = 0;
	int read_len = 0;
	int flag = 0;		//标记是否可以发送
	int start_length = 0;	//下载开始位置

	strcpy(file_name,cmd + strlen(GET) + 1);

	count = find_filename(file_name,name_str);

	if (count) {
		flag = 1;
		send(client_fd,&flag,sizeof(flag),0);	//发送可以接收文件的信息
	}	else {
		flag = 0;
		send(client_fd,&flag,sizeof(flag),0);
		return ;
	}
	send(client_fd,&count,sizeof(count),0);		//发送发送文件个数的信息

	j = 0;
	tmp = strtok(name_str," ");
	while (tmp) {
		strcpy(all_filename[j++],tmp);
		tmp = strtok(NULL," ");
	}

	j = count - 1;
	while (count--) {
		if (lstat(all_filename[j],&buf) == -1) {
			my_err("获取文件属性出错",__LINE__);
			exit(1);	//到达这里说明文件存在，如果无法查看属性说明有问题，必须退出
		}
		file_length = buf.st_size;
		send(client_fd,&file_length,sizeof(file_length),0);		//发送文件大小
		filename_length = strlen(all_filename[j]) + 1;
		send(client_fd,&filename_length,sizeof(filename_length),0);		//发送文件名长度包括/0
		send(client_fd,all_filename[j],filename_length,0);		//发送文件名
		recv(client_fd,&start_length,sizeof(start_length),MSG_WAITALL);

		times = ((file_length - start_length) + BLOCK - 1) / BLOCK;

		if ((fd = open(all_filename[j],O_RDONLY)) == -1) {
			my_err("打开下载文件出错",__LINE__);
			close(client_fd);
			exit(1);
		}
		
		lseek(fd,start_length,SEEK_SET);
		while (times-- > 0) {
			read_len = read(fd,file_block,sizeof(file_block));
			send(client_fd,&read_len,sizeof(read_len),0);
			send(client_fd,file_block,read_len,0);
		}

		j--;
		close(fd);
	}
	system_log("下载文件");
}

void do_pwd(int client_fd)
{
	char pwd[300];
	char *tmp;

	getcwd(pwd,sizeof(pwd));
	tmp = pwd + strlen(ROOT_DIR);
	strcpy(pwd,"~");
	strcat(pwd,tmp);

	send(client_fd,pwd,sizeof(pwd),0);
	system_log("查看当前目录");
}

void do_mkdir(char *cmd)
{
	if (mkdir(cmd + strlen(MKDIR) + 1,0777) != -1)
		system_log("创建文件夹");
}
void do_cmd(int client_fd)
{
	char cmd[1024],*q_cmd = NULL,tmp[1024];
	int cmd_length = 0;		//即将收的命令的长度,包括/0(字符串都是要包括/0的长度)


	while (1) {
		recv(client_fd,&cmd_length,sizeof(cmd_length),MSG_WAITALL);
		if (recv(client_fd,cmd,cmd_length,MSG_WAITALL) == -1) {
			my_err("接收命令出错",__LINE__);
			close(client_fd);
			exit(1);
		}

		//~代表ftp根目录
		if (q_cmd = strstr(cmd,"~")) {
			strcpy(tmp,q_cmd + 1);
			strcpy(q_cmd,ROOT_DIR);
			strcat(q_cmd,tmp);
		}

		if (!strncmp(cmd,CD,strlen(CD))) {
				do_cd(client_fd,cmd);
		}	else if (!strncmp(cmd,LS,strlen(LS))) {
				do_ls(client_fd,cmd);
		}	else if (!strncmp(cmd,GET,strlen(GET))) {
				do_get(client_fd,cmd);
		}	else if (!strncmp(cmd,PUT,strlen(PUT))) {
				do_put(client_fd,cmd);
		}	else if (!strncmp(cmd,MKDIR,strlen(MKDIR))) {
				do_mkdir(cmd);
		}	else if (!strncmp(cmd,PWD,strlen(PWD))) {
				do_pwd(client_fd);
		}	else if (!strncmp(cmd,QUIT,strlen(QUIT))) {
				system_log("客户端退出");
				close(client_fd);
				exit(0);
		}
	}
}

int find_filename(char *name,char str[])
{
	struct stat buf;
	DIR *dir;
	struct dirent *pdr;
	int count = 0;
	char tmp1[300] = {0},tmp2[300] = {0};
	char temp = '0';
	int j = 0,k = 0;

	memset(str,0,sizeof(str));
	if ((name[0] != '*') && (name[strlen(name) - 1]) != '*') {
		if (lstat(name,&buf) == -1 || S_ISDIR(buf.st_mode)) {
			return 0;
		}
		strcpy(str,name);
		return 1;
	}

	if ((dir = opendir(".")) == NULL) {
		my_err("打开文件夹错误",__LINE__);
		return 0;
	}

	while (pdr = readdir(dir)) {
		if (lstat(pdr->d_name,&buf) == -1) {
			my_err("获取文件属性出错",__LINE__);
			return 0;
		}

		if (!S_ISDIR(buf.st_mode)) {
			//如果只是一个*
			if (strlen(name) == 1) {
				strcat(str,pdr->d_name);
				strcat(str," ");
				count++;
			}	else if (name[strlen(name) - 1] == '*') {
				if (!strncmp(pdr->d_name,name,strlen(name) - 1)) {
					strcat(str,pdr->d_name);
					strcat(str," ");
					count++;
				}
			}	else {
				strcpy(tmp1,pdr->d_name);
				strcpy(tmp2,name);
				//逆置比较后缀
				for (k = 0,j = strlen(tmp1) - 1;k < j;k++,j--)
				{
					temp = tmp1[k];
					tmp1[k] = tmp1[j];
					tmp1[j] = temp;
				}
				for (k = 0,j = strlen(tmp2) - 1;k < j;k++,j--)
				{
					temp = tmp2[k];
					tmp2[k] = tmp2[j];
					tmp2[j] = temp;
				}

				if (!strncmp(tmp1,tmp2,strlen(tmp2) - 1)) {
					strcat(str,pdr->d_name);
					strcat(str," ");
					count++;
				}
			}
		}
	}
	return count;
}



















