#include "unp.h"
#include <stdio.h>
// #include	<netdb.h>
#include <ctype.h>
#include <stdlib.h>


#define ECHO_PORT 53815
#define TIME_PORT 20913
int host_or_ip(char *server);
void sig_chld(int signo);
// const char *Inet_ntop(int family, const void *addrptr, char *strptr, size_t len);

int kidsdead;

int main(int argc, char **argv){
	struct hostent *hptr;
	char buf[INET6_ADDRSTRLEN];
	char **listptr;
	char *list[100];
	
	int fd[2];
	pid_t pid;

	int n;
	char line[1024];

	if (argc != 2)
		err_quit("The \"client\" program has the following usage:\n\n\t./client <server>\n");

	int hostFlag = host_or_ip(argv[1]);
	struct in_addr net_addr;

	// WORKS
	if (hostFlag)
	{
		printf("host\n");
		if ( (hptr = gethostbyname(argv[1])) == NULL) {
			err_quit("gethostbyname error for host: %s: %s",
					argv[1], hstrerror(h_errno));
		}
		printf("The server IP is blaah\n");
		int i=0;
		for (i = 0, listptr = hptr->h_addr_list; *listptr != NULL; listptr++) {
			list[i] = *listptr;
			printf("\taddress: %s\n",
				   inet_ntop(AF_INET, list[i], buf, sizeof(buf)));
		}

		printf("hptr->h_addr_list is %s\n", inet_ntop(AF_INET, *hptr->h_addr_list,buf, sizeof(buf)));

	}
	else{
		// lol i will just keep this, oh well
		inet_pton(AF_INET, argv[1], &net_addr);

		printf("not host\n");
		if ( (hptr = gethostbyaddr((char*)&net_addr, sizeof(struct in_addr), AF_INET)) == NULL) {
			err_quit("gethostbyname error for ip: %s: %s",
					argv[1], hstrerror(h_errno));
		}
		printf("The server host is %s\n", hptr->h_name);
		printf("blahblahblah is %s\n", inet_ntop(AF_INET, &net_addr, buf, sizeof(buf)));
	}


	printf("ARGS ARE\n");
	int i;
	for (i = 0; i < argc; ++i)
	{
		printf("%s\n", argv[i]);
	}
	printf("ARGS ARE\n");
	printf("%d\n", ECHO_PORT);
	int option;	
	while(1){
		// queries the user which service is being requested.
		// There are two options : echo and time
		printf("1 - echo\n2 - time\n3 - quit\n");
		signal(SIGCHLD, sig_chld);

		scanf("%d", &option);
		if (option==1)
		{
			if (pipe(fd) < 0)
				err_sys("pipe error");
			if ((pid = fork()) < 0) {
				err_sys("fork error");
			} else if (pid > 0) {		/* parent */
				close(fd[1]);







				// The parent exits the second loop when the child closes the pipe
				// (how does the parent detect this?)
				// and/or the SIGCHLD signal is generated when the child terminates.
				int maxfd = fd[0];
				fd_set rset;
				char received[1025];
				ssize_t	amount_read;

				FD_ZERO(&rset);

				while(1){			
					
					// handle SIGCHLD
					if (kidsdead==1)
					{
						printf("the kid is dead\n");
						kidsdead=0;
						close(fd[0]);
						break;
					}

					FD_SET(STDIN_FILENO, &rset);
					FD_SET(fd[0], &rset);

					bzero(&received, sizeof(received));

					// use select here to monitor stdin and the pipe
					if(select(maxfd+1, &rset, NULL, NULL, NULL) < 0){
						/* (never returns 0 on timeout) */
						if(errno == EINTR){
							perror("eintr in select");
							continue;
						}
						perror("writen in select error");
						err_sys("select failed");
					}

					// stdin
					if(FD_ISSET(STDIN_FILENO, &rset)) {	/* user typed */
						
						printf("Type in the xterm window\n");

						fflush(NULL);
						if ( (amount_read = read(STDIN_FILENO, received, sizeof(received))) <= 0){  
							if(amount_read != 0){
								perror("read error\n");
								// ghi
							}
						}
					}
										
					if(FD_ISSET(fd[0], &rset)) {	/* kid has something to say */

						if( (n = read(fd[0], line, 1024)) == 0){
							printf("child closed pipe\n");
							break;
						}
						printf("Kid says \"%s\"\n", line);
					}

				}














			} else {				/* child */
				close(fd[0]);

				char pass_the_pipe[5];
				sprintf(pass_the_pipe, "%d", fd[1]);
				// printf("pass_the_pipe is %s\n", pass_the_pipe);
				// maybe void* pass_the b/c solaris
				if(hostFlag){
					// printf("*hptr->h_addr_list :: %s\nbuf :: %s\n", *hptr->h_addr_list, buf);					
					if(execlp("xterm", "xterm", "-e", "./echo_cli", inet_ntop(AF_INET, *hptr->h_addr_list, buf, sizeof(buf)), pass_the_pipe, NULL) == -1){
						err_sys("echo execlp error");
					}
				}else{
					// printf("&net_addr :: %s\nbuf :: %s\n", (char *)&net_addr, buf);
					if(execlp("xterm", "xterm", "-e", "./echo_cli", inet_ntop(AF_INET, &net_addr, buf, sizeof(buf)), pass_the_pipe, NULL) == -1){
						err_sys("echo execlp error");
					}					
				}
			}

			printf("its echo\n");
		}
		else if(option==2)
		{
			if (pipe(fd) < 0)
				err_sys("pipe error");
			if ((pid = fork()) < 0) {
				err_sys("fork error");
			} else if (pid > 0) {		/* parent */
				close(fd[1]);


				// The parent exits the second loop when the child closes the pipe
				// (how does the parent detect this?)
				// and/or the SIGCHLD signal is generated when the child terminates.
				int maxfd = fd[0];
				fd_set rset;
				char received[1025];
				ssize_t	amount_read;

				FD_ZERO(&rset);

				while(1){			
					
					// handle SIGCHLD
					if (kidsdead==1)
					{
						printf("the kid is dead\n");
						kidsdead=0;
						close(fd[0]);
						break;
					}

					FD_SET(STDIN_FILENO, &rset);
					FD_SET(fd[0], &rset);

					bzero(&received, sizeof(received));

					// use select here to monitor stdin and the pipe
					if(select(maxfd+1, &rset, NULL, NULL, NULL) < 0){
						/* (never returns 0 on timeout) */
						if(errno == EINTR){
							perror("eintr in select");
							continue;
						}
						perror("writen in select error");
						err_sys("select failed");
					}

					// stdin
					if(FD_ISSET(STDIN_FILENO, &rset)) {	/* user typed */
						
						printf("Type in the xterm window\n");

						fflush(NULL);
						if ( (amount_read = read(STDIN_FILENO, received, sizeof(received))) <= 0){  
							if(amount_read != 0){
								perror("read error\n");
								// ghi
							}
						}
					}
										
					if(FD_ISSET(fd[0], &rset)) {	/* kid has something to say */

						if( (n = read(fd[0], line, 1024)) == 0){
							printf("child closed pipe\n");
							break;
						}
						printf("Kid says \"%s\"\n", line);
					}

				}















			} else {				/* child */
				close(fd[0]);
				// write(fd[1], "hello world\n", 12);

				char pass_the_pipe[5];
				sprintf(pass_the_pipe, "%d", fd[1]);
				printf("pass_the_pipe is %s\n", pass_the_pipe);
				// maybe void* pass_the b/c solaris
				if(hostFlag){
					// printf("*hptr->h_addr_list :: %s\nbuf :: %s\n", *hptr->h_addr_list, buf);
					if(execlp("xterm", "xterm", "-e", "./time_cli", inet_ntop(AF_INET, *hptr->h_addr_list, buf, sizeof(buf)), pass_the_pipe, NULL) == -1){
						err_sys("echo execlp error");
					}
				}else{
					// printf("&net_addr :: %s\nbuf :: %s\n", (char *)&net_addr, buf);					
					if(execlp("xterm", "xterm", "-e", "./time_cli", inet_ntop(AF_INET, &net_addr, buf, sizeof(buf)), pass_the_pipe, NULL) == -1){
						err_sys("echo execlp error");
					}					
				}
			}

		}
		else if(option==3)
		{
			break;
		}else{
			printf("Hey use the xterm window.\n");
		}
	}

	printf("Good day sir!\n");

	return 0;
}

int host_or_ip(char *server){
	// loop through is digit or 46 for .
	int i;
	for (i = 0; i < strlen(server); ++i)
		if( !isdigit(server[i]) && server[i] != 46)
			return 1;// host	
	return 0;// ip
}

void
sig_chld(int signo)
{
	pid_t	pid;
	int		stat;

	while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0){
		printf("child %d terminated\n", pid); // printf in sig handler? zalewski dont approve
		kidsdead = 1;
	}
	return;
}



