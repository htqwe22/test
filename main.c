#define DEBUG
#include "util.h"
#include "ftp.h"
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <getopt.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>

struct ftp_client client;

static int loop_exit ;
typedef int (*exec_func)(struct ftp_client *client, int argc ,char **argv);

struct cmd_func
{
	const char *cmd;
	exec_func func;
};



void signal_handler(int sig) 
{
	loop_exit = 1;
}

int poll_sock(int fd, int timeout_ms);


int ftp_recv(int fd,char *buff, int size,int timeout_ms)
{
	int	ret = poll_sock(fd, timeout_ms);
	if (ret >0){
		ret = recv(fd, buff, size -1,0);
		if (ret >0){
			buff[ret-1] = '\0';
		}
	}else{
		ebug("server unreachable\r\n");
	}
	return ret;
}

int ftp_send(int fd,char *buff, int size)
{
	return send(fd,buff,size,0);
}


int exec_cd(struct ftp_client *client, int argc ,char **argv)
{
	char buff[1024];
	int ret = sprintf(buff,"CWD %s\r\n",argv[1]);
	ftp_send(client->fd, buff,ret);
	ret = ftp_recv(client->fd, buff,sizeof(buff), 2000);
	if (ret > 0){
		debug("S>C:%s\r\n",buff);
	}
	return 0;
}

int exec_type(struct ftp_client *client, int argc ,char **argv)
{
	char buff[1024];
	int ret;
	/*default binary or ascii as input 'A'*/
	char type = 'I';
	if (argc > 1)
		type = argv[1][0];
	ret = sprintf(buff,"TYPE %c\r\n",type);
	ftp_send(client->fd, buff,ret);
	ret = ftp_recv(client->fd, buff,sizeof(buff), 2000);
	if (ret > 0){
		debug("S>C:%s\r\n",buff);
	}
	return 0;
}


int exec_size(struct ftp_client *client, int argc ,char **argv)
{
	char buff[1024];
	int ret;
	if(argc < 2){
		red_bug("you should use %s <file name> to create one directory\r\n",argv[0]);
		return -1;
	}

	ret = sprintf(buff,"SIZE %s\r\n",argv[1]);
	ftp_send(client->fd, buff,ret);
	ret = ftp_recv(client->fd, buff,sizeof(buff), 2000);
	if (ret > 0){
		debug("S>C:%s\r\n",buff);
		sscanf(buff,"213 %d",&ret);
	}
	return ret;
}


int exec_pasv(struct ftp_client *client, int argc ,char **argv)
{
	char buff[1024];
	char *p_ip;
	int ip[4];
	int p1 = 0,p2 = 0;
	int ret = sprintf(buff,"PASV\r\n");
	ftp_send(client->fd, buff,ret);
	ret = ftp_recv(client->fd, buff,sizeof(buff), 2000);
	if(ret <= 0)
		return -1;
	debug("S>C:%s\r\n",buff);
	p_ip = strrchr(buff, '(');
	if (p_ip == NULL)
		return -1;
	p_ip += 1;
	printf("p_ip is %s\r\n",p_ip);
	sscanf(p_ip,"%d,%d,%d,%d,%d,%d",ip,ip+1,ip+2,ip+3,&p1,&p2);
	debug("p1 = %d,p2 = %d\r\n",p1,p2);
	return (p1<<8) | p2;
}


int exec_ls(struct ftp_client *client, int argc ,char **argv)
{
	char buff[1024];
	char *path = ".";

	FILE *p;
	int fd1,ret;
	int int_port;
	char port[8];

	if (argc > 1){
		path = argv[1];
	}

	int_port = exec_pasv(client,argc,argv);
	if (port <= 0){
		return -1;
	}
	sprintf(port,"%d", int_port);
	fd1 = connet_server(client->server,port);
	if (fd1 <= 0)
	{
		perror("connect new");
		return -1;
	}
	p = fdopen(fd1, "r");
	/*send list*/
	ret = sprintf(buff,"LIST %s\r\n",path);
	ftp_send(client->fd, buff,ret);

	/*receive answer*/
	while (fgets(buff, sizeof(buff), p) != NULL ) {
		strtok(buff, "\n");

		/* dir*/
		if ( buff[0] == 0x64 )
			printf("%s%s%s\n", BLUE,buff, END);
		/* symlink*/
		if ( buff[0] == 0x6c )
			printf("%s%s%s\n", CYN,buff,END);
		/* File */
		else if ( buff[0] == 0x2d )
			printf("%s%s%s\n", YEL,buff, END);
	}

	fflush(stdout);
	printf(WHT END);
	ret = ftp_recv(client->fd, buff,sizeof(buff), 1000);
	if (ret >0){
		debug("S>C:%s\r\n",buff);
	}
	ret = ftp_recv(client->fd, buff,sizeof(buff), 1000);
	if (ret >0){
		debug("S>C:%s\r\n",buff);
	}
	close(fd1);
	fclose(p);
	return 0;
}


int exec_pwd(struct ftp_client *client, int argc ,char **argv)
{
	char buff[1024];
	int ret = sprintf(buff,"PWD\r\n");
	ftp_send(client->fd, buff,ret);
	ret = ftp_recv(client->fd, buff,sizeof(buff), 2000);
	if (ret > 0){
		debug("S>C:%s\r\n",buff);
	}
	return 0;
}


int exec_exit(struct ftp_client *client, int argc ,char **argv)
{
	char buff[1024];
	int ret = sprintf(buff,"QUIT\r\n");
	ftp_send(client->fd, buff,ret);
	ret = ftp_recv(client->fd, buff,sizeof(buff), 2000);
	if (ret > 0){
		debug("S>C:%s\r\n",buff);
	}
	return 0;
}

int if_file_exist(const char *file_with_path)
{
	struct stat buffer;
	return (stat(file_with_path, &buffer) == 0);
}


int exec_download(struct ftp_client *client, int argc ,char **argv)
{
	char buff[1024];
	char port[8];
	char *path, *filename;
	int fd1,ret;
	int file_size,int_port,get_length = 0;
	FILE *pfile;
	if(argc < 2){
		red_bug("you should use %s <remote file > to download\r\n",argv[0]);
		return -1;
	}
	path = argv[1];
	file_size = exec_size(client, argc ,argv);
	if (file_size <= 0){
		debug("get file size error\r\n");
		return -2;
	}
	int_port = exec_pasv(client,argc,argv);
	if (port <= 0){
		return -3;
	}
	sprintf(port,"%d", int_port);
	fd1 = connet_server(client->server,port);
	if (fd1 < 0)
	{
		perror("connect new");
		return -4;
	}
	
	filename = strrchr(path,'/');
	if (filename == NULL)
		filename = path;
	else
		filename += 1;
	debug("will download file %s\r\n",filename);
	if (if_file_exist(filename)){
		ebug("file exist\r\n");
		close(fd1);
		return -5;
	}
	pfile = fopen(filename, "w+");
	if(pfile == NULL){
		debug("open file %s failed\r\n",filename);
		ebug("file exist\r\n");
		close(fd1);
		return -6;
	}
	/*start download*/
	ret = sprintf(buff,"RETR %s\r\n",path);
	ftp_send(client->fd, buff,ret);
	ret = ftp_recv(client->fd, buff,sizeof(buff), 2000);
	if (ret <= 0 || atoi(buff) != 150){
		ebug("CMD RETR error\r\n");
		close(fd1);
		fclose(pfile);
		return -7;	
	}
	debug("S>C:%s\r\n",buff);
	
	get_length = 0;
	while ((ret = ftp_recv(fd1, buff,sizeof(buff),1000))>0){
		get_length += ret;
		fwrite(buff,1,ret,pfile);
	}
	debug("recv %d data\r\n",get_length);
	fclose(pfile);
	close(fd1);

	/*Recv the retr answer*/
	ret = ftp_recv(client->fd, buff,sizeof(buff), 2000);
	if (ret > 0){
		debug("S>C:%s\r\n",buff);
	}

	return 0;
}

long get_file_size(FILE *pfile)
{
	long file_size;
	long store_cur = ftell(pfile);
	fseek(pfile,0,SEEK_END);
	file_size = ftell(pfile);
	fseek(pfile,store_cur,SEEK_SET);
	return file_size;
}

int exec_upload(struct ftp_client *client, int argc ,char **argv)
{
	char buff[1024];
	char port[8];
	char *path, *filename;
	int fd1,ret;
	int int_port,send_length = 0;
	long file_size;
	FILE *pfile;
	if(argc < 2){
		red_bug("you should use %s <local filename > to <remote firename>upload\r\n",argv[0]);
		return -1;
	}
	path = "./";
	filename = argv[1];
	if (argc > 3){
		path = argv[2];
	}

	pfile = fopen(filename,"r");
	if(pfile == NULL){
		ebug("open local file failed\r\n");
		return -2;
	}
	
	file_size = get_file_size(pfile);
	debug("local file is %ld bytes\r\n",file_size);

	int_port = exec_pasv(client,argc,argv);
	if (port <= 0){
		return -3;
	}
	sprintf(port,"%d", int_port);
	fd1 = connet_server(client->server,port);
	if (fd1 < 0)
	{
		perror("connect new");
		return -4;
	}
	ret = sprintf(buff,"STOR %s/%s\r\n", path,filename);
	ftp_send(client->fd, buff,ret);
	ret = ftp_recv(client->fd, buff,sizeof(buff), 1000);
	if (ret <= 0 ){
		ebug("CMD STOR error\r\n");
		close(fd1);
		fclose(pfile);
		return -5;	
	}
	debug("S>C:%s\r\n",buff);
	send_length = 0;

	while(!feof(pfile)){
		if((ret = fread(buff,1, sizeof(buff), pfile))> 0){
			send_length += ret;
			ftp_send(fd1, buff, ret);
		}
	}
	debug("upload %d bytes\r\n",send_length);
	close(fd1);
	fclose(pfile);
	ret = ftp_recv(client->fd, buff,sizeof(buff), 1000);
	if (ret > 0 ){
		debug("S>C:%s\r\n",buff);
	}
	return 0;

}


int exec_remove(struct ftp_client *client, int argc ,char **argv)
{
	char buff[1024];
	int ch;
	int is_dir = 0, ret;
	char *del_name;
	if(argc < 2){
		red_bug("you should use %s <file or dir name> to delete it\r\n",argv[0]);
		return -1;
	}
	del_name = argv[1];
	while((ch = getopt(argc,argv,"r:")) != -1){
		debug("optarg:%s\n",optarg);
		switch(ch){
			case 'r' :
				debug("dir name is %s\r\n",optarg);
				is_dir = 1;
				del_name = optarg;
				break;
		}
	}
	if (is_dir){
		ret = sprintf(buff,"RMD %s\r\n",del_name);
	}else{
		ret = sprintf(buff,"DELE %s\r\n",del_name);
	}
	ftp_send(client->fd, buff,ret);
	ret = ftp_recv(client->fd, buff,sizeof(buff), 2000);
	if (ret > 0){
		debug("S>C:%s\r\n",buff);
	}
	
	return 0;
}

int exec_mkdir(struct ftp_client *client, int argc ,char **argv)
{
	char buff[1024];
	int ret;
	if(argc < 2){
		red_bug("you should use %s <dir name> to create one directory\r\n",argv[0]);
		return -1;
	}
	ret = sprintf(buff,"MKD %s\r\n",argv[1]);
	ftp_send(client->fd, buff,ret);
	ret = ftp_recv(client->fd, buff,sizeof(buff), 2000);
	if (ret > 0){
		debug("S>C:%s\r\n",buff);
	}
	return 0;
}




struct cmd_func cmd_list[] = 
{
	{"ls",exec_ls},
	{"pwd",exec_pwd},
	{"cd",exec_cd},
	{"scp",exec_download}, // download
	{"update",exec_upload},  //upload
	{"rm",exec_remove},  //delete file or directory
	{"mkdir",exec_mkdir},  //mkidr
	{"exit",exec_exit},
	{"size",exec_size},
	{"type",exec_type},
	{NULL,NULL}
};



int get_request(char * response,int size) 
{
    if(fgets(response, size, stdin) == NULL) {
        perror("Error reading user input");
		return -1;
    }
	return 0;
}


int do_input_request(char *cmd)
{
	int i,argc = 0;
	char *argv[6];
	struct cmd_func *iter;
//	debug("get input %s\r\n",cmd);
	if(*cmd < ' ' || *cmd > '~') return -1;
	for(i=0;cmd[i];i++){
		if (isspace(cmd[i])){
			cmd[i] = 0;
			continue;
		}
		if (i == 0 || cmd[i-1] == 0 ){
			if(argc<ARRAY_SIZE(argv)-1){
				argv[argc++] = cmd +i;
			}else{
				break;
			}
		}
	}
	
	ibug("ARGC(%d),CMD(%s)\r\n",argc,argv[0]);
	for (iter = &cmd_list; iter->cmd; iter++){
		if (0 == strcasecmp(argv[0], iter->cmd) && iter->func != NULL){
			if (0 == strcasecmp(argv[0],"exit")){
				loop_exit = 1;
			}
			return (*iter->func)(&client,argc,argv);
		}
	}
	return -1;
}



int input_loop(void)
{
	char input[256];
	debug("enter shell\r\n");
	for (;loop_exit == 0 ;){
		ibug(">");
		if (get_request(input,sizeof(input))){
			debug("get input error\r\n");
			sleep(1);
			continue;
		}
		do_input_request(input);
	}
	return 0;
}


int main(int argc ,char **argv)
{
	int ret,code;
	char server_msg[256];
	char *server = "localhost";
	char *port = "21";
	char *user = "ubuntu";
	char *passwd = "wskidtf";
	if (argc >1)
		server = argv[1];
	if (argc >2)
		port = argv[2];
	if (argc >3)
		user = argv[3];
	if (argc >4)
		passwd = argv[4];
	signal(SIGINT, signal_handler);

	if (init_client(&client,server,port,user,passwd) <=0){
		ebug("init client failed\r\n");
		return -1;
	}
	
	ret = ftp_recv(client.fd, server_msg, sizeof(server_msg), 1000);
	if (ret >0){
		debug("S>C:%s\r\n",server_msg);
		if ( 0 != memcmp("220",server_msg,3)){
			debug("server not support \r\n");
		}
	}else{
		ebug("server unreachable\r\n");
	}

	/*login user*/
	ret = sprintf(server_msg,"USER %s\r\n",client.user);
	ftp_send(client.fd, server_msg,ret);
	code = 0;
	ret = ftp_recv(client.fd, server_msg, sizeof(server_msg), 1000);
	if (ret >0){
		debug("S>C:%s\r\n",server_msg);
		code = atoi(server_msg);
		debug("code is %d",code);
	}
	if (code != 331){
		ebug("server : user\r\n");
		return -1;
	}
	/*passwd */
	ret = sprintf(server_msg,"PASS %s\r\n",client.passwd);
	ftp_send(client.fd, server_msg,ret);
	code = 0;
	ret = ftp_recv(client.fd, server_msg, sizeof(server_msg), 3000);
	if (ret >0){
		debug("S>C:%s\r\n",server_msg);
		code = atoi(server_msg);
		debug("code is %d\r\n",code);
	}
	debug("code is %d\r\n",code);
	if (code == 230 ){
		debug("login OK\r\n");
	}else{
		ebug("server code is %d, msg:%s\r\n",code,server_msg);
		return -1;
	}
	/*set type as binary*/
	ret = sprintf(server_msg,"TYPE I\r\n");
	ftp_send(client.fd, server_msg,ret);
	ret = ftp_recv(client.fd, server_msg,sizeof(server_msg), 2000);
	if (ret > 0){
		debug("S>C:%s\r\n",server_msg);
	}
	input_loop();
	close(client.fd);
	return 0;
}


