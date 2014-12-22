// do not judge me about the kev wraps ok? I had csaw ctf and stuff

// first tested client forks with printing all args and ctrl c and kill -9 to test sighandles
// second tested server with netcat to see my threads working
		// needed to put FD_SET(lis, &rset); in the loop so i could switch back n forth 

// should be in header
#define ECHO_PORT 53815
#define TIME_PORT 20913

#include "unp.h"
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <netdb.h>

void select_all_the_things(int echo_lis, int time_lis);
static void *echo_thread(void *arg);
static void *time_thread(void *arg);
void kevClose(int fd);
void kevPthread_detach(pthread_t tid);
int kevAccept(int fd, struct sockaddr *sa, socklen_t *salenptr);
void kevPthread_create(pthread_t *tid, const pthread_attr_t *attr, void * (*func)(void *), void *arg);
// should be in header


int main(int argc, char **argv){

	int echo_lis, time_lis;
	int flags;
	const int			on = 1;
	struct sockaddr_in	echoaddr, timeaddr;
	


	if( (echo_lis = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		err_sys("echo_lis = socket() failed");
	}
	if( (time_lis = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		err_sys("time_lis = socket() failed");
	}

	if( (flags = fcntl(echo_lis, F_GETFL, 0)) == -1){
		err_sys("fcntl problem in echo_lis F_GETFL");
	}if( (fcntl(echo_lis, F_SETFL, flags | O_NONBLOCK)) == -1){
		err_sys("fcntl problem in echo_lis F_SETFL");
	}

	if( (flags = fcntl(time_lis, F_GETFL, 0)) == -1){
		err_sys("fcntl problem in time_lis F_GETFL");
	}if( (fcntl(time_lis, F_SETFL, flags | O_NONBLOCK)) == -1){
		err_sys("fcntl problem in time_lis F_SETFL");
	}

	if(setsockopt(echo_lis, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0){
		err_sys("setsockopt echo_lis error");
	}

	if(setsockopt(time_lis, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0){
		err_sys("setsockopt time_lis error");
	}

	bzero(&echoaddr, sizeof(echoaddr));
	echoaddr.sin_family      = AF_INET;
	echoaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	echoaddr.sin_port        = htons(ECHO_PORT);	/* echo */

	bzero(&timeaddr, sizeof(timeaddr));
	timeaddr.sin_family      = AF_INET;
	timeaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	timeaddr.sin_port        = htons(TIME_PORT);	/* time */

	if(bind(echo_lis, (SA *) &echoaddr, sizeof(echoaddr)) < 0){
		err_sys("bind echo_lis error");
	}
	if(bind(time_lis, (SA *) &timeaddr, sizeof(timeaddr)) < 0){
		err_sys("bind time_lis error");
	}

	if(listen(echo_lis, LISTENQ) < 0){
		err_sys("listen echo_lis error");
	}
	if(listen(time_lis, LISTENQ) < 0){
		err_sys("listen time_lis error");
	}


	// perror("b4select");
	

	select_all_the_things(echo_lis, time_lis);
}

void select_all_the_things(int echo_lis, int time_lis){
	socklen_t			clilen;
	struct sockaddr_in	cliaddr;
	
	int connfd;
	pthread_t tid;
	int maxfd = max(echo_lis, time_lis);
	
	fd_set rset;
	int flags;

	FD_ZERO(&rset);
	// perror("b4while");

	while(1){
		FD_SET(echo_lis, &rset);
		FD_SET(time_lis, &rset);

		if(select(maxfd+1, &rset, NULL, NULL, NULL) < 0){
			/* (never returns 0 on timeout) */
			perror("select less than 0");
			if(errno == EINTR)
				continue;
			err_sys("select failed");
		}

		if (FD_ISSET(echo_lis, &rset)) {	/* new client connection */
			clilen = sizeof(cliaddr);
			connfd = kevAccept(echo_lis, (SA *) &cliaddr, &clilen); // sorry badr

			// char idkmane[1024];
			// printf("new client: %s, port %d\n",
			// 		inet_ntop(AF_INET, &cliaddr.sin_addr, idkmane, NULL),
			// 		ntohs(cliaddr.sin_port));
			// fflush(NULL);

			if(connfd > maxfd)
				maxfd = connfd;

			if( (flags = fcntl(connfd, F_GETFL, 0)) == -1)
				err_sys("fcntl problem in connfd F_GETFL");
			
			flags &= ~O_NONBLOCK;

			if( (fcntl(connfd, F_SETFL, flags)) == -1)
				err_sys("fcntl problem in connfd F_SETFL");

			kevPthread_create(&tid, NULL, &echo_thread, (void *) connfd);
		}else if(FD_ISSET(time_lis, &rset)){
//
			// perror("b4accept");
//
			clilen = sizeof(cliaddr);
			connfd = kevAccept(time_lis, (SA *) &cliaddr, &clilen); // sorry badr

			// should reuse but i dont care, imma clean it up before dan guido sees it 
			if(connfd > maxfd)
				maxfd = connfd;

			if( (flags = fcntl(connfd, F_GETFL, 0)) == -1)
				err_sys("fcntl problem in connfd F_GETFL");
			
			flags &= ~O_NONBLOCK;

			if( (fcntl(connfd, F_SETFL, flags)) == -1)
				err_sys("fcntl problem in connfd F_SETFL");

			kevPthread_create(&tid, NULL, &time_thread, (void *) connfd);
		}else{
			err_sys("dafuq select?");		
		}



	}
}

static void *echo_thread(void *arg)
{
	kevPthread_detach(pthread_self());
	int connfd = (int)arg;

	printf("echo_thread\n");
	printf("SEE ME\n");
	char received[1025];
	int amount_read;

	while(1){
		bzero(&received, sizeof(received));
		// perror("about to receive");
		if ( (amount_read = Readline(connfd, received, sizeof(received))) <= 0){
			if(amount_read != 0){
				printf("readline error\n");
				// handle error
				perror("readline messed up");
				break;
			}
			// idk
			printf("Client says goodbye\n");
			perror("we think server says goodbye");
			fflush(NULL);
			break;
		}
		// perror("after to receive");

		// debug
		printf("Server received \"%s\"\n", received);
		fflush(NULL);
		// debug
		

		if(writen(connfd, received, amount_read) != amount_read)
			err_sys("writen error");
	}
	perror("kev close");
	kevClose(connfd);		/* done with connected socket */
	return(NULL);
}

static void *time_thread(void *arg)
{
	kevPthread_detach(pthread_self());
	int connfd = (int)arg;
	printf("time_thread\n");
	char buff[1024];
	time_t ticks;
	
	struct timeval itseleven;
	itseleven.tv_sec = 5;
	itseleven.tv_usec = 0;
	fd_set rset;
	FD_ZERO(&rset);

	while(1){
		FD_SET(connfd, &rset);
		bzero(&buff, sizeof(buff));
		if(select(connfd+1, &rset, NULL, NULL, &itseleven) != 0){
			/* (never returns 0 on timeout) */
			if(errno == EINTR){
				perror("eintr in select");
				continue;
			}
			if(read(connfd, buff, sizeof(buff))==0){
				break;
			}
			perror("writen in select error");
			err_sys("select failed");
		}
		
		ticks = time(NULL);
        snprintf(buff, sizeof(buff), "%.24s\r\n", ctime(&ticks));
        if(write(connfd, buff, strlen(buff)) != strlen(buff)){
        	perror("in time thread write had problem");
        }
	}
	perror("kev close");
	kevClose(connfd);		/* done with connected socket */
	return(NULL);
}

void
kevPthread_create(pthread_t *tid, const pthread_attr_t *attr,
			   void * (*func)(void *), void *arg)
{
	int		n;

	if ( (n = pthread_create(tid, attr, func, arg)) == 0)
		return;
	errno = n;
	printf("errno is %d\n", errno);
	err_sys("pthread_create error");
}

int
kevAccept(int fd, struct sockaddr *sa, socklen_t *salenptr)
{
	int		n;

again:
	if ( (n = accept(fd, sa, salenptr)) < 0) {
#ifdef	EPROTO
		if (errno == EPROTO || errno == ECONNABORTED)
#else
		if (errno == ECONNABORTED)
#endif
			goto again;
		else
			err_sys("accept error");
	}
	return(n);
}

void kevPthread_detach(pthread_t tid)
{
	int		n;

	if ( (n = pthread_detach(tid)) == 0)
		return;
	errno = n;
	err_sys("pthread_detach error");
}

void
kevClose(int fd)
{
	if (close(fd) == -1)
		err_sys("close error");
}