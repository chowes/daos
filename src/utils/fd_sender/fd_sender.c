/*
 * fd_sender - a simple utility to send a file descriptor over a Unix domain
 * socket.
 *
 * Usage: fd_sender -d <file descriptor #> -s <socket filename>
 *
 * -d: which file descriptor # to send
 *
 * -s: the socket filename to send the file descriptor capability over
 *
 * (Created to assist testing of DAOSGCP-213 patch)
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <sys/capability.h>
#include <fcntl.h>

int
main(int argc, char *argv[])
{
	char *socket_filename = NULL;
	char *fd_str          = NULL;
	char *tailptr         = NULL;
	int   fd              = -1;

	int   c;
	while ((c = getopt(argc, argv, "d:s:")) != -1) {
		switch (c) {
		case 'd':
			fd_str = optarg;
			break;
		case 's':
			socket_filename = optarg;
			break;
		default:
			fprintf(stderr, "Usage: %s -d <file descriptor #> -s <socket filename>\n",
				argv[0]);
			exit(1);
		}
	}

	if (socket_filename == NULL || fd_str == NULL) {
		fprintf(stderr, "Usage: %s -d <file descriptor #> -s <socket filename>\n", argv[0]);
		exit(1);
	}

	fd = strtoul(fd_str, &tailptr, 0);

	// Create a Unix domain socket.
	int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("socket");
		exit(1);
	}

	// Connect to the Unix domain socket.
	struct sockaddr_un addr;
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, socket_filename, sizeof(addr.sun_path));
	if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("connect");
		exit(1);
	}

	// Send the file descriptor as a capability over the Unix domain socket.

	// We have to send at least 1 byte of non-control data, so...
	char          bogus[] = ":)";
	struct msghdr msg;
	struct iovec  iov = {.iov_base = &bogus, .iov_len = sizeof(bogus)};

	// This is basically just a cute way to either access the header as
	// a struct or reference the whole thing as a buffer, all while ensuring
	// there's enough space for header + payload.
	union {
		char           buf[CMSG_SPACE(sizeof(int))];
		struct cmsghdr align;
	} controlMsg;

	memset(controlMsg.buf, 0, sizeof(controlMsg.buf));

	// Note that in case this is all referring to the "data" payload which
	// we absolutely don't actually care about (but must be >0).
	msg.msg_name       = NULL;
	msg.msg_namelen    = 0;
	msg.msg_iov        = &iov;
	msg.msg_iovlen     = 1;
	msg.msg_control    = &controlMsg;
	msg.msg_controllen = sizeof(controlMsg);
	msg.msg_flags      = 0;

	// Encode the actual file descriptor in the control message.
	controlMsg.align.cmsg_len   = CMSG_LEN(sizeof(fd));
	controlMsg.align.cmsg_level = SOL_SOCKET;
	controlMsg.align.cmsg_type  = SCM_RIGHTS;
	memcpy(CMSG_DATA(&controlMsg.align), &fd, sizeof(fd));

	if (sendmsg(sockfd, &msg, 0) < 0) {
		perror("sendmsg");
		exit(1);
	}

	close(fd);
	close(sockfd);

	return 0;
}