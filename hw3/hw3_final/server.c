#include "asgn3.h"

#define IN_SERV_WE_DONT_CARE 0
int port;

int main(int argc, char **argv)
{
	int					sockfd;
	struct sockaddr_un	servaddr, cliaddr;

	sockfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);

	unlink(WELLKNOWN_PATH);
	mkstemp(WELLKNOWN_PATH);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sun_family = AF_LOCAL;
	strcpy(servaddr.sun_path, WELLKNOWN_PATH);

	Bind(sockfd, (SA *) &servaddr, sizeof(servaddr));

	char hostname[128];
	char sent_msg[MAXLINE];
	time_t ticks;

	gethostname(hostname, sizeof hostname);
	//printf("My hostname: %s\n", hostname);


	char canonical_ip[128];
	
	char recieved_msg[MAXLINE];

	struct in_addr ipv4addr;
	struct hostent *he;

	while(1){

		// ghi maybe &recieved_msg
		msg_recv(sockfd, canonical_ip, &port, recieved_msg, FROMSERV);



		// beej
		inet_pton(AF_INET, canonical_ip, &ipv4addr);
		
		if((he = gethostbyaddr(&ipv4addr, sizeof(ipv4addr), AF_INET))==NULL)
			perror("gethostbyaddr error IMPORTANTIMPORTANTIMPORTANTIMPORTANTIMPORTANT");
		
		//printf("DEBUG Host name: %s\n", he->h_name);
		// beej

		printf("server at node %s responding to request from %s\n", hostname, he->h_name);
		bzero(&sent_msg, sizeof(sent_msg));		
        ticks = time(NULL);
        snprintf(sent_msg, sizeof(sent_msg), "%.24s\r\n", ctime(&ticks));
        //printf("we're sending %s\n", sent_msg);
		msg_send(sockfd, canonical_ip, port, sent_msg, IN_SERV_WE_DONT_CARE,0);

	}
}
