#include "unp.h"
#include "asgn3.h"
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include <stdlib.h>
#include "hw_addrs.h"
#include "odr_process.h"

#define PROTO_NUM 0x8182

#define ETH_FRAME_LEN 114
struct hwa_info* hw_list = NULL;

void free_hw_if( struct hwa_info* hw_if)
{
	if(hw_if->hwa_next != NULL)
	{
		free_hw_if(hw_if->hwa_next);
	}
	else
	{
		free(hw_if);
	}
}

void get_hw_if(void)
{
	struct hwa_info	*hwa,*hwahead, *hw_node;
	struct sockaddr	*sa;
	char   *ptr;
	int    i, prflag;

	printf("\n");

	for (hwahead= hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next) 
	{
		if(!strncmp(hwa->if_name, LO_NAME, strlen(LO_NAME)))	
		{
			continue;
		}
		if(!strncmp(hwa->if_name, ETH_0_NAME, strlen(ETH_0_NAME)))	
		{
			continue;
		}
		else
		{
			if(hw_list == NULL)
			{
				hw_list = (struct hwa_info*)malloc(sizeof(struct hwa_info));
				memcpy(hw_list, hwa, sizeof(struct hwa_info));
				hw_list->hwa_next = NULL;
			}
			else
			{
				hw_node = (struct hwa_info*)malloc(sizeof(struct hwa_info));
				memcpy(hw_node,  hwa, sizeof(struct hwa_info));
				hw_node->hwa_next = hw_list->hwa_next;
				hw_list->hwa_next = hw_node;
			}
		}
	}
	for(hwa = hw_list; hwa!= NULL; hwa = hwa->hwa_next)
	{
		printf("%s :%s", hwa->if_name, ((hwa->ip_alias) == IP_ALIAS) ? " (alias)\n" : "\n");
		
		if ( (sa = hwa->ip_addr) != NULL)
			printf("         IP addr = %s\n", Sock_ntop_host(sa, sizeof(*sa)));
				
		prflag = 0;
		i = 0;
		do {
			if (hwa->if_haddr[i] != '\0') {
				prflag = 1;
				break;
			}
		} while (++i < IF_HADDR);

		if (prflag) {
			printf("         HW addr = ");
			ptr = hwa->if_haddr;
			i = IF_HADDR;
			do {
				printf("%.2x%s", *ptr++ & 0xff, (i == 1) ? " " : ":");
			} while (--i > 0);
		}

		printf("\n         interface index = %d\n\n", hwa->if_index);
	}
	free_hwa_info(hwahead);
	//free_hw_if(hw_list);
}

int pf_send(int s)
{

	int j = 0;
	/*target address*/
	struct sockaddr_ll socket_address;

	/*buffer for ethernet frame*/
	void* buffer = (void*)malloc(ETH_FRAME_LEN);
 
	/*pointer to ethenet header*/
	unsigned char* etherhead = buffer;
	
	/*userdata in ethernet frame*/
	unsigned char* data = buffer + 14;
	
	/*another pointer to ethernet header*/
	struct ethhdr *eh = (struct ethhdr *)etherhead;
 
	int send_result = 0;

	/*our MAC address*/
	unsigned char src_mac[6];// = {0x00, 0x01, 0x02, 0xFA, 0x70, 0xAA};
	memcpy(src_mac, hw_list->if_haddr, ETH_ALEN);

	//printf("src mac %2x %2x %2x %2x %2x %2x\n", src_mac[0],src_mac[1],src_mac[2],src_mac[3],src_mac[4],src_mac[5]);
	/*other host MAC address*/
	unsigned char dest_mac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

	/*prepare sockaddr_ll*/

	/*RAW communication*/
	socket_address.sll_family   = PF_PACKET;	
	/*we don't use a protocoll above ethernet layer
	  ->just use anything here*/
	//socket_address.sll_protocol = htons(ETH_P_IP);	

	/*index of the network device
	see full code later how to retrieve it*/
	socket_address.sll_ifindex  = 4;

	/*ARP hardware identifier is ethernet*/
	//socket_address.sll_hatype   = ARPHRD_ETHER;
	
	/*target is another host*/
	//socket_address.sll_pkttype  = PACKET_OTHERHOST;

	/*address length*/
	socket_address.sll_halen    = ETH_ALEN;		
	/*MAC - begin*/
	socket_address.sll_addr[0]  = 0xff;		
	socket_address.sll_addr[1]  = 0xff;		
	socket_address.sll_addr[2]  = 0xff;
	socket_address.sll_addr[3]  = 0xff;
	socket_address.sll_addr[4]  = 0xff;
	socket_address.sll_addr[5]  = 0xff;
	/*MAC - end*/
	socket_address.sll_addr[6]  = 0x00;/*not used*/
	socket_address.sll_addr[7]  = 0x00;/*not used*/


	/*set the frame header*/
	memcpy((void*)buffer, (void*)dest_mac, ETH_ALEN);
	memcpy((void*)(buffer+ETH_ALEN), (void*)src_mac, ETH_ALEN);
	eh->h_proto = htons(0x8182);
	/*fill the frame with some data*/
	for ( j = 0; j < 96; j++) {
		data[j] = (unsigned char)0x33;
	}

	/*send the packet*/
	send_result = sendto(s, buffer, ETH_FRAME_LEN, 0, 
	      (struct sockaddr*)&socket_address, sizeof(socket_address));
	if (send_result < 0) 
	{
		perror("send pf pkt error");
	 }
}


int pf_recv(int s)
{
	struct sockaddr_ll socket_address;
	socklen_t len;
	int i = 0;
	unsigned char* buffer = (unsigned char*)malloc(ETH_FRAME_LEN); /*Buffer for ethernet frame*/
	int length = 0; /*length of the received frame*/ 

	length = recvfrom(s, buffer, ETH_FRAME_LEN, 0, (struct sockaddr*)&socket_address, &len);
	if (length < 0) 
	{ 
		perror("recv pf pkt error"); 
	}
	for(i = 0;i < length; i++)
	{
		printf("%.2x ", *(buffer+i));
		if((i-9) % 10 == 0) printf("\n");
	}
	printf("pkt ifindex %d\n", socket_address.sll_ifindex);
}


int main (int argc, char **argv){
	int staleness_in_seconds;
	int raw_socket;
	int domain_datagram_socket;
	int maxfd;	
	struct sockaddr_un odr_address;
	struct sockaddr_ll raw_address;  
	socklen_t odr_addr_len;
	socklen_t raw_addr_len;

	fd_set rset;

	if(argc!=2)
		err_quit("usage: odr_process <staleness in seconds>");
	staleness_in_seconds = atoi(argv[1]);


	get_hw_if();


	raw_socket = Socket(AF_PACKET, SOCK_RAW, htons(PROTO_NUM));
	domain_datagram_socket = Socket(AF_LOCAL, SOCK_DGRAM, 0);
	
	// Just your test
	// Just your test
	pf_send(raw_socket);
	pf_recv(raw_socket);
	// Just your test
	// Just your test

	bzero(&odr_address, sizeof(odr_address));
	odr_address.sun_family = AF_LOCAL;
	strcpy(odr_address.sun_path, ODR_SERVICE_PATH);

	Bind(domain_datagram_socket, (SA *) &odr_address, sizeof(odr_address));

	FD_ZERO(&rset);
	maxfd = max(domain_datagram_socket, raw_socket);

	while(1){
		FD_SET(domain_datagram_socket, &rset);
		FD_SET(raw_socket, &rset);

		if(select(maxfd+1, &rset, NULL, NULL, NULL) < 0){
			/* (never returns 0 on timeout) */
			perror("select less than 0");
			if(errno == EINTR)
				continue;
			err_sys("select failed");
		}

		if(FD_ISSET(domain_datagram_socket, &rset)){

			bzero(msg, sizeof(msg));
			odr_addr_len = sizeof(odr_address);
			how_much_read = Recvfrom(domain_datagram_socket, msg, sizeof(msg), 0, &odr_address, &odr_addr_len);
		    printf("Recieved from %s\n", odr_address.sun_path);
		    // keep track of sunpath's and stuff
		    // NOTE howmuchread and msg and stuff isnt made yet

		}else if(FD_ISSET(raw_socket, &rset)){
			bzero(msg, sizeof(msg));
			raw_addr_len = sizeof(raw_address);			
			how_much_read = Recvfrom(raw_socket, msg, sizeof(msg), 0, &raw_address, &raw_addr_len);
			// read http://unixhelp.ed.ac.uk/CGI/man-cgi?packet+7
			// int	    sll_ifindex;  /* Interface number */
			// that's the 
			// You will need to try out PF_PACKET sockets for yourselves and familiarize yourselves with how they behave. If, when you read from the socket and provide a sockaddr_ll structure, the kernel returns to you the index of the interface on which the incoming frame was received, then one socket will be enough.
		}

	}

	exit(0);
}



