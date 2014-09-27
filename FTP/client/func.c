#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include "readline.h"
#include "client.h"
#include <dirent.h>
#include <sys/time.h>

const char LS[] = "ls";
const char CD[] = "cd";
const char GET[] = "get";
const char PUT[] = "put";
const char MKDIR[] = "mkdir";
const char PWD[] = "pwd";
const char ALIAS[] = "alias";
const char QUIT[] = "quit";

const char DOWN_DIR[] = "ftp_download";
const char ALIASFILE[] = ".ftprc";

struct alias alias_cmd[ALIAS_COUNT];	//最多100个命令别名
int alias_count = 0;	//存储了多少个别名

int socket_init()
{
	int sock_fd = 0;
	struct sockaddr_in serv_addr;
	char ip_add[20];

	printf("创建套接字...\n");
	sock_fd = socket(AF_INET,SOCK_STREAM,0);	//创建套接字
	if (sock_fd == -1) {
		perror("创建套接字出错.\n");
		exit(1);
	}
	memset(&serv_addr,0,sizeof(struct sockaddr_in));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(4500);
	printf("创建套接字完成.\n");

	printf("请输入FTP服务器IP地址：");
	fgets(ip_add,sizeof(ip_add),stdin);
	if (inet_aton(ip_add,&serv_addr.sin_addr) == -1) {
		perror("转换地址IP出错");
		exit(1);
	}
	if (connect(sock_fd,(struct sockaddr *)&serv_addr,sizeof(struct sockaddr_in)) == -1) {
		perror("连接服务器出错");
		exit(1);
	}

	return sock_fd;
}

void do_cd(int sock_fd,char *cmd)
{
	char recve[300];
	int cmd_length = 0;
	int recv_length = 0;

	cmd_length = strlen(cmd) + 1;
	send(sock_fd,&cmd_length,sizeof(cmd_length),0);
	send(sock_fd,cmd,cmd_length,0);

	recv(sock_fd,&recv_length,sizeof(recv_length),MSG_WAITALL);
	recv(sock_fd,recve,recv_length,MSG_WAITALL);
	puts(recve);
}
void do_ls(int sock_fd,char *cmd)
{
	char recve[8192];
	char *precv;
	char file_name_arr[300][300] = {0};		//存储send_file_name中的一个个文件名，排序
	int i = 0,j = 0;
	int cmd_length = 0;
	int recv_length = 0;

	cmd_length = strlen(cmd) + 1;
	send(sock_fd,&cmd_length,sizeof(cmd_length),0);
	send(sock_fd,cmd,cmd_length,0);		//按应该发送的长度发送

	recv(sock_fd,&recv_length,sizeof(recv_length),MSG_WAITALL);
	recv(sock_fd,recve,recv_length,MSG_WAITALL);	//按应该接收的长度接收

	i = 0;
	precv = strtok(recve," ");
	while (precv) {
		strcpy(file_name_arr[i++],precv);
		precv = strtok(NULL," ");
	}

	qsort(file_name_arr,i,sizeof(file_name_arr[0]),str_cmp);

	printf("%s",file_name_arr[0]);
	for (j = 1;j < i ;j++)
	{
		printf("    ");
		printf("%s",file_name_arr[j]);
	}
	printf("\n");
}
void do_put(int sock_fd,char *cmd)
{
	struct stat buf;
	struct timeval start,end;
	char *cmdcpy = (char *)malloc(strlen(cmd) + 1);
	char *every_cmd[100] = {0};	//一条命名按空格分开的命令
	char file_name_arr[300][300] = {0};
	char name_str[8192] = {0};
	char *tmp = NULL;
	char single_cmd[100] = {0};
	char file_block[BLOCK] = {0};
	int file_length = 0;		//即将发送的文件的长度
	int i = 0,j = 0,k = 0;
	int cmd_length = 0;		//即将发送的命令的长度
	int flag = 0;		//服务器返回的是否可以上传的判断
	int count = 0;		//一个命令上传的文件的个数
	int times = 0;
	int read_len = 0;
	int fd = 0;
	int speed = 0;		//上传速度
	int speed_print_count = 0;
	int time_print_count = 0; //剩余下载时间的打印字符数
	int send_already = 0;

	strcpy(cmdcpy,cmd);
	every_cmd[0] = strtok(cmdcpy," ");		//按空格分开
	while (every_cmd[i]) {
		every_cmd[++i] = strtok(NULL," ");
	}
	
	for (j = 1;j < i;j++)
	{
		count = find_filename(every_cmd[j],name_str);

		if (!count) {
			printf("对不起没有<%s>这个文件.\n",every_cmd[j]);
		}

		k = 0;
		tmp = strtok(name_str," ");
		while (tmp) {
			strcpy(file_name_arr[k++],tmp);
			tmp = strtok(NULL," ");
		}

		k = 0;
		//分别传送当前命令对应的所有文件
		while (count--) {
			strcpy(single_cmd,"put ");
			strcat(single_cmd,file_name_arr[k]);
			cmd_length = strlen(single_cmd) + 1;
			send(sock_fd,&cmd_length,sizeof(cmd_length),0);	//发送下面要发送的命令的长度
			send(sock_fd,single_cmd,cmd_length,0);	//发送一个put xxx到服务器
			recv(sock_fd,&flag,sizeof(flag),MSG_WAITALL);	//是否可以上传当前命令的文件

			//如果可以上传
			if (flag) {
				lstat(file_name_arr[k],&buf);
				file_length = buf.st_size;
				send(sock_fd,&file_length,sizeof(file_length),0);		//发送要上传文件的长度

				if ((fd = open(file_name_arr[k],O_RDONLY)) == -1) {
					perror("打开上传文件失败");
				}
				times = (file_length + BLOCK - 1) / BLOCK;
				printf("%s正在上传中,上传大小为%.2lfKB......\n",file_name_arr[k],(double)file_length / 1024);
				gettimeofday(&start,NULL);
				printf("上传速度和剩余时间：");
				speed = 0;
				while (times--) {
					read_len = read(fd,file_block,sizeof(file_block));
					send(sock_fd,&read_len,sizeof(read_len),0);
					send(sock_fd,file_block,read_len,0);
					speed+=read_len;
					gettimeofday(&end,NULL);
					if (end.tv_sec - start.tv_sec >= 1) {
						send_already+=speed;
						while(time_print_count--) {
							printf("\b \b");
						}
						while (speed_print_count--) {
							printf("\b \b");
						}
						speed_print_count = printf("%.2lfKB/S,",(double)speed / 1024);
						time_print_count = printf("%dS",(file_length - send_already) / speed);

						speed = 0;
						gettimeofday(&start,NULL);
					}
				}
				printf("\n%s上传完毕\n",file_name_arr[k]);

				close(fd);
			}	else {
				printf("对不起有<%s>的同名文件,请修改名称后重新上传.\n",file_name_arr[k]);
			}

			k++;
		}
	}

	free(cmdcpy);
}
void do_get(int sock_fd,char *cmd)
{
	struct stat buf;
	struct timeval start,end;
	char *every_cmd[100] = {0};		//最多100个命令这个地方如果输入的太多空格超过100个就会出错
	char *cmdcpy = (char *)malloc(strlen(cmd) + 1);
	char single_cmd[100] = {0};
	char new_file[300] = {0};			//存储下载的文件的绝对路径和文件名
	char file_block[BLOCK] = {0};
	char recv_filename[300] = {0};
	char continue_down = 'n';	//是否断点续传
	int filename_length = 0;
	int i = 0,j = 0;
	int file_length = 0,times = 0;
	int flag = 0;		//服务器返回的是否可以下载的判断
	int fd = 0;
	int count = 0;		//一个get xxx命令需要下载的文件个数
	int should_recv_length = 0;	//标记当次recv文件的长度
	int true_recv_length = 0;
	int cmd_length = 0;		//即将发送的命令的长度
	int start_length = 0;	//文件开始下载的长度（为了支持断点续传）
	int speed = 0;		//下载速度
	int speed_print_count = 0;
	int time_print_count = 0; //剩余下载时间的打印字符数
	int recv_already = 0;

	strcpy(cmdcpy,cmd);
	every_cmd[0] = strtok(cmdcpy," ");		//按空格分开
	while (every_cmd[i]) {
		every_cmd[++i] = strtok(NULL," ");
	}
	
	for (j = 1;j < i;j++)
	{
		flag = 0;
		strcpy(single_cmd,"get ");
		strcat(single_cmd,every_cmd[j]);
		cmd_length = strlen(single_cmd) + 1;
		send(sock_fd,&cmd_length,sizeof(cmd_length),0);		//发送下面要发送的命令的长度
		send(sock_fd,single_cmd,cmd_length,0);		//发送一个get xxx到服务器
		recv(sock_fd,&flag,sizeof(flag),MSG_WAITALL);			//是否可以下载当前命令的文件

		//如果可以下载
		if (flag) {
			//当前命令需要下载几个文件，如get *就要下载所有文件
			recv(sock_fd,&count,sizeof(count),MSG_WAITALL);	
			while (count--) {
				recv(sock_fd,&file_length,sizeof(file_length),MSG_WAITALL);			//接收文件大小

				recv(sock_fd,&filename_length,sizeof(filename_length),MSG_WAITALL);//接收文件名的长度包括/0
				recv(sock_fd,recv_filename,filename_length,MSG_WAITALL);	//按长度接收文件名不会粘包

				mkdir(DOWN_DIR,0777);		//每次都创建确保程序每次下载都有这个目录
				strcpy(new_file,DOWN_DIR);
				strcat(new_file,"/");
				strcat(new_file,recv_filename);

				//获取本地文件大小（如果有）
				if (lstat(new_file,&buf) == -1) {
					start_length = 0;
				}	else {
					printf("本地有<%s>同名文件，是否开启断点续传？(否定将重命名下载)",recv_filename);
					continue_down = getchar();
					
					if (continue_down == 'Y' || continue_down == 'y') {
						start_length = buf.st_size;
					}	else {
						start_length = 0;

						strcat(new_file,"1");	//重命名就是加一个1
						while (lstat(new_file,&buf) != -1) {
							strcat(new_file,"1");	//还有重明就继续加1
						}
					}
				}

				send(sock_fd,&start_length,sizeof(start_length),0);//发送开始位置（服务器文件指针移动距离）
				times = ((file_length - start_length) + BLOCK - 1) / BLOCK;

				if ((fd = open(new_file,O_WRONLY|O_CREAT|O_APPEND,0664)) == -1) {
					perror("创建下载文件出错");
					return ;
				}
				printf("%s正在下载中,下载大小为%.2lfKB......\n",recv_filename,(double)(file_length - start_length) / 1024);
				gettimeofday(&start,NULL);
				printf("下载速度和剩余时间：");
				speed = 0;
				while (times-- > 0) {
					//接收此次要接收的长度
					recv(sock_fd,&should_recv_length,sizeof(should_recv_length),MSG_WAITALL);	
					//接受具体文件
					true_recv_length = recv(sock_fd,file_block,should_recv_length,MSG_WAITALL);
					speed+=true_recv_length;
					gettimeofday(&end,NULL);
					if (end.tv_sec - start.tv_sec >= 1) {
						recv_already+=speed;
						while (time_print_count--) {
							printf("\b \b");
						}
						while (speed_print_count--) {
							printf("\b \b");
						}
						speed_print_count = printf("%.2lfKB/S,",(double)speed / 1024);
						time_print_count = printf("%dS",(file_length - start_length - recv_already) / speed);
						speed = 0;
						gettimeofday(&start,NULL);
					}
					write(fd,file_block,true_recv_length);
				}

				printf("\n%s下载完毕\n",recv_filename);
				close(fd);
			}
		}	else {
			printf("不存在%s文件或者它是一个目录\n",every_cmd[j]);
		}
	}

	free(cmdcpy);	//父进程和子进程这个地址打印出来是一样的，但是那是虚拟地址，物理地址还是不一样的
}

void do_pwd(int sock_fd,char *cmd)
{
	char pwd[300];
	int cmd_length = strlen(cmd) + 1;

	send(sock_fd,&cmd_length,sizeof(cmd_length),0);
	send(sock_fd,cmd,cmd_length,0);

	recv(sock_fd,pwd,sizeof(pwd),MSG_WAITALL);
	puts(pwd);
}

void do_mkdir(int sock_fd,char *cmd)
{
	int cmd_length = 0;

	cmd_length = strlen(cmd) + 1;
	send(sock_fd,&cmd_length,sizeof(sock_fd),0);
	send(sock_fd,cmd,cmd_length,0);
}

int do_alias(char *cmd)
{
	char left[50] = {0},right[50] = {0};
	char *tmp = NULL;
	int fd = 0;
	int i = 0;

	tmp = strtok(cmd,"=");
	strcpy(left,tmp + strlen(ALIAS) + 1);

	tmp = strtok(NULL,"=");
	if (!tmp) {
		return 0;
	}
	strcpy(right,tmp);
	
	if (!strcmp(left,right)) {
		return 0;
	}

	//不能对应两个命令
	for (i = 0;i < alias_count;i++)
	{
		if (!strcmp(left,alias_cmd[i].left)) {
			return 0;
		}
        if (!strcmp(left,alias_cmd[i].right) && !strcmp(right,alias_cmd[i].left)) {
            return 0;
        }
	}

	fd = open(ALIASFILE,O_WRONLY|O_CREAT|O_APPEND,0664);
	write(fd,left,strlen(left));
	write(fd,"=",strlen("="));
	write(fd,right,strlen(right));
	write(fd,":",strlen(":"));	//:分开,类似环境变量

	strcpy(alias_cmd[alias_count].left,left);
	strcpy(alias_cmd[alias_count].right,right);
	alias_count++;

	return 1;
}

void cmd_init(struct alias cmd[],int count)
{
	char tmp[2048];
	char *q1 = NULL,*q2 = NULL;
	int fd = 0;

	if ((fd = open(ALIASFILE,O_RDONLY)) == -1) {
		return ;
	}
	if (!read(fd,tmp,sizeof(tmp))) {
		return ;
	}

	q1 = strtok(tmp,":");
	while (q1 && (alias_count < count)) {
		strcpy(alias_cmd[alias_count].left,q1);
		q2 = q1;
		while (*q2 != '=') {
			q2++;
		}
		*(alias_cmd[alias_count].left + (q2 - q1)) = 0;
		strcpy(alias_cmd[alias_count].right,q2 + 1);
		q1 = strtok(NULL,":");
		alias_count++;
	}
}

void do_cmd(int sock_fd)
{
	char *cmd = NULL,first_cmd[100] = {0},*q_cmd = NULL;
	char err_msg[] = "命令有误.";
	char cmd_tmp[1024];
	int flag1 = 0;	//标记是否有重命名
	int i = 0;

	initialize_readline();
	cmd_init(alias_cmd,ALIAS_COUNT);
	signal(SIGCHLD,SIG_IGN);
	setbuf(stdout,NULL);

	while (1) {
		cmd = getl();

        q_cmd = cmd;
        flag1 = 0;
        memset(first_cmd,0,sizeof(first_cmd));
        while (*q_cmd && *q_cmd != ' ') {
            strncat(first_cmd,q_cmd,1);
            q_cmd++;
        }
        strcpy(cmd_tmp,q_cmd);
        
        do{
            flag1 = 0;
            for (i = 0;i < alias_count;i++)
            {
                if (!strcmp(first_cmd,alias_cmd[i].left)) {
                    strcpy(first_cmd,alias_cmd[i].right);
                    flag1 = 1;
                    break;
                }
            }
        } while(flag1);
        strcpy(cmd,first_cmd);
        strcat(cmd,cmd_tmp);

		if (!strncmp(first_cmd,CD,strlen(CD))) {
				do_cd(sock_fd,cmd);
		}	else if (!strncmp(first_cmd,LS,strlen(LS))) {
				do_ls(sock_fd,cmd);
		}	else if (!strncmp(first_cmd,GET,strlen(GET)) && strlen(cmd) != strlen(GET)) {
				do_get(sock_fd,cmd);
		}	else if (!strncmp(first_cmd,PUT,strlen(PUT)) && strlen(cmd) != strlen(PUT)) {
				do_put(sock_fd,cmd);
		}	else if (!strncmp(first_cmd,MKDIR,strlen(MKDIR)) && strlen(cmd) != strlen(MKDIR)) {
				do_mkdir(sock_fd,cmd);
		}	else if (!strncmp(first_cmd,PWD,strlen(PWD))) {
				do_pwd(sock_fd,cmd);
		}	else if (!strncmp(first_cmd,ALIAS,strlen(ALIAS)) && strlen(cmd) != strlen(ALIAS)) {
				if (!do_alias(cmd)) {
					puts(err_msg);
				}
		}	else if (!strncmp(first_cmd,QUIT,strlen(QUIT))) {
				send(sock_fd,QUIT,strlen(QUIT) + 1,0);
				close(sock_fd);
				exit(0);
		}	else {
				puts(err_msg);
		}
	}
}

int str_cmp(const void *a,const void *b)
{
	return strcmp((char *)a,(char *)b);
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
		perror("打开文件夹错误");
		return 0;
	}

	while (pdr = readdir(dir)) {
		if (lstat(pdr->d_name,&buf) == -1) {
			perror("获取文件属性出错");
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
