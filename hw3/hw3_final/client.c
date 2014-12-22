#include "asgn3.h"

void ouch(int sig);


static sigjmp_buf jmpbuf;

char hostname[128];
char name[128];

int first_time=1;

char canonical_src_ip[128];
int src_port;
char recieved_msg[MAXLINE];

char timestamp[128];
time_t ticks;

// we must change these two when the time comes
char sent_msg[128]= "AA";
int force_rediscovery_flag=0;



int main(int argc, char **argv)
{
	int	sockfd;
	struct sockaddr_un cliaddr;
	int vm_number=0;


	char temp_file[] = "/tmp/immagetanAXXXXXX";
	int fd;
	int try_count=1;
	
	fd = mkstemp(temp_file);
	if(fd==-1)
		printf("mkstemp fail\n");
	unlink(temp_file);
	//printf("DEBUG temp_file is %s\n", temp_file);


	sockfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);

	bzero(&cliaddr, sizeof(cliaddr));		/* bind an address for us */
	cliaddr.sun_family = AF_LOCAL;
	strcpy(cliaddr.sun_path, temp_file);

	Bind(sockfd, (SA *) &cliaddr, sizeof(cliaddr)); // ghi

	gethostname(hostname, sizeof hostname);
	//printf("DEBUG My hostname: %s\n", hostname);
	


	// Create a mostly open interupt mask -- only masking SIGALRM
	sigemptyset(&signals_open);
    sigaddset(&signals_open, SIGALRM);
	// Fill in sigaction -- only masking SIGALRM    
    act_open.sa_flags = 0;
    sigemptyset(&act_open.sa_mask);
    sigaddset(&act_open.sa_mask, SIGALRM);
    act_open.sa_handler = ouch;
    // during SIGALRM call, further SIGALRM calls are blocked
    sigaction(SIGALRM, &act_open, 0);

	while(1)
	{
		char recv[MAXLINE];
		vm_number = 0;
		printf("Please enter a number between 1 and 10 inclusively for which # VM the server node should be.\n");
		if(fgets(recv, MAXLINE,stdin)== NULL)
		{
			printf("fgets input error");
			continue;
		}
		//printf("input %s\n", recv);
		if(sscanf(recv, "%d", &vm_number)<=0)
		{
			printf("non-valid input: <1-10> accepted\n");
			continue;
		}
		//printf("vm number %d\n",vm_number);
		if(vm_number<1 || 10<vm_number){
			printf("I said between 1 and 10 inclusively.\n");
			continue;
		}


	// get canonical_dest_ip for msg_send
		// beej
		int i;
	    struct hostent *he;
	    struct in_addr **addr_list;
	    struct in_addr ipv4addr;
		// ghi &name or name
		bzero(&name, sizeof(name));

        snprintf(name, sizeof(name), "vm%d", vm_number);

        //printf("name is %s\n", name);
		if ((he = gethostbyname(name)) == NULL) {  // get the host info
	        herror("gethostbyname");
	        return 2;
	    }

	    // print information about this host:
	    //printf("Official name is: %s\n", he->h_name);
	    //printf("    IP addresses: ");
	    addr_list = (struct in_addr **)he->h_addr_list;
	    for(i = 0; addr_list[i] != NULL; i++) {
	        //printf("%s ", inet_ntoa(*addr_list[i]));
	    }
	    printf("\n");
		// beej

	    //printf("message we're going to send is %s\n", sent_msg);
		printf("client at node %s sending request to server at %s\n", hostname, name);
		
		// Send
		// Send
		// Send
		msg_send(sockfd, inet_ntoa(*addr_list[0]), SERVER_PORT, sent_msg, force_rediscovery_flag,0);
		// Send
		// Send
		// Send



		recieve:
		alarm(3);

	    sigprocmask(SIG_SETMASK, &signals_open, NULL);
		// Now all signals are allowed except SIGALRM
	    //printf("DEBUG in client.c: only SIGALRM blocked now\n");
	    if(sigsetjmp(jmpbuf,1) != 0){
			if(first_time)
			{
				first_time=0;
				force_rediscovery_flag=1;
				printf("client at node %s: timeout on response from %s \n", hostname, name);
				printf("resend msg with force a rediscovery flag\n");
				//printf("DEBUG retransmitting\n");				
				msg_send(sockfd, inet_ntoa(*addr_list[0]), SERVER_PORT, sent_msg, force_rediscovery_flag,0);
				force_rediscovery_flag=0;
				try_count++;
				goto recieve;
			}
			else
			{
				if(try_count >2)
				{
					printf("try %d time over and restart the process\n", try_count);
					first_time=1;
					try_count=1;
					alarm(0);
					continue;
				}
				printf("client at node %s: timeout on response from %s\n", hostname, name);
				msg_send(sockfd, inet_ntoa(*addr_list[0]), SERVER_PORT, sent_msg, force_rediscovery_flag,0);
				try_count++;
				goto recieve;
			}
		}
		
			msg_recv(sockfd, canonical_src_ip, &src_port, recieved_msg, FROMCLI);

			first_time=1;
			try_count=1;
			alarm(0);

			inet_pton(AF_INET, canonical_src_ip, &ipv4addr);
		
			if((he = gethostbyaddr(&ipv4addr, sizeof(ipv4addr), AF_INET))==NULL)
				perror("gethostbyaddr error IMPORTANTIMPORTANTIMPORTANTIMPORTANTIMPORTANT");
			//printf("We've now exited msg_recv\n");
			printf("client at node %s: received from %s <%s>\n", hostname, he->h_name,recieved_msg);
			
		
#if 0
		


		bzero(&timestamp, sizeof(timestamp));		
        ticks = time(NULL);
        snprintf(timestamp, sizeof(timestamp), "%.24s\r\n", ctime(&ticks));

		printf("client at node %s: recieved from %s <%s>\n", hostname, name, timestamp);
#endif		
	}

	exit(0);
}

void ouch(int sig)
{
    //printf("OUCH! - I got signal %d\n", sig);
	siglongjmp(jmpbuf, 1);
}
