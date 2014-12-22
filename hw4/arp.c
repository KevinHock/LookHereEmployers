#include "unp.h"
#include "hw_addrs.h"
#include "arp.h"

#include <linux/if_packet.h>
#include <linux/if_ether.h>

#define  PF_PROTO_VAL     0x3841 // ghi didn't check ghi
#define  ARP_PROTO_VAL    0x4789 // ghi didn't check ghi
#define  DEBUG 			  if ( debug ) printf
#define  MAX_VISITS       100
#define  WELLKNOWN_PATH   "/tmp/some0th3rd1r"
#define ETH_0_NAME  "eth0"
#define HARDTYPE_1_FOR_ETH 1
#define ARP_REQUEST_OP 1
#define ARP_REPLY_OP   2

unsigned char l_ip[4];
unsigned char l_mac[6];
int    l_if_index;
long debug = 1;

struct arp_frame
{
	uint16_t		hardtype;
	uint16_t		prottype;
	uint8_t			hardsize;
	uint8_t			protsize;
	uint16_t		op;
	unsigned char   sender_Ethernet_addr[6];
	// unsigned long   sender_IP_addr;
	unsigned char			sender_IP_addr[4];
	unsigned char	target_Ethernet_addr[6];
	// unsigned long	target_IP_addr;
	unsigned char			target_IP_addr[4];
};

typedef struct packet
{
	// in_addr_t payload[MAX_VISITS];
    //in_addr payload[MAX_VISITS];
    unsigned char payload[MAX_VISITS];
	int 	index;
}tour;

struct cache_entry
{
	unsigned char 		 		ip_address[4];
	unsigned char 		 		hw_address[6];
	int 		 		sll_ifindex;
	int 				c0nnfd;
	unsigned short 		sll_hatype;
	int                 incomplete;
	struct cache_entry  *next_entry;
};

struct cache_entry* cashay = NULL;

struct cache_entry* retrieve_from_cache(char ip_address[]){
	struct cache_entry *entry = cashay;
	while(entry != NULL){
		if(!strncmp(ip_address, entry->ip_address,4))
			return entry;
		entry = entry->next_entry;
	}
	return NULL;
}

void print_cache(void)
{
	struct cache_entry *entry = cashay;
	printf("    IP             MAC_ADDR\n");
	while(entry != NULL)
	{

		printf("%d.%d.%d.%d  ", entry->ip_address[0],entry->ip_address[1],entry->ip_address[2],
			entry->ip_address[3]);
		printf("  %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n",entry->hw_address[0],entry->hw_address[1],
			entry->hw_address[2],entry->hw_address[3],entry->hw_address[4],entry->hw_address[5]);
		entry = entry->next_entry;
	}
	return ;
}

void test_cache(void)
{
	unsigned char ip[4] ={130,130,130,1};
	unsigned char mac[6]={01,02,03,04,05,06};
	int i;
	for( i = 1; i < 10; i++)
	{
		ip[3] =i;
		insert_into_cache(ip, mac, 2,0,1);
	}
	print_cache();
}
void get_hw_if(void)
{
	struct hwa_info	*hwa,*hwahead;
	struct sockaddr	*sa;

	//printf("\n");    

	for (hwahead= hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next) 
	{
		if(!strncmp(hwa->if_name, ETH_0_NAME, strlen(ETH_0_NAME)))	
		{
			// save this ip address 
			if ((sa = hwa->ip_addr) != NULL)
			{	
				sscanf(Sock_ntop_host(sa, sizeof(*sa)), "%d.%d.%d.%d",&l_ip[0],&l_ip[1],
				&l_ip[2],&l_ip[3]);
				//printf("local_ip :%d.%d.%d.%d \n",l_ip[0],l_ip[1],
				//l_ip[2],l_ip[3]);
			}
			memcpy(l_mac, hwa->if_haddr,ETH_ALEN);
			//printf("local_mac: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n",l_mac[0],l_mac[1],l_mac[2],
				//l_mac[3],l_mac[4],l_mac[5]);
			l_if_index = hwa->if_index;
			//printf("local if_index %d\n", l_if_index);
			break;
		}
	}
	free_hwa_info(hwahead);
}

void insert_into_cache(char 		  *ip_address,
					   char 		  *hw_address,
					   int 			  sll_ifindex,
					   int 			  connfd,
					   unsigned short sll_hatype){

	struct cache_entry *entry = cashay;
	struct cache_entry *previous_entry = NULL;
	while(entry != NULL){
		if(!strncmp(entry->ip_address, ip_address,4)){
			//DEBUG("Complete arp cache for %d.%d.%d.%d \n", entry->ip_address[0],entry->ip_address[1],
				//entry->ip_address[2],entry->ip_address[3]);

			// ghi ???????????????? ghi
			memcpy(entry->ip_address, ip_address, sizeof(entry->ip_address));
			if(hw_address != NULL)
			{
				memcpy(entry->hw_address, hw_address, sizeof(entry->hw_address));
				entry->incomplete=0;
			}
			else
			{
				entry->incomplete=1;
			}
			//entry->sll_ifindex = sll_ifindex;
			//entry->c0nnfd = connfd;
			//entry->sll_hatype = sll_hatype;

			return;
		}
		previous_entry = entry;
		entry = entry->next_entry;
	}
	if(cashay == NULL)
	{
		cashay = (struct cache_entry*)Malloc(sizeof(struct cache_entry));
		memcpy(cashay->ip_address, ip_address, sizeof(entry->ip_address));
		if(hw_address != NULL)
		{
			memcpy(cashay->hw_address, hw_address, ETH_ALEN);
			cashay->incomplete=0;
		}
		else
		{
			cashay->incomplete=1;
		}
		cashay->sll_ifindex = sll_ifindex;
		cashay->c0nnfd = connfd;
		cashay->sll_hatype = sll_hatype;
		cashay->next_entry = NULL;
		return;
	}
	else
	{
		entry = (struct cache_entry*)malloc(sizeof(struct cache_entry));

		memcpy(entry->ip_address, ip_address, sizeof(entry->ip_address));
		if(hw_address != NULL){
			memcpy(entry->hw_address, hw_address, ETH_ALEN);
			entry->incomplete=0;
		}
		else
		{
			entry->incomplete=1;
		}
		entry->sll_ifindex = sll_ifindex;
		entry->c0nnfd = connfd;
		entry->sll_hatype = sll_hatype;
		entry->next_entry = NULL;
		if(previous_entry != NULL)
			previous_entry->next_entry = entry;
		return;
	}
}

void test_send(int sock)
{
	struct arp_frame frame;
	unsigned char dst_mac[6]={0xff,0xff,0xff,0xff,0xff,0xff};
	unsigned char dst_ip[4]={130,245,156,22};
	// fill in arp frame
	bzero(&frame, sizeof(frame));
	memcpy(frame.sender_IP_addr, l_ip, sizeof(frame.sender_IP_addr));
								
	memcpy(frame.target_IP_addr, dst_ip, sizeof(frame.target_IP_addr));
		// htons multibyte vars
	frame.hardtype = htons(HARDTYPE_1_FOR_ETH);
	frame.prottype = htons(ARP_PROTO_VAL);
	frame.op 	   = htons(ARP_REQUEST_OP);
	// Always the same for ARP 
	frame.hardsize = 6;
	frame.protsize = 4;
	// Broadcast
	//memcpy(dst_mac, ether_aton("ff:ff:ff:ff:ff:ff"), sizeof(dst_mac));
			
	send_arp_frame(sock, &frame, ARP_REQUEST_OP ,l_if_index, dst_mac);		
}

void send_arp_request(int pf_sock, unsigned char dst_ip[])
{
	struct arp_frame frame;
	unsigned char dst_mac[6]={0xff,0xff,0xff,0xff,0xff,0xff};
	// fill in arp frame
	bzero(&frame, sizeof(frame));
	memcpy(frame.sender_IP_addr, l_ip, sizeof(frame.sender_IP_addr));
								
	memcpy(frame.target_IP_addr, dst_ip, sizeof(frame.target_IP_addr));
		// htons multibyte vars
	frame.hardtype = htons(HARDTYPE_1_FOR_ETH);
	frame.prottype = htons(ARP_PROTO_VAL);
	frame.op 	   = htons(ARP_REQUEST_OP);
	// Always the same for ARP 
	frame.hardsize = 6;
	frame.protsize = 4;
	// Broadcast
	//memcpy(dst_mac, ether_aton("ff:ff:ff:ff:ff:ff"), sizeof(dst_mac));
	printf("Send Arp Request: ff:ff:ff:ff:ff:ff %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n",l_mac[0],l_mac[1],l_mac[2],l_mac[3],
		l_mac[4],l_mac[5]);
	printf("                 who has %u.%u.%u.%u tell %u.%u.%u.%u\n", dst_ip[0],dst_ip[1],dst_ip[2],dst_ip[3],
		l_ip[0],l_ip[1],l_ip[2],l_ip[3]);
	send_arp_frame(pf_sock, &frame, ARP_REQUEST_OP ,l_if_index, dst_mac);	
}
void send_arp_reply(int pf_sock, struct arp_frame* recieved_arp_frame)
{
	struct arp_frame frame;
	unsigned char dst_mac[6]={};
	unsigned char dst_ip[4]={};
	// fill in arp frame
	bzero(&frame, sizeof(frame));
	memcpy(frame.sender_IP_addr, l_ip, sizeof(frame.sender_IP_addr));
								
	memcpy(frame.target_IP_addr, recieved_arp_frame->sender_IP_addr, sizeof(frame.target_IP_addr));
	// htons multibyte vars
	frame.hardtype = htons(HARDTYPE_1_FOR_ETH);
	frame.prottype = htons(ARP_PROTO_VAL);
	frame.op 	   = htons(ARP_REPLY_OP);
	// Always the same for ARP 
	frame.hardsize = 6;
	frame.protsize = 4;
	// Broadcast
	memcpy(dst_mac, recieved_arp_frame->sender_Ethernet_addr, ETH_ALEN);
	//printf("send arp reply\n");
	printf("Send Arp reply: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n",dst_mac[0],dst_mac[1],dst_mac[2],dst_mac[3],
		dst_mac[4],dst_mac[5],l_mac[0],l_mac[1],l_mac[2],l_mac[3],l_mac[4],l_mac[5]);
	printf("               tell %u.%u.%u.%u that %u.%u.%u.%u at %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n", frame.target_IP_addr[0],frame.target_IP_addr[1],
		frame.target_IP_addr[2],frame.target_IP_addr[3],l_ip[0],l_ip[1],l_ip[2],l_ip[3], l_mac[0],l_mac[1],l_mac[2],l_mac[3],l_mac[4],l_mac[5]);
	send_arp_frame(pf_sock, &frame, ARP_REPLY_OP ,l_if_index, dst_mac);
}

int main(int argc, char **argv)
{
	// You do Step 1
	int pf_sock, unix_sock;
	struct sockaddr_un unixaddr, recvd_unixaddr;	
	struct sockaddr_ll recvd_pfaddr;	
	socklen_t pfaddrlen;
	socklen_t unixaddrlen;
	ssize_t how_much_read;
	int maxfd, connfd;
	fd_set rset;
	arp_msg msg;
	char recv_buff[MAXLINE];

	pf_sock = Socket(PF_PACKET, SOCK_RAW, htons(PF_PROTO_VAL));
	unix_sock = Socket(AF_UNIX,SOCK_STREAM,0);

	unlink(WELLKNOWN_PATH);
	bzero(&unixaddr, sizeof(unixaddr));
	unixaddr.sun_family = AF_UNIX;
	strcpy(unixaddr.sun_path, WELLKNOWN_PATH);
	Bind(unix_sock, (SA *) &unixaddr, sizeof(unixaddr));
	Listen(unix_sock, LISTENQ);

	get_hw_if();
	printf("eth0 : <%u.%u.%u.%u, %.2x:%.2x:%.2x:%.2x:%.2x:%.2x>\n",l_ip[0],l_ip[1],l_ip[2],l_ip[3],
		l_mac[0],l_mac[1],l_mac[2],l_mac[3],l_mac[4],l_mac[5]);
#if 1
	FD_ZERO(&rset);
	maxfd = max(pf_sock, unix_sock);	

	while(1)
	{
		FD_SET(pf_sock, &rset);
		FD_SET(unix_sock, &rset);

		if(select(maxfd+1, &rset, NULL, NULL, NULL) < 0){
			/* (never returns 0 on timeout) */
			if(errno == EINTR)
				continue;
			err_sys("select failed");
		}

		if(FD_ISSET(unix_sock, &rset)){
			bzero(&msg, sizeof(msg));
			unixaddrlen = sizeof(recvd_unixaddr);			
			connfd = Accept(unix_sock, (SA*)&recvd_unixaddr, &unixaddrlen);
			//printf("arp process receive connfd %d\n", connfd);


			if((how_much_read = Recv(connfd, &msg, sizeof(msg), 0)) > 0){

				struct hwaddr HWaddr;
				//sscanf(msg, "%s  %d  %s  %hu", IPstr, &(HWaddr.sll_ifindex), /* should be %u*/ &halen, &(HWaddr.sll_hatype));
				HWaddr.sll_ifindex = msg.sll_ifindex;
				HWaddr.sll_hatype  = msg.sll_hatype;
				//HWaddr.sll_halen   = msg.sll_halen;
				struct cache_entry* entry = retrieve_from_cache(msg.dst_ip);
				//printf("arp process receive request for %d.%d.%d.%d\n",msg.dst_ip[0], msg.dst_ip[1],
					//msg.dst_ip[2],msg.dst_ip[3]);
				if(entry == NULL){
					//DEBUG("We DID NOT find an entry!\n");

					insert_into_cache(msg.dst_ip, NULL, HWaddr.sll_ifindex, connfd, HWaddr.sll_hatype);
					//printf("insert arp cache incomplete!!!\n");
					send_arp_request(pf_sock, msg.dst_ip);
					
				}else{
					//DEBUG("We found an entry!\n");
					memcpy(msg.sll_addr, entry->hw_address, ETH_ALEN);
					Send(connfd, &msg, sizeof(msg), 0);
					Close(connfd);
				}
			}else{ /* error check */
				//printf("we recv'd %lu bytes\n", how_much_read);
			}

		}else if(FD_ISSET(pf_sock, &rset)){
			//bzero(&msg, sizeof(msg));
			bzero(&recvd_pfaddr, sizeof(recvd_pfaddr));
			pfaddrlen = sizeof(recvd_pfaddr);
			//how_much_read = Recvfrom(pf_sock, msg, sizeof(msg), 0, (SA*)&recvd_pfaddr, &pfaddrlen);
			how_much_read = Recvfrom(pf_sock, recv_buff, MAXLINE, 0, NULL, NULL);
		    //printf("Recieved from %d\n", recvd_pfaddr.sll_ifindex);

		    //printf("is this 14? %lu\n", sizeof(struct ethhdr));

			struct arp_frame recieved_arp_frame;
			// 14 is the ethhdr we dont care about since all the information in the ethhdr is in the arp_frame. and we only care about prottype and op
			memcpy(&recieved_arp_frame, recv_buff+14, sizeof(recieved_arp_frame));
			recieved_arp_frame.prottype = ntohs(recieved_arp_frame.prottype);
			recieved_arp_frame.op = ntohs(recieved_arp_frame.op);

			// print the request frame
			//printf("Just recieved this frame:\n");
			//print_raw_frame(&recieved_arp_frame);

			//int i;
			//for(i=0; i<14+ sizeof(struct arp_frame); i++){
			//	printf("%.2x ", recv_buff[i]);
			//	if((i-9) % 10 == 0) printf("\n");
			//	fflush(stdout);
			//}
			//printf("\n");

			if(recieved_arp_frame.op == ARP_REQUEST_OP)
			{
				//printf("receive arp request\n");
				
				if(!strncmp(recieved_arp_frame.target_IP_addr, l_ip, 4))
				{
					//receive targeted arp request
					//send back arp reply
					printf("Receive Arp Request: ff:ff:ff:ff:ff:ff %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n",recieved_arp_frame.sender_Ethernet_addr[0],recieved_arp_frame.sender_Ethernet_addr[1],
						recieved_arp_frame.sender_Ethernet_addr[2],recieved_arp_frame.sender_Ethernet_addr[3],recieved_arp_frame.sender_Ethernet_addr[4],recieved_arp_frame.sender_Ethernet_addr[5]);
					printf("                 who has %u.%u.%u.%u tell %u.%u.%u.%u\n", l_ip[0],l_ip[1],l_ip[2],l_ip[3],recieved_arp_frame.sender_IP_addr[0],
						recieved_arp_frame.sender_IP_addr[1],recieved_arp_frame.sender_IP_addr[2],recieved_arp_frame.sender_IP_addr[3]);
					send_arp_reply(pf_sock, &recieved_arp_frame);
				}
				else
				{
					;
				}
			}
			else if(recieved_arp_frame.op == ARP_REPLY_OP)
			{
				printf("Receive Arp reply: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n",l_mac[0],l_mac[1],l_mac[2],l_mac[3],l_mac[4],l_mac[5],recieved_arp_frame.sender_Ethernet_addr[0],
					recieved_arp_frame.sender_Ethernet_addr[1],recieved_arp_frame.sender_Ethernet_addr[2],recieved_arp_frame.sender_Ethernet_addr[3],recieved_arp_frame.sender_Ethernet_addr[4],recieved_arp_frame.sender_Ethernet_addr[5]);
				printf("               tell %u.%u.%u.%u that %u.%u.%u.%u at %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n", l_ip[0],l_ip[1],l_ip[2],l_ip[3],
				recieved_arp_frame.sender_IP_addr[0],recieved_arp_frame.sender_IP_addr[1],recieved_arp_frame.sender_IP_addr[2],recieved_arp_frame.sender_IP_addr[3],recieved_arp_frame.sender_Ethernet_addr[0],recieved_arp_frame.sender_Ethernet_addr[1],
				recieved_arp_frame.sender_Ethernet_addr[2],recieved_arp_frame.sender_Ethernet_addr[3],recieved_arp_frame.sender_Ethernet_addr[4],recieved_arp_frame.sender_Ethernet_addr[5]);
				
				insert_into_cache(recieved_arp_frame.sender_IP_addr,
					recieved_arp_frame.sender_Ethernet_addr,l_if_index,0,1);
				//printf("insert arp cache complete!!!\n");
				//print_cache();
				struct cache_entry* entry = retrieve_from_cache(recieved_arp_frame.sender_IP_addr);
				if(entry != NULL)
				{
					int connfd = entry->c0nnfd;
					//printf("retrieve from cache connfd %d\n", connfd);
					memcpy(msg.sll_addr, entry->hw_address, ETH_ALEN);
					Send(connfd, &msg, sizeof(msg), 0);
					Close(connfd);
				}
			}

		}
		#if 0
		else if(FD_ISSET(connfd, &rset)){
			bzero(msg, sizeof(msg));
			how_much_read = Recv(connfd, msg, MAXLINE, 0);
			// ???
			printf("????????????????????????\n");
			printf("????????????????????????\n");
			printf("????????????????????????\n");
			printf("msg = %s\n", msg);
			printf("????????????????????????\n");
			printf("????????????????????????\n");
			printf("????????????????????????\n");
		}
		#endif
	}

	

#endif
}
int send_arp_frame(
	int 		    sock, 	
	struct arp_frame 	    *frame, 
	int type,//1 request; 2: reply
	int 		    if_index, 
	unsigned char   *dst_mac)
{
	/*target address*/
	struct sockaddr_ll socket_address;

	/*buffer for ethernet frame*/
	unsigned char buffer[ETH_FRAME_LEN];
 
	/*pointer to ethenet header*/
	unsigned char* etherhead = buffer;
	
	/*userdata in ethernet frame*/
	unsigned char* data = buffer + 14;
	
	/*another pointer to ethernet header*/
	struct ethhdr *eh = (struct ethhdr *)etherhead;
 
	size_t frame_len = 0;
	unsigned char src_mac[ETH_ALEN];
	unsigned char dest_mac[ETH_ALEN];

	//get_mac_by_if_index(if_index,src_mac);
	memcpy(src_mac, l_mac, ETH_ALEN);

	memcpy(frame->sender_Ethernet_addr,src_mac, ETH_ALEN);
	if(type == ARP_REQUEST_OP)
	{
		memset(dest_mac, 0xff, ETH_ALEN);
	}
	else
	{
		memcpy(dest_mac, dst_mac,ETH_ALEN);
		memcpy(frame->target_Ethernet_addr, dst_mac, ETH_ALEN );
	}
	//memcpy(dst_mac, frame->target_Ethernet_addr, sizeof(frame->target_Ethernet_addr));

	/*RAW communication*/
	socket_address.sll_family   = PF_PACKET;

	
	//socket_address.sll_protocol = htons(ETH_P_IP);	
	//socket_address.sll_hatype   = ARPHRD_ETHER;
	//socket_address.sll_pkttype  = PACKET_OTHERHOST;

	/*index of the network device
	see full code later how to retrieve it*/
	socket_address.sll_ifindex  = if_index;

	//socket_address.sll_pkttype  = PACKET_OTHERHOST;
	/*address length*/
	socket_address.sll_halen    = ETH_ALEN;		
	/*MAC - begin*/
	socket_address.sll_addr[0]  = dest_mac[0]; 	
	socket_address.sll_addr[1]  = dest_mac[1];		
	socket_address.sll_addr[2]  = dest_mac[2]; 
	socket_address.sll_addr[3]  = dest_mac[3]; 
	socket_address.sll_addr[4]  = dest_mac[4]; 
	socket_address.sll_addr[5]  = dest_mac[5];
	/*MAC - end*/
	socket_address.sll_addr[6]  = dest_mac[6] = 0x00;/*not used*/
	socket_address.sll_addr[7]  = dest_mac[7] = 0x00;/*not used*/

	/*set the frame header*/
	memcpy((void*)buffer, (void*)dest_mac, ETH_ALEN);
	memcpy((void*)(buffer+ETH_ALEN), (void*)src_mac, ETH_ALEN);
	eh->h_proto = htons(PF_PROTO_VAL); 

	memcpy(data, frame, sizeof(struct arp_frame ));
	frame_len = 14 + sizeof(struct arp_frame );


	//printf("send arp frame:\n");
	//int i;
	//for(i=0; i<14+ sizeof(struct arp_frame); i++){
	//	printf("%.2x ", buffer[i]);
	//	if((i-9) % 10 == 0) printf("\n");
	//	fflush(stdout);
	//}
	//printf("\n");
	Sendto(sock, buffer, frame_len/*ETH_FRAME_LEN*/, 0, 
	      (struct sockaddr*)&socket_address, sizeof(socket_address));
	return 0;
}