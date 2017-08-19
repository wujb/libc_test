/*****************************************************************************
#	CopyRight@Biaoun-Junbiaow@gmail.com
#
#      Filename: main.c
#        Author: Biaoun
#        Create: 2017-08-18 23:35:43
# Last Modified: 2017-08-19 10:49:51
#   Description: ---
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stddef.h>
#include <getopt.h>

#define QLEN	10
#define STALE	30
#define CLI_PATH	"/tmp/"
#define APP_NAME	"demo"

static const struct option long_options[] = {
	{"help",	no_argument,	NULL,	'h'},
	{"server",	no_argument,	NULL,	's'},
	{"client",	no_argument,	NULL,	'c'},
	{0, 0, 0, 0},
};

void usage(void)
{
	printf("%s --server | --client | --help\n", APP_NAME);
}

/**
 *	serv_listen - create the server socket handler
 *	@name: point to the connect name
 *	
 *	@return: the socket handler
 */
int serv_listen(const char *name)
{
	int fd = -1;
	int	size = 0;
	struct sockaddr_un un = {0};
	
	un.sun_family = AF_UNIX;
	strcpy(un.sun_path, name);

	unlink(un.sun_path);

	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		perror("socket create failed!\n");
		unlink(un.sun_path);		
		return -1;
	}


	size = sizeof(struct sockaddr_un);
	if (bind(fd, (struct sockaddr *)&un, size) < 0) {
		perror("bind failed!\n");
		close(fd);
		return -1;
	}
	
	printf("UNIX socket create and bind!\n");

	if (listen(fd, QLEN) < 0) {
		perror("listen error!\n");	
	}
	
	return fd;
}

/**
 * serv_accept - server terminal accepte the client connect
 *
 * @fd: the socket handler to accept
 * @uidptr: the pointer of client user identifer
 *
 * @return:
 *
 */
int serv_accept(int fd, uid_t *uidptr)
{
	int clifd, err, ret;
	socklen_t len;
	struct sockaddr_un un;
	struct stat statbuf;
	time_t statetime;

	len = sizeof(struct sockaddr_un);
	if ((clifd = accept(fd, (struct sockaddr *)&un, &len)) < 0) {
		return -1;
	}

	len -= offsetof(struct sockaddr_un, sun_path);
	un.sun_path[len] = 0;

	if (stat(un.sun_path, &statbuf) < 0) {
		return -2;	
	}

	if (S_ISSOCK(statbuf.st_mode) == 0) {
		return -3;	
	}

	if (statbuf.st_mode & ((S_IRWXG | S_IRWXO) || statbuf.st_mode & S_IRWXU != S_IRWXU)) {
		return -4;	
	}

	statetime = time(NULL) - STALE;
	if (statbuf.st_atime < statetime || statbuf.st_ctime < statetime || statbuf.st_mtime < statetime) { 
		return -5;	
	}
	
	if (uidptr != NULL) {
		*uidptr  = statbuf.st_uid;	
	}

	unlink(un.sun_path);

	return clifd;
}


/**
 *	cli_conn - create client socket handler
 *
 *	@name: client path name
 *
 *	@return: socket handler
 */
int cli_conn(const char *name)
{
	int fd, len, err, ret;
	struct sockaddr_un un;

	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		perror("socket create failed!\n");	
		return -1;
	}

	memset(&un, 0, sizeof(struct sockaddr_un));
	un.sun_family = AF_UNIX;
	sprintf(un.sun_path, "%s%05d", CLI_PATH, getpid());

	len = offsetof(struct sockaddr_un, sun_path) + strlen(un.sun_path);
	unlink(un.sun_path);
	
	if ((bind(fd, (struct sockaddr *)&un, len)) < 0) {
		perror("bind failed!\n");	
		close(fd);	
		return -2;
	}

	if (chmod(un.sun_path, S_IRWXU) < 0) {
		perror("chmod failed!\n");
		close(fd);	
		return -3;
	}

	memset(&un, 0, sizeof(struct sockaddr_un));
	un.sun_family = AF_UNIX;
	strcpy(un.sun_path, name);

	len = offsetof(struct sockaddr_un, sun_path) + strlen(un.sun_path);
	if (connect(fd, (struct sockaddr *)&un, len) < 0) {
		perror("connect failed!\n");	
		close(fd);	
		return -4;
	}

	return fd;
}

int main(int argc, char *argv[])
{
	int opt = -1;
	int index = -1;
	int sfd = -1;
	int cfd = -1;
	uid_t uid = -1;
	char buf[64] = {0};
	char *wbuf = "123456789abcdf123456789abcdef";
	opt = getopt_long(argc, argv, "", long_options, &index);
	if (opt == -1) {
		usage();	
		return 1;
	}

	switch (opt) {
	case 'h':
		usage();	
		break;
	case 's':
		printf("Server!\n");
		sfd = serv_listen("event.socket");	
		if (sfd < 0) {
			printf("erro: %d\n", sfd);
			exit(-1);
		}

		cfd = serv_accept(sfd, &uid);
		if (cfd < 0) {
			printf("serv accept failed: %d\n", cfd);	
			close(sfd);
			exit(-1);
		}

		if (read(cfd, buf, 32) < 0) {
			printf("read error!\n");	
			close(sfd);
			exit(-1);
		}

		printf("recv msg: %s\n", buf);
		break;
	case 'c':
		printf("Client!\n");
		cfd = cli_conn("event.socket");
		if (cfd < 0) {
			printf("client connect server failed: %d\n", cfd);	
			exit(-1);
		}

		write(cfd, wbuf, 32);	
		break;
	default:
		usage();	
		break;	
	}

	return 0;
}
