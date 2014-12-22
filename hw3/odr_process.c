#include "unp.h"
#include "asgn3.h"
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include <stdlib.h>
#include "hw_addrs.h"
#include "odr_process.h"












// put in .h

#define DEBUG if ( debug ) printf
#define PROTO_NUM 0x8182
#define ETH_FRAME_LEN 114

long debug = 1;
time_t staleness_in_seconds;
struct sockaddr_un odr_address;
route_table* g_route_table = NULL;
typedef struct sun_path_info
{
	char sun_path[1024];
	time_t ts;

	//ghi
}sun_path_info;

int quote_port_quote;
sun_path_info infoz[1024];

struct hwa_info* hw_list = NULL;
// put in .h




int hash_key(unsigned char ip[])
{
	return ip[3] % 10;
}

route_table* create_route_table()
{
	g_route_table = (route_table*)malloc(sizeof(route_table));
	memset(g_route_table, 0, sizeof(route_table));
	return g_route_table;
}

int insert_data_into_hash(route_table* p_route_table, route_info* data)
{
	route_index* pnode;
	if(NULL == p_route_table)
	{
		return -1;
	}
	int key = hash_key(data->dst_ip);
	if(NULL == p_route_table->value[key])
	{
		pnode = (route_index*)malloc(sizeof(route_index));
		memset(pnode, 0, sizeof(route_index));
		memcpy(&(pnode->data), data, sizeof(route_info));
		p_route_table->value[key] = pnode;
		return 0;
	}

	if(NULL != find_dstip_in_hash(p_route_table, data->dst_ip))
	{
		return -1;
	}

	pnode = p_route_table->value[key];
	while(NULL != pnode->next)
	{
		pnode = pnode->next;
	}
	pnode->next = (route_index*)malloc(sizeof(route_index));
	memset(pnode->next,0, sizeof(route_index));
	memcpy(&(pnode->next->data), data, sizeof(route_info));
	return 0;
}

int delete_data_from_hash(route_table *p_route_table, route_info *data)
{
	route_index *phead, *pnode;
	int key = hash_key(data->dst_ip);
	if(NULL == p_route_table || NULL == p_route_table->value[key])
	{
		return -1;
	}

	if(NULL == (pnode = find_dstip_in_hash(p_route_table, data->dst_ip)))
	{
		return -1;
	}

	if(pnode == p_route_table->value[key])
	{
		p_route_table->value[key] = pnode->next;
		goto final;
	}

	phead = p_route_table->value[key];
	while(pnode != phead->next)
	{
		phead = phead->next;
	}
	phead->next = pnode->next;

final:
	free(pnode);
	return 0;
}

void print_route_info(route_info *info)
{
	printf("%d.%d.%d.%d  ",info->dst_ip[0],info->dst_ip[1],info->dst_ip[2],info->dst_ip[3]);
	printf("     %d    ", info->next_hop_if);
	printf("    %.2x:%.2x:%.2x:%.2x:%.2x:%.2x", info->next_hop_addr[0],info->next_hop_addr[1],
		info->next_hop_addr[2],info->next_hop_addr[3],info->next_hop_addr[4],info->next_hop_addr[5]);
	printf("      %d   ", info->hop_count);
	printf("       %d ", info->broadcast_id);
	printf("    %d\n", info->record_time);
}


void print_route_table(void)
{
	route_index *node;
	int i;
	printf("   DstIp      Next_hop_if     Next_hop_addr    Hop_count   BroadId   Time\n");
	for(i =0; i < ROUTE_TABLE_SIZE; i++)
	{
		node = g_route_table->value[i];
		while(node != NULL)
		{
			print_route_info(&(node->data));
			node = node->next;
		}
	}
}

void get_mac_by_if_index(int if_index, unsigned char srcmac[])
{
	struct hw_info	*hw = hw_list;
	while(hw != NULL)
	{
		if(hw->if_index ==  if_index)
		{
			memcpy(srcmac, hw->if_haddr,ETH_ALEN);
			return;
		}
		hw = hw->hw_next;
	}
}

void print_rreq(void *buffer)
{
	pkt_rreq *data =(pkt_rreq*)buffer;

	printf("pkt Tyep: rreq\n");
	printf("rrep_set :  	%d\n",data->rrep_set);
	printf("hop_count : 	%d\n", data->hop_count);
	printf("force_flg : 	%d\n", data->force_flg);
	printf("broad id  :     %d\n", data->broadcast_id);
	printf("dst_ip :        %d.%d.%d.%d\n",data->dst_ip[0],data->dst_ip[1],
		                            data->dst_ip[2],data->dst_ip[3]);
	printf("src_ip :        %d.%d.%d.%d\n",data->src_ip[0],data->src_ip[1],
		                            data->src_ip[2],data->src_ip[3]);
	printf("dst_port:       %d\n", data->dst_port);
	printf("src_port:       %d\n", data->src_port);
	return;
}

void print_rrep(void *buffer)
{
	pkt_rrep *data = (pkt_rrep*)buffer;
	printf("pkt Tyep: rrep\n");
	printf("hop_count : 	%d\n", data->hop_count);
	printf("force_flg : 	%d\n", data->force_flg);
	printf("dst_ip :        %d.%d.%d.%d\n",data->dst_ip[0],data->dst_ip[1],
		                            data->dst_ip[2],data->dst_ip[3]);
	printf("src_ip :        %d.%d.%d.%d\n",data->src_ip[0],data->src_ip[1],
		                            data->src_ip[2],data->src_ip[3]);
	printf("dst_port:       %d\n", data->dst_port);
	printf("src_port:       %d\n", data->src_port);
}

void print_payload(void *buffer)
{
	pkt_payload *data = (pkt_payload*)buffer;
	printf("pkt Tyep:  payload\n");
	printf("hop_count : 	%d\n", data->hop_count);
	printf("length: 	    %d\n", data->len);
	printf("dst_ip :        %d.%d.%d.%d\n",data->dst_ip[0],data->dst_ip[1],
		                            data->dst_ip[2],data->dst_ip[3]);
	printf("src_ip :        %d.%d.%d.%d\n",data->src_ip[0],data->src_ip[1],
		                            data->src_ip[2],data->src_ip[3]);
	printf("dst_port:       %d\n", data->dst_port);
	printf("src_port:       %d\n", data->src_port);
	printf("payload :\n");
	int i = 0;
	for(;i < data->len; i++)
	{
		unsigned char *c = ((unsigned char*)data)+sizeof(pkt_payload)+i;
		printf("%2x ", *c);
		if((i-9)%10 == 0)
			printf("\n");
	}
}
 
void pkt_print(void* buffer)
{
	int pkt_type = *((unsigned char *)buffer);
	if(pkt_type == RREQ_TYPE)
	{
		print_rreq(buffer);
	}
	else if(pkt_type == RREP_TYPE)
	{
		print_rrep(buffer);
	}
	else if(pkt_type == PAYLOAD_TYPE) // payload
	{
		print_payload(buffer);
	}
	return;
}

route_index* find_dstip_in_hash(route_table* p_route_table, unsigned char dst_ip[])
{
	route_index *pnode;
	int key = hash_key(dst_ip);
	if(NULL == p_route_table)
	{
		return NULL;
	}
	if(NULL == (pnode = p_route_table->value[key]))
	{
		return NULL;
	}

	while(pnode)
	{
		if(!strncmp(dst_ip, pnode->data.dst_ip, 4))
		{
			return pnode;
		}
		pnode = pnode->next;
	}
	return NULL;
}





















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




int the_only_bind(){
	int sockfd;
	sockfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
	bzero(&odr_address, sizeof(odr_address));// ghi not really needed since its global
	odr_address.sun_family = AF_LOCAL;
	strcpy(odr_address.sun_path, ODR_SERVICE_PATH);
	Bind(sockfd, (SA *) &odr_address, sizeof(odr_address));
	return sockfd;
}

int keep_track_of_sun_pathz(char *sun_path){
	int i;
	i = (sizeof(infoz)/sizeof(sun_family));
	printf("(sizeof(infoz)/sizeof(sun_family)) is %d\n", i); // ghi

	// have we seen this sun_path before
	for(i=0;i<(sizeof(infoz)/sizeof(sun_path_info));i++){
		if(!strcmp(sun_path, infoz[i].sun_path)){

			// if stale
			if(is_this_ts_stale(infoz[i].ts)){
				bzero(infoz[i].sun_path, sizeof(infoz[i].sun_path));

				// if sun_path was blank this entire time things are messed up.
				// this should never happen ghi
				if(!memcmp(sun_path, infoz[i].sun_path, sizeof(sun_path))){
					printf("THE WORLD IS BURNING DOWN\n");
					printf("THE WORLD IS BURNING DOWN\n");
				}

				goto fill_in;
			}

			return i;
		}
	}

	fill_in:
	// make an entry
	// and do stuff with quote_port_quote
	// put in current time as ts
	quote_port_quote++;
	strncpy(infoz[quote_port_quote].sun_path, sun_path, sizeof(infoz[quote_port_quote].sun_path));
	infoz[quote_port_quote].ts=time(NULL);
	return quote_port_quote;
}

int is_this_ts_stale(time_t ts){
	time_t current_time = time(NULL);
	if(staleness_in_seconds > difftime(ts, current_time)){
		DEBUG("NOT STALE.\nts is=%d current_time=%d timediff=%d staleness_in_seconds=%d\n", ts, current_time, difftime(ts,current_time), staleness_in_seconds);
		return 0;
	}
	DEBUG("STALE.\nts is=%d current_time=%d timediff=%d staleness_in_seconds=%d\n", ts, current_time, difftime(ts,current_time), staleness_in_seconds);
	return 1;
}

int is_there_a_route(char *canonical_dst_ip){
	route_index *index = find_dstip_in_hash(g_route_table, data->dst_ip);
	DEBUG("in is_there_a_route");
	
	if(debug && index!=NULL)
		print_route_info(&(index->data));

	if(index != NULL)
		if(!is_this_ts_stale(index->data.record_time))
			return 1;
	return 0;
}

int send_raw_frame(int sock, void* pkt, int pkt_type, int if_index, unsigned char dst_mac[])t sock, void* pkt, int pkt_type, int if_index, unsigned char dst_mac[])
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
 


	int send_result = 0;
	int pkt_len = 0;

	unsigned char src_mac[ETH_ALEN];
	get_mac_by_if_index(if_index,src_mac);

	



	/*RAW communication*/
	socket_address.sll_family   = PF_PACKET;

	
	socket_address.sll_protocol = htons(ETH_P_IP);	
	socket_address.sll_hatype   = ARPHRD_ETHER;
	socket_address.sll_pkttype  = PACKET_OTHERHOST;

	/*index of the network device
	see full code later how to retrieve it*/
	socket_address.sll_ifindex  = if_index;

	//socket_address.sll_pkttype  = PACKET_OTHERHOST;
	/*address length*/
	socket_address.sll_halen    = ETH_ALEN;		
	/*MAC - begin*/
	socket_address.sll_addr[0]  = dst_mac[0];		
	socket_address.sll_addr[1]  = dst_mac[1];		
	socket_address.sll_addr[2]  = dst_mac[2];
	socket_address.sll_addr[3]  = dst_mac[3];
	socket_address.sll_addr[4]  = dst_mac[4];
	socket_address.sll_addr[5]  = dst_mac[5];
	/*MAC - end*/
	socket_address.sll_addr[6]  = 0x00;/*not used*/
	socket_address.sll_addr[7]  = 0x00;/*not used*/


	/*set the frame header*/
	memcpy((void*)buffer, (void*)dest_mac, ETH_ALEN);
	memcpy((void*)(buffer+ETH_ALEN), (void*)src_mac, ETH_ALEN);
	eh->h_proto = htons(ETH_ODR_TYPE);


	DEBUG("%s:%d: sizeof(*pkt) is %lu\n", __FUNCTION__, __LINE__, sizeof(*pkt));
	memcpy(data, pkt, sizeof(*pkt));
	pkt_len = 14 + sizeof(*pkt);


	
	Sendto(sock, buffer, pkt_len/*ETH_FRAME_LEN*/, 0, 
	      (struct sockaddr*)&socket_address, sizeof(socket_address));
	
	return 0;
}



int main(int argc, char **argv){
	int the_only_raw_socket;
	int domain_datagram_socket;
	int maxfd;	
	struct sockaddr_ll raw_address;  
	socklen_t odr_addr_len;
	socklen_t raw_addr_len;
	fd_set rset;
	ssize_t howmuchread;
	int index;

	if(argc!=2)
		err_quit("usage: odr_process <staleness in seconds>");
	staleness_in_seconds = atoi(argv[1]);

	get_hw_if();
	g_route_table = create_route_table();

	// IMPORTANT
	// IMPORTANT
	// IMPORTANT
	// the only raw socket should recv a PAYLOAD_TYPE packet and then send 
	// it to the server path that server.c should be connected to, if we're the
	// intended recipient
	the_only_raw_socket = Socket(AF_PACKET, SOCK_RAW, htons(PROTO_NUM));
	// IMPORTANT
	// IMPORTANT
	// IMPORTANT
	// IMPORTANT
	// the domain_datagram_socket is connected to ODR_SERVICE_PATH
	// ODR_SERVICE_PATH is the path msg_send in the api should write to
	domain_datagram_socket = the_only_bind();
	// IMPORTANT
	// IMPORTANT
	// IMPORTANT

	FD_ZERO(&rset);
	maxfd = max(domain_datagram_socket, the_only_raw_socket);

	while(1){
		FD_SET(domain_datagram_socket, &rset);
		FD_SET(the_only_raw_socket, &rset);

		Select(maxfd+1, &rset, NULL, NULL, NULL);

		if(FD_ISSET(domain_datagram_socket, &rset)){

			bzero(msg, sizeof(msg));
			odr_addr_len = sizeof(odr_address);
			how_much_read = Recvfrom(domain_datagram_socket, msg, sizeof(msg), 0, &odr_address, &odr_addr_len);
		    printf("Recieved from %s\n", odr_address.sun_path);

		    // keep track of sunpath's and stuff
		    index = keep_track_of_sun_pathz(odr_address.sun_path);
		    int force_flag;
		    int dst_port;
			char canonical_dst_ip[128];
			char recieved_msg[128];


			// because this is coming from the same host no endian stuff needs to be done
			// refer to the comment at "domain_datagram_socket = the_only_bind();"
		    sscanf(msg, "%s    %d    %s    %d",canonical_dst_ip, &dst_port,recieved_msg, &force_flag);

			printf("canonical_dst_ip is %s\n", canonical_dst_ip);
			printf("dst_port is %d\n", dst_port);
			printf("recieved_msg is %s\n", recieved_msg);
			printf("force_flag is %d  \n", force_flag);


			if(force_flag==1 || !is_there_a_route(canonical_dst_ip)){
				DEBUG("%s:%d: force_flag is %d and !is_there_a_route(canonical_dst_ip) is %d \n",__FUNCTION__, __LINE__, force_flag, !is_there_a_route(canonical_dst_ip));
				// NO ROUTE



				// flood all but interface it came in on

			// IMPORTANT
			// IMPORTANT
			// what to make the last dst_mac for send_raw_frame
			// also
			// how to avoid sending on lo and eth0
			// IMPORTANT
			// IMPORTANT

				// pkt_rreq frame = calloc(sizeof(pkt_rreq));
				// // frame.pkt_type = 0;
				// frame.rrep_set
				// frame.hop_count
				// frame.force_flag
				// memcpy(frame.dst_ip
				// memcpy(frame.src_ip
				// frame.dst_port
				// frame.src_port
				// frame.broadcast_id


				// struct hw_info	*hw = hw_list;
				// while(hw != NULL)
				// {
				// 	if(hw->if_index !=  if_index)
				// 	{
				// 		memcpy(srcmac, hw->if_haddr,ETH_ALEN);
				// 		send_raw_frame(the_only_raw_socket, &frame, hw->if_index, ???????);
				// 		// send_raw_frame(int sock, void* pkt, int if_index, unsigned char dst_mac[])
				// 	}
				// 	hw = hw->hw_next;
				// }

				// send rreq
				// using the_only_raw_socket
				// broadcast_id++
				// 
				// 
			}

			// WE HAVE A ROUTE

			// send msg using the_only_raw_socket





		}else if(FD_ISSET(the_only_raw_socket, &rset)){
			bzero(msg, sizeof(msg));
			raw_addr_len = sizeof(raw_address);			
			how_much_read = Recvfrom(the_only_raw_socket, msg, sizeof(msg), 0, &raw_address, &raw_addr_len);
			// read http://unixhelp.ed.ac.uk/CGI/man-cgi?packet+7
			// int	    sll_ifindex;  /* Interface number */
			// that's the 
			// You will need to try out PF_PACKET sockets for yourselves and familiarize yourselves with how they behave. If, when you read from the socket and provide a sockaddr_ll structure, the kernel returns to you the index of the interface on which the incoming frame was received, then one socket will be enough.
			
			printf("we're receiving on ifindex %d from %s\n", raw_address.sll_ifindex, raw_address.sll_addr);

			// analyze msg and do stuff
			process_msg(msg);


		}

	}

	exit(0);
}

// 
void process_msg(char *msg){
	int pkt_type = *((unsigned char *)msg);
	switch(pkt_type){
		case RREQ_TYPE:
			DEBUG("we've just received an RREQ_TYPE");
			turn_rreq_ho(msg);
			// do stuff				
			break;
		case RREP_TYPE:
			DEBUG("we've just received an RREP_TYPE");
			turn_rrep_ho(msg);
			// do stuff	
			break;
		case PAYLOAD_TYPE:
			DEBUG("we've just received an PAYLOAD_TYPE");
			turn_payload_ho(msg);
			// do stuff	
			break;
	}
}




// IMPORTANT
// IMPORTANT
// IMPORTANT
// I'm assuming chars and char[] are not affected by endianess
// I think I'm right, not 100% sure though
// IMPORTANT
// IMPORTANT
// IMPORTANT

//turn to host order
void turn_rreq_ho(pkt_rreq *msg){
	msg.dst_port = ntohs(msg.dst_port);
	msg.src_port = ntohs(msg.src_port);
	msg.broadcast_id = ntohl(msg.broadcast_id);
}

//turn to host order
void turn_rrep_ho(pkt_rreq *msg){
	msg.dst_port = ntohs(msg.dst_port);
	msg.src_port = ntohs(msg.src_port);
}

//turn to host order
void turn_payload_ho(pkt_rreq *msg){
	msg.len = ntohs(msg.len);
	msg.dst_port = ntohs(msg.dst_port);
	msg.src_port = ntohs(msg.src_port);
}


//turn to network order
void turn_rreq_no(pkt_rreq *msg){
	msg.dst_port = htons(msg.dst_port);
	msg.src_port = htons(msg.src_port);
	msg.broadcast_id = htonl(msg.broadcast_id);
}

//turn to network order
void turn_rrep_no(pkt_rreq *msg){
	msg.dst_port = htons(msg.dst_port);
	msg.src_port = htons(msg.src_port);
}

//turn to network order
void turn_payload_no(pkt_rreq *msg){
	msg.len = htons(msg.len);
	msg.dst_port = htons(msg.dst_port);
	msg.src_port = htons(msg.src_port);
}