/*
 * fd_receiver - a simple utility to receive a file descriptor capability
 * over a Unix domain socket and then exec an arbitrary bash command
 * once received.
 *
 * Usage: fd_receiver -d <target fd> -s <socket filename> -- <bash args>
 *
 * -d: which file descriptor # the received file descriptor should
 * be slotted into.
 *
 * -s: the socket filename to bind to and wait for file descriptor to
 * be transmitted over
 *
 * Include "--" after flags this utility should process in order to terminate
 * getopt process.
 *
 * All subsequent args are passed to /bin/bash when exec-ing.
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

int
main(int argc, char *argv[])
{
	char  *socket_filename = NULL;
	char  *fd_str          = NULL;
	char  *tailptr         = NULL;
	int    target_fd       = -1;
	char **args            = NULL;

	int    c;
	while ((c = getopt(argc, argv, "d:s:")) != -1) {
		switch (c) {
		case 'd':
			fd_str = optarg;
			break;
		case 's':
			socket_filename = optarg;
			break;
		default:
			fprintf(stderr,
				"Usage: %s -d <target fd> -s <socket_filename> -- <bash args>\n",
				argv[0]);
			exit(1);
		}
	}

	if (fd_str == NULL || socket_filename == NULL) {
		fprintf(stderr, "Usage: %s -d <target fd> -s <socket_filename> -- <bash args>\n",
			argv[0]);
		exit(1);
	}

	target_fd = strtoul(fd_str, &tailptr, 0);

	// Create a Unix domain socket.
	int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("socket");
		exit(1);
	}

	// Bind the socket to the specified filename.
	struct sockaddr_un addr;
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, socket_filename, sizeof(addr.sun_path));
	if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind");
		exit(1);
	}

	// Listen for incoming connections.
	if (listen(sockfd, 5) < 0) {
		perror("listen");
		exit(1);
	}

	// Accept an incoming connection.
	int clientfd = accept(sockfd, NULL, NULL);
	if (clientfd < 0) {
		perror("accept");
		exit(1);
	}

	// Receive the file descriptor from the client.
	struct msghdr   msg;
	struct cmsghdr *cmsg;
	int             fd;

	// This is basically just a cute way to either access the header as
	// a struct or reference the whole thing as a buffer, all while ensuring
	// there's enough space for header + payload.
	union {
		char           buf[CMSG_SPACE(sizeof(int))];
		struct cmsghdr align;
	} controlMsg;

	msg.msg_name       = NULL;
	msg.msg_namelen    = 0;
	msg.msg_iov        = NULL;
	msg.msg_iovlen     = 0;
	msg.msg_control    = &controlMsg.align;
	msg.msg_controllen = CMSG_SPACE(sizeof(fd));
	msg.msg_flags      = 0;

	if (recvmsg(clientfd, &msg, 0) < 0) {
		perror("recvmsg");
		exit(1);
	}

	cmsg = CMSG_FIRSTHDR(&msg);
	if (cmsg == NULL || cmsg->cmsg_len < CMSG_LEN(sizeof(fd))) {
		fprintf(stderr, "No file descriptor received.\n");
		exit(1);
	}

	if (cmsg->cmsg_type != SCM_RIGHTS) {
		fprintf(stderr, "Unexpected control message type.\n");
		exit(1);
	}

	// Grab which fd the system decided to use.
	memcpy(&fd, CMSG_DATA(cmsg), sizeof(fd));

	// Dup2 into the file descriptor we actually want to use.
	if (dup2(fd, target_fd) < 0) {
		fprintf(stderr, "Failed to dup2 from %d to %d.\n", fd, target_fd);
		exit(1);
	}

	// skip "--" if it exists (it should)
	if (optind < argc && strncmp(argv[optind], "--", 2) == 0) {
		++optind;
	}

	// Be nice to execvp and set the first arg to "/bin/bash"
	argv[optind - 1] = "/bin/bash";

	// Exec. Note this shouldn't return.
	if (execvp("/bin/bash", &argv[optind - 1]) < 0) {
		perror("execvp");
	}
	exit(1);

	return 0;
}