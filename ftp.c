#define DEBUG
#include "util.h"
#include "ftp.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include <arpa/ftp.h>
#include <netdb.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <getopt.h>
#include <ctype.h>
#include <sys/stat.h>

void set_noblock(int sockfd){
 	int flags = fcntl(sockfd,F_GETFL,0);
	fcntl(sockfd,F_SETFL,flags|O_NONBLOCK);
}

int connet_server(char *saddr,char *port)
{
#if 0
if (( getnameinfo((struct sockaddr *)p->ai_addr,
					p->ai_addrlen, 
					ip, sizeof(ip),
					NULL, (socklen_t) 0U, 
					NI_NUMERICHOST)) != 0)
		print(PERROR, TRUE,"Cannot resolve %s- (%s).", server,gai_strerror(1));

	if (( fd = socket(p->ai_family, p->ai_socktype, 0)) < 0 )
		print(PERROR, TRUE,"Failed to create socket.");

#endif
#define SOCK_LEN sizeof(struct sockaddr_in)
	int sockfd=0;
	struct sockaddr_in serverAddr;
	struct hostent *host;
	if((host=gethostbyname(saddr))==NULL){
		ebug("Gethostname error, %s\n",strerror(errno));
		return -1;
	}
	if(-1==(sockfd=socket(AF_INET,SOCK_STREAM,0))){
		ebug("Socket Error:%s\n",strerror(errno));
		return -1;
	}
	memset(&serverAddr,0,SOCK_LEN);
	serverAddr.sin_family=AF_INET;
	serverAddr.sin_port=htons(atoi(port));
	serverAddr.sin_addr=*((struct in_addr *)host->h_addr);
	debug("get host name is %s IP is %s Port is %d \n",host->h_addr,inet_ntoa(serverAddr.sin_addr),atoi(port));
	if(-1==connect(sockfd,(struct sockaddr *)&serverAddr,SOCK_LEN)){
		ebug("connect server Error:%s",strerror(errno));	
		return -1;
	}

	debug("sock[%d] connect server:%s success\r\n",sockfd,saddr);
	return sockfd;
}

int init_client(struct ftp_client *client, char *server, char *port,char *user, char *passwd)
{
	client->stat = 0;
	sprintf(client->server,"%s",server);
	sprintf(client->port,"%s",port);
	sprintf(client->user,"%s",user);
	sprintf(client->passwd,"%s",passwd);
	client->fd = connet_server(client->server,client->port);
	return client->fd;
}

int get_max_fd(int fd[],int fd_num)
{
	int i,max_fd = 0;
	for (i = 0;i < fd_num-1; i++){
		max_fd = max(fd[i],fd[i+1]);
	}
	return max_fd;
}

int poll_sock(int fd, int timeout_ms)
{
	fd_set local_sets;
	struct timeval tmo;
	tmo.tv_sec = timeout_ms/1000;
	tmo.tv_usec = (timeout_ms-tmo.tv_sec*1000)*1000;
	FD_ZERO(&local_sets);
	FD_SET (fd, &local_sets);
	return select(fd+1, &local_sets, NULL, NULL, &tmo);
}



