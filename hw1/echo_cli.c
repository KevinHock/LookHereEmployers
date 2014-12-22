#include "unp.h"

// should be in header
#define ECHO_PORT 53815
#define TIME_PORT 20913

void select_all_the_things(int sock, int hey_mom);
// should be in header


int main(int argc, char **argv){
	int hey_mom = atoi(argv[2]);

	int	sock;
	struct sockaddr_in	echoaddr;

// if debug
	// int i;
	// for (i = 0; i < argc; i++){
	// 	write(hey_mom, argv[i], strlen(argv[i])+1);
	// 	write(hey_mom, "\n", strlen("\n")+1);
	// }
	// write(hey_mom, "__done printing arguments__\n", strlen("__done printing arguments__\n"));
// #endif


	if ( (sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		write(hey_mom, "socket error", 13);
		err_sys("socket error");
	}

	bzero(&echoaddr, sizeof(echoaddr));
	echoaddr.sin_family      = AF_INET;
	echoaddr.sin_port        = htons(ECHO_PORT);	/* echo */
	inet_pton(AF_INET, argv[1], &echoaddr.sin_addr);
	
	if (connect(sock, (SA *) &echoaddr, sizeof(echoaddr)) < 0){
		write(hey_mom, "connect error", strlen("connect error")+1);
		err_sys("connect error");
	}

	select_all_the_things(sock, hey_mom);
}

void select_all_the_things(int sock, int hey_mom){
	int maxfd = max(sock, hey_mom);
	fd_set rset;
	char received[1025];
	ssize_t	amount_read;

	FD_ZERO(&rset);

	while(1){
		FD_SET(STDIN_FILENO, &rset);
		FD_SET(sock, &rset);

		bzero(&received, sizeof(received));

		if(select(maxfd+1, &rset, NULL, NULL, NULL) < 0){
			/* (never returns 0 on timeout) */
			if(errno == EINTR){
				perror("eintr in select");
				continue;
			}
			perror("select error");
			while(1){}
			err_sys("select failed");
		}

		// sock
		if(FD_ISSET(sock, &rset)) {	/* there is stuff to echo */
			// perror("in sock is set");
			if ( (amount_read = read(sock, received, sizeof(received))) <= 0){
				if(amount_read != 0){
					perror("read in sock FD_ISSET error\n");
					while(1){}
					err_sys("read failed");
				}
				write(hey_mom, "\n\n\nServer says bye\n\n\n", strlen("\n\n\nServer says bye\n\n\n")+1);
				perror("server says bye");
				// while(1){}
				break;
			}

			if(writen(hey_mom, received, amount_read) != amount_read){
				perror("writen in sock FD_ISSET error");
				while(1){}
				err_sys("writen error");
			}
		}

		bzero(&received, sizeof(received)); // ghi this needed?

		// stdin
		if(FD_ISSET(STDIN_FILENO, &rset)) {	/* user typed */
			if ( (amount_read = read(STDIN_FILENO, received, sizeof(received))) <= 0){ /* made the mistake of reading from sock here :facepalm: */
				if(amount_read != 0){
					perror("read in STDIN_FILENO FD_ISSET error\n");
					while(1){}
				}
				perror("ctrld in client i presume");
				// while(1){}

				// ...
				// ...
				// ...
				// ...
				break;
			}
// if debug
			// printf("received in STDIN_FILENO is %s\n", received);
			// fflush(NULL);
// #endif
			if(writen(sock, received, amount_read) != amount_read){
				perror("writen in STDIN_FILENO error");
				while(1){}
				err_sys("writen error");
			}
		}
	}
	close(hey_mom);
	close(sock);
}
