#include "asgn3.h"


void msg_send(int sockfd, char *canonical_dest_ip, int dest_port, char *sent_msg, int force_rediscovery_flag, int from_cli){


	datamsg msg;
	struct sockaddr_un address;
	bzero(&address, sizeof(address));
	address.sun_family = AF_LOCAL;
	// if(from_cli){
	strcpy(address.sun_path, ODR_SERVICE_PATH);
	// }else{
	// 	strcpy(address.sun_path, );
	// }
	
	bzero(&msg, sizeof(msg));	
	//snprintf(msg, MAXLINE, "%s    %d    %s    %d", canonical_dest_ip, dest_port, sent_msg, force_rediscovery_flag);
	strcpy(msg.canonical_ip, canonical_dest_ip);
	strcpy(msg.data, sent_msg);
	msg.port = dest_port;
	msg.force_flg = force_rediscovery_flag;
	fflush(NULL);
	// Connect(sockfd, (SA*) &address, sizeof(address)); ghi
	// printf("connected fine\n"); ghi

	//printf("msg_send: msg canonical ip %s\n", msg.canonical_ip);
	//printf("msg_send: msg data %s\n", msg.data);
	//printf("msg_send: port %d\n", msg.port);
	//printf("msg_send: force_flg %d\n",msg.force_flg);
	Sendto(sockfd, &msg, sizeof(msg), 0, &address, sizeof(address)); //sizeof(struct sockaddr_un));
}

void msg_recv(int sockfd, char *canonical_src_ip, int *src_port, char *recieved_msg, int from_cli){
	// i personally hate signals and having to mask them
	// lets just make it use select
	int maxfd = sockfd;
	struct sockaddr_un recv_address;
	fd_set rset;	
	int how_much_read;
	datamsg msg;
	socklen_t recv_addr_len;
	FD_ZERO(&rset);

	while(1){

		FD_SET(sockfd, &rset);

		// the following enables SIGALRM 
	    if(from_cli){
			//printf("about to UNBLOCK sigalarm\n");
	    	sigprocmask(SIG_UNBLOCK, &signals_open, NULL);
	    }
		if(select(maxfd+1, &rset, NULL, NULL, NULL) < 0){
			/* (never returns 0 on timeout) */
			perror("select less than 0");
			//if(errno == EINTR)
			//	continue;
			err_sys("select failed");
		}
		if(from_cli){
			sigprocmask(SIG_SETMASK, &signals_open, NULL);
			//printf("Now BLOCKed\n");
	    }
		// Now all signals are allowed except SIGALRM


		if (FD_ISSET(sockfd, &rset)) {
			recv_addr_len = sizeof(recv_address);
			bzero(&msg, sizeof(msg));
			how_much_read = Recvfrom(sockfd, &msg, sizeof(msg), 0, &recv_address, &recv_addr_len);
		    //printf("Recieved from %s\n", recv_address.sun_path);
		    if(how_much_read!=0){
				//printf("Just recv'd msg\n");
				//printf("DEBUG strlen of msg is %lu\n", strlen(msg));
				//printf("msg_recv: msg canonical ip %s\n", msg.canonical_ip);
				//printf("msg_recv: msg data %s\n", msg.data);
				//printf("msg_recv: port %d\n", msg.port);
				//printf("msg_recv: force_flg %d\n",msg.force_flg);

			    int throw_away; // we dont care about the force flag
				//sscanf(msg, "%s    %d    %s    %d",canonical_src_ip, src_port,recieved_msg, &throw_away);
			    strcpy(canonical_src_ip, msg.canonical_ip);
			    strcpy(recieved_msg, msg.data);
			    *src_port = msg.port;
			    throw_away = msg.force_flg;
				//printf("canonical_src_ip is %s\n", canonical_src_ip);
				//printf("src_port is %d\n", *src_port);
				//printf("recieved_msg is %s\n", recieved_msg);
				//printf("throw_away is %d (we dont care about the force flag) \n", throw_away);

				return;
			}
		}

	}
}