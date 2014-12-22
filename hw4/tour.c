#include "tour.h"
#include <netinet/ip_icmp.h>
ip_vm vm_table[]=
{
	{"130.245.156.21", "vm1"},
	{"130.245.156.22", "vm2"},
	{"130.245.156.23", "vm3"},
	{"130.245.156.24", "vm4"},
	{"130.245.156.25", "vm5"},
	{"130.245.156.26", "vm6"},
	{"130.245.156.27", "vm7"},
	{"130.245.156.28", "vm8"},
	{"130.245.156.29", "vm9"},
	{"130.245.156.20", "vm10"}
};

char* find_vm_name(const char* ip)
{
	int i = 0;
	int size = sizeof(vm_table)/sizeof(ip_vm);

	for(i = 0; i < size; i++)
	{
		if(!memcmp(vm_table[i].ip, ip, strlen(ip)))
		{
			return vm_table[i].name;
		}
	}
	return NULL;
}
unsigned short cksum(unsigned short* buffer, int size)
{
	unsigned long cksum=0;  
    while(size>1)  
    {  
        cksum+=*buffer++;  
        size-=sizeof(unsigned short );  
    }  
    if(size)  
    {  
        cksum+=*(unsigned char *)buffer;  
    }   
    while (cksum>>16)  
        cksum=(cksum>>16)+(cksum & 0xffff);  
    return (unsigned short ) (~cksum);  
}
int main(int argc, char **argv){
	const int  on = 1;
	unsigned char ttl = 1;
	time_t 	   rawtime;
	time_t 	   lasttime;
	int 	   maxfd;
	fd_set 	   rset;
	ssize_t	   how_much_read;
	char	   msg[MAXLINE];

	rt 			   = Socket(AF_INET, SOCK_RAW, MY_IP_PROTO);
	Setsockopt(rt, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on));

	pg			   = Socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	request_sock   = Socket(PF_PACKET, SOCK_RAW, htons(ETH_P_IP));
	multicast_sock = Socket(AF_INET, SOCK_DGRAM, 0);
	multicast_recv = Socket(AF_INET, SOCK_DGRAM, 0);
	Mcast_set_loop(multicast_sock,0);
	Setsockopt(multicast_sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));
	Setsockopt(multicast_recv, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));

	gethostname(our_hostname, sizeof(our_hostname));
	//printf("My our_hostname: %s\n", our_hostname);
	get_eth0_stuff();
	if(argc > 1)
	{
		//DEBUG("%s is the start of the tour!\n",our_hostname);

		int   i = 1;		
		tour  tour_pkt;
		
		// separate func
		bzero(&tour_pkt, sizeof(tour_pkt));

		fill_buff_with_ip_of_hostname(our_hostname);
		//printf("fill buff ok\n");
		inet_aton(ip_static_buff, &(tour_pkt.payload[0]));
		join_mcast(MULTICAST_ADDRESS, MULTICAST_PORT);
		#if 0
		// tokenize by ' ' and send each char* to gethostbyname
		char 		*token, *prev_token;
		const char  s[2] = " ";

		/* get the first token */
		token = strtok(argv[1], s);

		/* walk through other tokens */
		while(token != NULL)
		{
			DEBUG("token = %s\n", token );

			fill_buff_with_ip_of_hostname(token);
			inet_aton(ip_static_buff, &tour_pkt.payload[i]);

			prev_token  = token;
			token 		= strtok(NULL, s);
			if(!strcmp(prev_token, token)){
				perror("The same node should not appear consequentively in the tour list – i.e., the next node on the tour cannot be the current node itself");
				exit(0);
			}

			i++;
		}
		// separate func
		#endif
		while(i< argc)
		{
			//printf("argv[%d] is %s\n",i, argv[i]);
			fill_buff_with_ip_of_hostname(argv[i]);
			//printf("fill buff ok\n");
			inet_aton(ip_static_buff, &(tour_pkt.payload[i]));
			i++;
		}
		tour_pkt.index = 0; // We're the origin
		tour_pkt.total = i-1;
		//printf("tour_pkt total %d\n", tour_pkt.total);
		send_raw_tour_packet(&tour_pkt);

	}else{
		//DEBUG("We're NOT the start of the tour!");
		#if 0
		while(1)
		{
			int len = 0;
			len = Recv(rt, msg, sizeof(msg), 0);
			printf("received len %d\n",len);
			int j = 0;
			for(;j<len; j++)
			{
				printf("%.2x ", msg[j]);
				if((i-9) % 10 == 0) printf("\n");
				fflush(stdout);
			}
		}
		#endif
	}
	#if 0
	fill_buff_with_ip_of_hostname("vm2");
	inet_aton(ip_static_buff, (struct in_addr *)&dest_of_echo_req);
	printf("destofechoreq %x\n", dest_of_echo_req.s_addr);
	struct sockaddr_in IPaddr;
	struct hwaddr 	   HWaddr;
	bzero(&IPaddr, sizeof(IPaddr));
	bzero(&HWaddr, sizeof(HWaddr));
	IPaddr.sin_addr = dest_of_echo_req;
	//Areq((struct sockaddr *)&IPaddr, sizeof(struct sockaddr_in), &HWaddr);
	//printf("%.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n",HWaddr.sll_addr[0],HWaddr.sll_addr[1],HWaddr.sll_addr[2],
	//			HWaddr.sll_addr[3],HWaddr.sll_addr[4],HWaddr.sll_addr[5]);
	void sig_alrm(int signo);
	if(argc > 1){
	Signal(SIGALRM, sig_alrm);
	sig_alrm(SIGALRM);
	}
	//send_icmp_request(HWaddr.sll_addr);
	int n  = 0;
	struct timeval tval;
	
	while(1)
	{
		n = recv(pg, msg, sizeof(msg), 0);
		if(n<0)
		{
			if (errno == EINTR)
				continue;
		}
		printf("receive ping packet\n");
		//int j = 0;
		//for (; j < n; j++)
		//{
		//	printf("%.2x ", msg[j]);
		//	if((j-9) % 10 == 0) printf("\n");
		//	fflush(stdout);
		//}
		Gettimeofday(&tval, NULL);
		proc_v4(msg,n,NULL, &tval);
	}
	#endif
	#if 1
    //act_open.sa_flags = 0;
    //sigemptyset(&act_open.sa_mask);
    //sigaddset(&act_open.sa_mask, SIGALRM);
    //act_open.sa_handler = terminate;
	Signal(SIGALRM, sig_alrm);
	FD_ZERO(&rset);
	maxfd = max(pg, multicast_recv);
	maxfd = max(rt, maxfd);

	while(1){
		FD_SET(multicast_recv, &rset);
		FD_SET(rt, &rset);
		FD_SET(pg, &rset);

		if(select(maxfd+1, &rset, NULL, NULL, NULL)<0)
		{
			if(errno == EINTR)
			{
				continue;
			}
		}

		if(FD_ISSET(rt, &rset)){
			//DEBUG("possibly totally wrong here\n");
			//DEBUG("possibly totally wrong here\n");
			//DEBUG("possibly totally wrong here\n");
			alarm(0);
			bzero(msg, sizeof(msg));
			how_much_read = Recv(rt, msg, sizeof(msg), 0);
		

			//assert(how_much_read == sizeof(sizeof(my_ip_header)+sizeof(tour)));
			//printf("rt receive pkt len %d\n", how_much_read);
			my_ip_header  *head     =  (my_ip_header *)msg;
			tour 		  *tour_pkt =  (tour *)(msg+sizeof(my_ip_header));
			
			if(ntohs(head->id) != MY_IP_ID){
				//DEBUG("head->id != MY_IP_ID\n");
				continue;
			}
			char *name = find_vm_name(inet_ntoa(*(struct in_addr *)&head->saddr));
			time(&rawtime);
			printf("<%s> recieved source routing packet from <%s>\n", ctime (&rawtime), name);
			
			if(!already_here){
				already_here = 1;
				// IP_MULTICAST_TTL - If not otherwise specified, has a default value of 1.
				join_mcast(inet_ntoa(tour_pkt->multicast_addr), ntohs(tour_pkt->multicast_port));
				stop_pinging = 0;
			}
				// start icmp echo reqs
			for(i = 0; i < prev_sender_list.index; i++)
			{
				if(!memcmp(&prev_sender_list.previous_senders[i], &tour_pkt->payload[(tour_pkt->index)], sizeof(struct in_addr)))
					we_dont_need_to_send_echo_req = 1;
				else{
					we_dont_need_to_send_echo_req = 0;
					break;
				}
			}

			if(!we_dont_need_to_send_echo_req){
				//DEBUG("We Need To Send Echo Request\n");

				prev_sender_list.previous_senders[prev_sender_list.index] = tour_pkt->payload[(tour_pkt->index)];
				prev_sender_list.index++;

				dest_of_echo_req = tour_pkt->payload[(tour_pkt->index)];

				char *name = find_vm_name(inet_ntoa(*(struct in_addr *)&tour_pkt->payload[(tour_pkt->index)]));
				printf("PING %s (%s): %d data bytes\n",name,inet_ntoa(dest_of_echo_req), icmpdata);
				sig_alrm(SIGALRM);
			}
			else
			{
				char *name = find_vm_name(inet_ntoa(*(struct in_addr *)&tour_pkt->payload[(tour_pkt->index)]));
				printf("We have ping %s before\n",name);
			}
			

			// INCREMENTING THE INDEX
			tour_pkt->index = (tour_pkt->index)+1;
			if((tour_pkt->index) == MAX_VISITS){
				perror("MAX_VISITS has been reached.");
				exit(0);
			}

			// Now blank
			//bzero(&for_comparisons, sizeof(for_comparisons));
			// ghiTODO this may not work ghiTODO
			//if(!memcmp(&tour_pkt->payload[ntohl(tour_pkt->index)], &for_comparisons, sizeof(for_comparisons))){
			//printf("rt current pkt index %d, total index %d\n",tour_pkt->index, tour_pkt->total);
			if(tour_pkt->index == tour_pkt->total)
			{
				//printf("We're the final destination.\n");
				// ping the saddr
				final_destination = 1;
				char buffer[100];
				snprintf(buffer, 100, "<<<<< This is node %s.  Tour has ended.  Group members please identify yourselves. >>>>>",our_hostname);
				printf("Node %s.  Sending: <%s>.  ",our_hostname, buffer);
				send_multicast_message(buffer,strlen(buffer)+1);
				//printf("send mulitcast msg ok\n");

			}else{
				//printf("We're NOT the final destination, keep touring.\n");
				// pass it along
				send_raw_tour_packet(tour_pkt);
			}


		}else if(FD_ISSET(multicast_recv, &rset)){
			//DEBUG("Receiving multicast message.\n");
			printf("");
			bzero(msg, sizeof(msg));
			how_much_read = Recv(multicast_recv, msg, sizeof(msg), 0);
			//printf("multicast msg len %d\n", how_much_read);
			msg[how_much_read]='\0';
			if(!first_term){
				printf("Node %s.  Received <%s>.  ",our_hostname,msg);
				// then should immediately stop ping 
				//DEBUG("how_much_read is %lu\n", how_much_read);
				stop_pinging = 1;
				alarm(0);
				bzero(msg, sizeof(msg));
				snprintf(msg, sizeof(msg), "<<<<< Node %s I am a member of the group. >>>>>", our_hostname);
				printf("Node %s. Sending <%s>.  ", our_hostname, msg);
				fflush(stdout);
				first_term = 1;
				send_multicast_message(msg,strlen(msg)+1);
			}
			else
			{
				Signal(SIGALRM, terminate);
				alarm(5);
				printf("");
				printf("Node %s.  Received <%s>.  ",our_hostname,msg);
				fflush(stdout);
			}

			//send_multicast_message(msg);
		}else if(FD_ISSET(pg, &rset)){
			//DEBUG("Receiving echo reply message.\n");
			struct timeval tval;
			bzero(msg, sizeof(msg));
			how_much_read = Recv(pg, msg, sizeof(msg), 0);
			Gettimeofday(&tval, NULL);
			proc_v4(msg,how_much_read,NULL, &tval);
			//assert(how_much_read == sizeof(sizeof(my_eth_header)+sizeof(my_ip_header)+sizeof(my_icmp_header)));
			//DEBUG("how_much_read is %lu\n", how_much_read);
			//DEBUG("Node %s recieved %s\n", our_hostname, msg);
			// my_eth_header   *eh    =  (my_eth_header  *)msg;
			// my_ip_header    *iph   =  (my_ip_header   *)msg+sizeof(my_eth_header);
			// my_icmp_header  *icmph =  (my_icmp_header *)msg+sizeof(my_eth_header)+sizeof(my_ip_header);
			//my_ip_header *iph = (my_ip_header *)(msg+sizeof(my_eth_header));
			//if(!memcmp(&iph->saddr, &dest_of_echo_req, sizeof(dest_of_echo_req))){
			//	DEBUG("ghiTODO\n");
			//	count_of_replies++;
			//	DEBUG("ghiTODO\n");
				//DEBUG("ghiTODO\n");
				//DEBUG("ghiTODO\n");
			//	if(final_destination && count_of_replies >= 5){
			//		stop_pinging = 1;
			//		bzero(msg, sizeof(msg));
			///		snprintf(msg, sizeof(msg), "<<<<< This is node %s Tour has ended. Group members please identify yourselves.>>>>>", our_hostname);
			//		printf("Node %s sending %s", our_hostname, msg);
			//		send_multicast_message(msg, strlen(msg)+1);
			//		alarm(5);
			//	}
			//}else{
				//DEBUG("The following is FALSE !memcmp(&iph->saddr, &dest_of_echo_req, sizeof(dest_of_echo_req))");
			//}
		}
		#if 0
		time(&rawtime);
		if(lasttime+1 <= rawtime){
			if(!stop_pinging){	
				struct sockaddr_in IPaddr;
				struct hwaddr 	   HWaddr;
				bzero(&IPaddr, sizeof(IPaddr));
				bzero(&HWaddr, sizeof(HWaddr));
				IPaddr.sin_addr = dest_of_echo_req;
				Areq((struct sockaddr *)&IPaddr, sizeof(struct sockaddr_in), &HWaddr);
				send_raw_echo_request_message(HWaddr.sll_addr);
			}

		}else{
			lasttime = rawtime;
		}
		#endif

	}
	#endif
}

void send_raw_tour_packet(tour *tour_pkt){
	// Assuming tour_pkt has everything in network order
	
	//unsigned char  buffer[sizeof(my_ip_header)+sizeof(tour)]={};
	unsigned char buffer[MAXLINE];
	int length = sizeof(my_ip_header)+sizeof(tour);
	//printf("length %d\n",length);
	my_ip_header 		head;
	struct sockaddr_in  dest_addr;

	bzero(buffer, sizeof(buffer));

	//head.ihl       =  5;
	//head.version   =  4;
	head.v_hl = (4<<4)+5;
	head.tos 	   =  0;
	head.tot_len   =  htons(length);
	head.id  	   =  htons(MY_IP_ID);
	head.fragment  =  0;
	head.ttl       =  MAX_VISITS;
	head.protocol  =  MY_IP_PROTO;
	head.check     =  0;
	fill_buff_with_ip_of_hostname(our_hostname);
	inet_aton(ip_static_buff, (struct in_addr *)&head.saddr);
	head.daddr     =  get_uint_destination_of_tour(tour_pkt);
	//printf("head daddr: %x\n", head.daddr);
	//printf("head saddr: %x\n", head.saddr);
	//unsigned char *b=(unsigned char*)&(head.saddr);
	//printf("head daddr: %d.%d.%d.%d\n", b[0],b[1],b[2],b[3]);
	tour_pkt->multicast_port = htons(MULTICAST_PORT);
	inet_aton(MULTICAST_ADDRESS, &tour_pkt->multicast_addr);

	memcpy(buffer, &head, sizeof(head));
	memcpy(buffer+sizeof(head), tour_pkt, sizeof(tour));

	bzero(&dest_addr, sizeof(dest_addr));
	dest_addr.sin_family =  AF_INET;
	// ghiTODO what port ghiTODO
	dest_addr.sin_addr   =  tour_pkt->payload[(tour_pkt->index)+1];

	int j = 0;

	//for(;j < length; j++)
	//{
	//	printf("%.2x ", buffer[j]);
	//	if((j-9) % 10 == 0) printf("\n");
	//	fflush(stdout);
	//}
	Sendto(rt, buffer, length, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
}
void send_multicast_message(char *msg, int len){
	struct sockaddr_in dest_addr;

	bzero(&dest_addr, sizeof(dest_addr));
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(MULTICAST_PORT);
	inet_aton(MULTICAST_ADDRESS, &dest_addr.sin_addr);

	 Sendto(multicast_sock, msg, len, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
}

void fill_buff_with_ip_of_hostname(char *name){
    struct hostent *he;
    struct in_addr **addr_list;

	if ((he = gethostbyname(name)) == NULL) {  // get the host info
        herror("gethostbyname");
        exit(0);
    }
    addr_list = (struct in_addr **)he->h_addr_list;
    //printf("fill_buff_with_ip: %s\n", inet_ntoa(*addr_list[0]));
    strncpy(ip_static_buff, inet_ntoa(*addr_list[0]), sizeof(ip_static_buff));
}

uint32_t get_uint_destination_of_tour(tour *tour_pkt){
	struct in_addr dest;
	uint32_t ret_me;
	//printf("get_uint_dest_tour: tour index: %d\n",ntohl(10));
	dest = tour_pkt->payload[(tour_pkt->index)+1];
	//printf("get_unit_dest addr %x\n", dest);
	memcpy(&ret_me, &dest, sizeof(ret_me));

	return (ret_me);
}


void join_mcast(char *address_to_join, int port_to_bind){
	struct sockaddr_in  mcast_addr;
	// struct ip_mreq		mreq;

	//bzero(&mcast_addr, sizeof(mcast_addr));
	//mcast_addr.sin_family 	   =  AF_INET;
	//mcast_addr.sin_port 	   =  htons(port_to_bind);
	//mcast_addr.sin_addr.s_addr =  htonl(INADDR_ANY);

	//Bind(multicast_sock, (struct sockaddr *)&mcast_addr, sizeof(mcast_addr));

	// inet_aton(MULTICAST_ADDRESS, &mreq.imr_multiaddr);
	// mreq.imr_interface.s_addr = htonl(INADDR_ANY); /*ghiTODO eth0 maybe ghiTODO*/
	// Setsockopt(multicast_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
	//printf("join mcast: multi addr %s and port %d\n", address_to_join,port_to_bind);
	bzero(&mcast_addr, sizeof(mcast_addr));
	mcast_addr.sin_family 	   =  AF_INET;
	mcast_addr.sin_port 	   =  htons(port_to_bind);
	if(address_to_join == NULL)
		inet_aton(MULTICAST_ADDRESS, &mcast_addr.sin_addr);
	else
		inet_aton(address_to_join, &mcast_addr.sin_addr);
	Bind(multicast_recv, (struct sockaddr *)&mcast_addr, sizeof(mcast_addr));
// what interface what port

	Mcast_join(multicast_recv, &mcast_addr, sizeof(mcast_addr), NULL, 0);
}

void get_eth0_stuff(){
	struct hwa_info	*hwa,*hwahead;
	struct sockaddr	*sa;

	for(hwahead= hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next){
		if(!strncmp(hwa->if_name, ETH_0_NAME, strlen(ETH_0_NAME))){
			if((sa = hwa->ip_addr) != NULL){	
				memcpy(eth0_mac, hwa->if_haddr,ETH_ALEN);
			}else{
				//DEBUG("\n(sa = hwa->ip_addr) is NULL\n");
			}
			eth0_if = hwa->if_index;
		}
	}
	free_hwa_info(hwahead);
	//printf("local_mac: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n",eth0_mac[0],eth0_mac[1],eth0_mac[2],
				//eth0_mac[3],eth0_mac[4],eth0_mac[5]);
	//printf("local if_index %d\n", eth0_if);
}
void sig_alrm(int signo)
{
	struct sockaddr_in IPaddr;
	struct hwaddr 	   HWaddr;
	bzero(&IPaddr, sizeof(IPaddr));
	bzero(&HWaddr, sizeof(HWaddr));
	IPaddr.sin_addr = dest_of_echo_req;
	Areq((struct sockaddr *)&IPaddr, sizeof(struct sockaddr_in), &HWaddr);
	//vm2: 00:0c:29:d9:08:ec
	//unsigned char mac[6]={0x00,0x0c,0x29,0xd9,0x08,0xec};
	send_icmp_request(HWaddr.sll_addr);
	alarm(1);
	return;
}
void tv_sub(struct timeval* out, struct timeval *in)
{
	if((out->tv_usec -= in->tv_usec)<0)
	{
		--out->tv_sec;
		out->tv_usec += 1000000;
	}
	out->tv_sec -= in->tv_sec;
}
void proc_v4(char *ptr, ssize_t len, struct msghdr *msg, struct timeval *tvrecv)
{
	int hlen1, icmplen;
	double rtt;
	my_ip_header *ip;
	struct icmp *icmp;
	struct timeval *tvsend;

	ip = (my_ip_header*)ptr;
	hlen1 = (ip->v_hl&0xf)<<2;
	if(ip->protocol != IPPROTO_ICMP)
	{
		return;
	}

	icmp = (struct icmp*)(ptr+hlen1);
	icmplen =  len - hlen1;

	if(ntohs(icmp->icmp_id) != MY_ICMP_ID)
	{
		//printf("unkown icmp_id %x\n", ntohs(icmp->icmp_id));
		return;
	}
	//int j = 0;
	//for (; j < len; j++)
	//{
	//	printf("%.2x ", ptr[j]);
	//	if((j-9) % 10 == 0) printf("\n");
	//	fflush(stdout);
	//}
	if(icmp->icmp_type == ICMP_ECHOREPLY)
	{

		tvsend = (struct timeval*)icmp->icmp_data;
		tv_sub(tvrecv, tvsend);
		rtt = tvrecv->tv_sec*1000.0 + tvrecv->tv_usec/1000.0;

		printf("%d bytes from %s: seq=%u, ttl=%d, rtt=%.3f ms\n",
			icmplen, inet_ntoa(dest_of_echo_req),ntohs(icmp->icmp_seq), ip->ttl, rtt);
	}
}
void send_icmp_request(unsigned char *dst_mac)
{
	static int nset= 0;
	unsigned char buffer[MAXLINE]={};
	int icmplen = 8+icmpdata;
	int length = sizeof(my_ip_header)+sizeof(my_eth_header)+icmplen;
	int ip_len = sizeof(my_ip_header)+icmplen;
	my_eth_header		eh;
	my_ip_header 		iph;
	struct icmp		    *icmp;
	unsigned char       sendbuf[MAXLINE];


	icmp= (struct icmp*)sendbuf;
	struct sockaddr_ll  dest_addr;

	bzero(buffer, sizeof(buffer));

	memcpy(eh.h_dest, dst_mac, ETH_ALEN);	
	memcpy(eh.h_source, eth0_mac, ETH_ALEN);	
	eh.h_proto = htons(ETH_P_IP);

	//iph.ihl      =  5;
	//iph.version  =  4;
	iph.v_hl = (4<<4)+5;
	iph.tos 	 =  0;
	iph.tot_len  =  htons(ip_len);
	iph.id  	 =  htons(MY_IP_ID);
	iph.fragment =  0;
	iph.ttl      =  MAX_VISITS;
	iph.protocol =  IPPROTO_ICMP;
	iph.check    = 0;
	iph.check    = cksum(&iph, 20);//htons((in_cksum((unsigned short *)&iph,20)));
	fill_buff_with_ip_of_hostname(our_hostname);
	inet_aton(ip_static_buff, (struct in_addr *)&iph.saddr);
	memcpy(&iph.daddr, &dest_of_echo_req, sizeof(iph.daddr));

	//icmph.icmph_type     =  MY_ICMP_ECHO;
	//icmph.icmph_code     =  0;
	//icmph.icmph_checksum =  0;
	//icmph.icmph_id       =  MY_ICMP_ID;
	//icmph.icmph_sequence =  nset++;
	//icmph.icmph_checksum = in_cksum((unsigned short*)&icmph, 8+icmpdata);

	icmp->icmp_type = ICMP_ECHO;
	icmp->icmp_code = 0;
	icmp->icmp_id  = htons(MY_ICMP_ID);
	icmp->icmp_seq = htons(nset++);

	memset(icmp->icmp_data, 0xa5, icmpdata);

	Gettimeofday((struct timeval*)(icmp->icmp_data), NULL);
	
	icmp->icmp_cksum = 0;
	icmp->icmp_cksum  = (in_cksum((unsigned short*)icmp, icmplen));
	

	//printf("echo request length %d\n", length);
	memcpy(buffer, &eh, sizeof(eh));
	memcpy(buffer+sizeof(eh), &iph, sizeof(iph));
	memcpy(buffer+sizeof(eh)+sizeof(iph), icmp, icmplen);

	bzero(&dest_addr, sizeof(dest_addr));
	dest_addr.sll_family   =  PF_PACKET;
	dest_addr.sll_protocol =  htons(ETH_P_IP);	
	dest_addr.sll_ifindex  =  eth0_if;
	// dest_addr.sll_hatype   =  ARPHRD_ETHER;
	// dest_addr.sll_pkttype  =  PACKET_OTHERHOST;
	dest_addr.sll_halen    =  ETH_ALEN;
	memcpy(dest_addr.sll_addr, dst_mac, ETH_ALEN);
	//int j =0;
	//for(;j< length; j++)
	//{
	//	printf("%.2x ", buffer[j]);
	//	if((j-9) % 10 == 0) printf("\n");
	//	fflush(stdout);
	//}
	Sendto(request_sock, buffer, length, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
}

void terminate(int sig){
    printf("Tour module is now exiting\n");
    exit(0);
}