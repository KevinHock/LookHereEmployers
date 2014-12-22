#include "unp.h"

// should be in header
#define ECHO_PORT 53815
#define TIME_PORT 20913

void select_all_the_things(int sock, int hey_mom);
// should be in header

int main(int argc, char **argv){
	int hey_mom = atoi(argv[2]);
	int	sock;
	struct sockaddr_in	timeaddr;

// if debug
	// int i;
	// for (i = 0; i < argc; i++){
	// 	write(hey_mom, argv[i], strlen(argv[i]));
	// 	write(hey_mom, "\n", strlen("\n"));
	// }
	// write(hey_mom, "__done printing arguments__\n", strlen("__done printing arguments__\n"));
// #endif


	if ( (sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		write(hey_mom, "socket error", 13);
		err_sys("socket error");
	}

	bzero(&timeaddr, sizeof(timeaddr));
	timeaddr.sin_family      = AF_INET;
	timeaddr.sin_port        = htons(TIME_PORT);	/* time */
	inet_pton(AF_INET, argv[1], &timeaddr.sin_addr);
	
	if (connect(sock, (SA *) &timeaddr, sizeof(timeaddr)) < 0){
		write(hey_mom, "connect error", strlen("connect error")+1);
		err_sys("connect error");
	}

	select_all_the_things(sock, hey_mom);
}

void select_all_the_things(int sock, int hey_mom){
	char received[1025];
	ssize_t	amount_read;

	while(1){
		bzero(&received, sizeof(received));

		if ( (amount_read = readline(sock, received, sizeof(received))) <= 0){
			if(amount_read != 0)
				write(hey_mom, "readline error", strlen("readline error")+1);
			else{
				write(hey_mom, "\n\n\nServer says bye\n\n\n", strlen("\n\n\nServer says bye\n\n\n")+1);
				perror("server says bye");
				// while(1){}
			}
			break;
		}

		if(writen(hey_mom, received, amount_read) != amount_read){
				perror("writen error");
				while(1){}
				err_sys("writen error");
		}
	}
	close(hey_mom);
	close(sock);
}
