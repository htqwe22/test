#ifndef FTP__H
#define FTP__H
#ifdef __cplusplus
extern "C" {
#endif
#include <sys/types.h>
#include <sys/socket.h>

struct ftp_client
{
	int fd;
	char port[6];
	char server[26];
	char user[12];
	char passwd[12];
	int stat;
};
int connet_server(char *saddr,char *port);

int init_client(struct ftp_client *client, char *server, char *port,char *user, char *passwd);
int poll_sock(int fd, int timeout_ms);


#ifdef __cplusplus
}
#endif

#endif

