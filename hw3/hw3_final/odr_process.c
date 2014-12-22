// nothing here
// take 1 argument
// ...
#include "unp.h"
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <stdlib.h>
#include "hw_addrs.h"
#include "odr_process.h"
#include "asgn3.h"

#define ETH_FRAME_LEN 100
struct hw_info* hw_list = NULL;
route_table* g_route_table = NULL;
unsigned char local_ip[4];
static int b_id = 1;//broadcast_id 

int if_sock_array[MAX_IF_NUM];
int g_timeout;
int domain_datagram_socket;
port_file_head g_port_file;

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

void test_hash_table(void)
{
	//route_table *p= create_route_table();
	route_info data;
	route_index *index;
	int i = 0;

	unsigned char dstip[4] ={192,168,10,0};
	unsigned char mac[ETH_ALEN] = {00,01,02,03,04,05};
	unsigned char hop_count = 3;
	unsigned char hop_if =3;
	unsigned char b_id =10;
	time_t   time1 = time(NULL);

	for(i = 0; i < 20; i++)
	{
		dstip[3] = dstip[3]+1;
		memcpy(data.dst_ip, dstip,4);
		memcpy(data.next_hop_addr, mac, ETH_ALEN);
		data.hop_count = hop_count;
		data.next_hop_if = hop_if;
		data.broadcast_id = b_id;
		data.record_time = time1;

		if(insert_data_into_hash(g_route_table, &data)<0)
		{
			printf("Hash table insert fail\n");
		}
	}
	printf("after add 20 route index\n");
	print_route_table();

	dstip[3]+=1;
	for(;i > 10; i--)
	{
		dstip[3] = dstip[3]-1;
		index= find_dstip_in_hash(g_route_table, dstip);
		if(index == NULL)
		{
			printf("find index fail\n");
		}

		if(delete_data_from_hash(g_route_table, &(index->data))<0)
			printf("delete fail\n");
	}
	printf("after del 10 route index\n");
	print_route_table();

}

void free_hw_if( struct hw_info* hw_if)
{
	if(hw_if->hw_next != NULL)
	{
		free_hw_if(hw_if->hw_next);
	}
	else
	{
		free(hw_if);
	}
}

void get_hw_if(void)
{
	struct hwa_info	*hwa,*hwahead;
	struct hw_info *hwb, *hw_node;
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
			// save this ip address 
			if ((sa = hwa->ip_addr) != NULL)
			{	
				sscanf(Sock_ntop_host(sa, sizeof(*sa)), "%d.%d.%d.%d",&local_ip[0],&local_ip[1],
				&local_ip[2],&local_ip[3]);
				//printf("local_ip :%d.%d.%d.%d \n",local_ip[0],local_ip[1],
				//local_ip[2],local_ip[3]);
			}
			//memcpy(local_mac, hwa->if_haddr,ETH_ALEN);
			//printf("local_mac: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n",local_mac[0],local_mac[1],local_mac[2],
				//local_mac[3],local_mac[4],local_mac[5]);
			continue;
		}
		else
		{
			if(hw_list == NULL)
			{
				hw_list = (struct hw_info*)malloc(sizeof(struct hw_info));
				memcpy(hw_list->if_name, hwa->if_name, IF_NAME);
				memcpy(hw_list->if_haddr, hwa->if_haddr,IF_HADDR);
				hw_list->if_index = hwa->if_index;
				memcpy(&hw_list->ip_addr, hwa->ip_addr, sizeof(struct sockaddr));
				hw_list->hw_next = NULL;
			}
			else
			{
				hw_node = (struct hw_info*)malloc(sizeof(struct hw_info));
				memcpy(hw_node->if_name, hwa->if_name, IF_NAME);
				memcpy(hw_node->if_haddr, hwa->if_haddr,IF_HADDR);
				hw_node->if_index = hwa->if_index;
				memcpy(&hw_node->ip_addr, hwa->ip_addr, sizeof(struct sockaddr));
				hw_node->hw_next = hw_list->hw_next;
				hw_list->hw_next = hw_node;
			}
		}
	}
	free_hwa_info(hwahead);
	for(hwb = hw_list; hwb!= NULL; hwb = hwb->hw_next)
	{
		//printf("%s :%s", hwb->if_name, "\n");
		
		if ( (sa = &(hwb->ip_addr)) != NULL)
			//printf("         IP addr = %s\n", Sock_ntop_host(sa, sizeof(*sa)));
				
		prflag = 0;
		i = 0;
		do {
			if (hwb->if_haddr[i] != '\0') {
				prflag = 1;
				break;
			}
		} while (++i < IF_HADDR);

		if (prflag) {
			//printf("         HW addr = ");
			ptr = hwb->if_haddr;
			i = IF_HADDR;
			do {
				//printf("%.2x%s", *ptr++ & 0xff, (i == 1) ? " " : ":");
			} while (--i > 0);
		}

		//printf("\n         interface index = %d\n\n", hwb->if_index);
	}
	//free_hw_if(hw_list);
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

void get_ip_by_if_index(int if_index, unsigned char src_ip[])
{
#if 0
	struct hw_info	*hw = hw_list;
	struct sockaddr	*sa;
	while(hw != NULL)
	{
		if(hw->if_index ==  if_index)
		{
			sa = &(hw->ip_addr);
			if(sa != NULL)
			{
				sscanf(Sock_ntop_host(sa, sizeof(*sa)), "%d.%d.%d.%d",&src_ip[0],&src_ip[1],
				&src_ip[2],&src_ip[3]);
				//printf("if_index %d, src_ip: %d.%d.%d.%d\n",if_index,src_ip[0],src_ip[1],
					//src_ip[2],src_ip[3]);
			}
			return;
		}
		hw = hw->hw_next;
	}
#endif
	memcpy(src_ip, local_ip, 4);
	return;
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

int find_name_by_port(int port, char*s)
{
	int i;
	for(i = 0; i < g_port_file.next_pos; i++)
	{
		if(port == g_port_file.pf[i].port)
		{
			strcpy(s, g_port_file.pf[i].name);
			return 0;
		}
	}
	return -1;
}

void find_temp_port(const char* s, int* port)
{
	int i;
	for(i = 0; i< g_port_file.next_pos; i++)
	{
		if(!memcmp(g_port_file.pf[i].name, s, strlen(s)))
		{
			*port = g_port_file.pf[i].port;
			return;
		}
	}


	//need to assign a new port
	if(g_port_file.next_pos == PORT_FILE_NUM)
	{
		return;
	}
	else
	{
		int index = g_port_file.next_pos;
		g_port_file.pf[index].in_use = 1;
		*port = g_port_file.pf[index].port;
		g_port_file.pf[index].tt = time(NULL);
		memcpy(g_port_file.pf[index].name, s, strlen(s));
		g_port_file.pf[index].name[strlen(s)] ='\0';
		//printf("temp file name is %s\n", g_port_file.pf[index].name);
		g_port_file.next_pos++;
	}
}


void init_port_file(void)
{
	int i;
	// pos 0 for server well known port
	g_port_file.pf[0].port = SERVER_PORT;
	g_port_file.pf[0].tt = -1;
	g_port_file.pf[0].in_use = 1;
	memcpy(g_port_file.pf[0].name, WELLKNOWN_PATH, sizeof(WELLKNOWN_PATH));
	//printf("server path name %s\n", g_port_file.pf[0].name);
	g_port_file.next_pos = 1;
	for(i = 1; i < PORT_FILE_NUM; i++)
	{
		g_port_file.pf[i].port = 22000+i;
		g_port_file.pf[i].tt = 0;
		g_port_file.pf[i].in_use = 0;
		memset(g_port_file.pf[i].name, 0, 64);
	}

	return;
}
void write_to_sc(int src_port, int dst_port, unsigned char src_ip[], unsigned char*data, int len)
{
	char name[64], ip[64];
	datamsg msg;

	struct sockaddr_un address;
	
	find_name_by_port(dst_port, name);
	//printf("write_to_sc: get name %s for port %d\n", name, dst_port);

	bzero(&address, sizeof(address));
	address.sun_family = AF_LOCAL;
	// if(from_cli){
	strcpy(address.sun_path, name);

	bzero(&msg, sizeof(msg));	
	snprintf(ip, 64, "%d.%d.%d.%d", src_ip[0],src_ip[1],src_ip[2],src_ip[3]);
	//snprintf(msg, MAXLINE, "%s    %d    %s    %d", ip, src_port, data, 0/*no use*/);
	//printf("write_to_sc: msg %s\n", msg);
	strcpy(msg.data, data);
	strcpy(msg.canonical_ip, ip);
	msg.port = src_port;
	msg.force_flg = 0;
	Sendto(domain_datagram_socket, &msg, sizeof(msg), 0, &address, sizeof(address)); 

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
void fill_rreq_pkt(void *data, int if_index, unsigned char dstip[], unsigned short src_port,
	   unsigned short dst_port, unsigned int force_flg)
{
	pkt_rreq *pkt = (pkt_rreq*)data;

	pkt->broadcast_id = b_id;
	pkt->pkt_type = RREQ_TYPE;
	pkt->hop_count = 1;
	pkt->force_flg = force_flg;

	pkt->rrep_set = 0;
	memcpy(pkt->dst_ip, dstip, 4);
	get_ip_by_if_index(if_index, pkt->src_ip);
	pkt->src_port = src_port;
	pkt->dst_port = dst_port;

	return;
}
void fill_rrep_pkt(void *data, int if_index, unsigned char dstip[],unsigned short src_port,
		unsigned short dst_port, unsigned int force_flg)
{
	pkt_rrep *pkt = (pkt_rrep*)data;

	pkt->pkt_type = RREP_TYPE;
	pkt->hop_count = 1;
	pkt->force_flg = force_flg;

	memcpy(pkt->dst_ip, dstip, 4);
	get_ip_by_if_index(if_index, pkt->src_ip);
	pkt->src_port = src_port;
	pkt->dst_port = dst_port;

	return;
}

void fill_payload_pkt(void *data, void*load, unsigned char dst_ip[],unsigned short src_port,
		unsigned short dst_port, unsigned short len, int force_flg)
{
	pkt_payload *pkt = (pkt_payload*)data;

	pkt->pkt_type = PAYLOAD_TYPE;
	pkt->hop_count = 1;
	pkt->len = len;
	memcpy(pkt->src_ip, local_ip,4);
	memcpy(pkt->dst_ip, dst_ip, 4);
	pkt->src_port = src_port;
	pkt->dst_port = dst_port;
	pkt->force_flg = force_flg;
	memcpy(data+sizeof(pkt_payload),load,len);
	return;
}


void relay_rreq_pkt(void *data, int if_index)
{
	pkt_rreq *pkt = (pkt_rreq*)data;
	pkt->hop_count++;
	return;
}




void relay_rrep_pkt(void* data)
{
	pkt_rrep *pkt = (pkt_rrep*)data;
	pkt->hop_count++;
	return;
}

void relay_payload_pkt(void* data)
{
	pkt_payload *pkt = (pkt_payload*)data;
	pkt->hop_count++;
	return;
}




int is_src_ip_local(unsigned char src_ip[])
{
	struct hw_info	*hw = hw_list;
	struct sockaddr	*sa;
	unsigned char ip[4];
	while(hw != NULL)
	{
		sa = &(hw->ip_addr);
		if(sa != NULL)
		{
			sscanf(Sock_ntop_host(sa, sizeof(*sa)), "%d.%d.%d.%d",&ip[0],&ip[1],
				&ip[2],&ip[3]);
			if(!memcmp(src_ip, ip, 4))
			{
				return 1;
			}
		}
		hw = hw->hw_next;
	}
	return 0;
}

int comp_route_info(route_info *old, route_info *new, int type)
{
	if(type == RREQ_TYPE)
	{
		if(old->broadcast_id < new->broadcast_id)
		{
			return 1;
		}
		else if(old->broadcast_id > new->broadcast_id)
		{
			return 0;
		}
		else // the same broadcast_id 
		{
			if(old->hop_count >= new->hop_count)
			{
				return 1;
			}
			else 
				return 0;
		}
	}
	else if((type == RREP_TYPE) ||(type == PAYLOAD_TYPE))
	{
		if(old->hop_count > new->hop_count)
		{
			return 1;
		}
		else 
			return 0;
	}
}

void rreq_op(int sock, void* buffer, unsigned char src_mac[], int recv_if_index)
{
	pkt_rreq* data = (pkt_rreq*)buffer;
	route_info   new_info;
	route_index *r_index;
	int force_flg = 0;
	int if_index_error = 0;
	unsigned char dst_ip[4],  src_ip[4], if_ip[4];

	if((recv_if_index !=3) && (recv_if_index !=4))
	{
		if_index_error = 0;
		//printf("recv rreq from error if_index\n");
		return;
	}

	memcpy(src_ip, data->src_ip, 4);

	get_ip_by_if_index(recv_if_index, dst_ip);

	//check if the rreq is sent by this node
	if(is_src_ip_local(src_ip))
	{
		//do nothing, discard the pkt
		//printf("receive duplicated rreq pkt\n");
		return;
	}
	//pkt_print(buffer);

	get_ip_by_if_index(recv_if_index, if_ip);
	//printf("recv rreq from if_index %d with ip %d.%d.%d.%d\n", recv_if_index,if_ip[0],if_ip[1],
	//	if_ip[2],if_ip[3]);
	//printf("                 dst_ip: %d.%d.%d.%d\n" ,data->dst_ip[0],data->dst_ip[1],
	//	data->dst_ip[2],data->dst_ip[3]);
	//printf("                 src_ip: %d.%d.%d.%d\n",data->src_ip[0],data->src_ip[1],
	//	data->src_ip[2],data->src_ip[3]);	//build a new route index
	memcpy(new_info.dst_ip, src_ip,4);
	memcpy(new_info.next_hop_addr, src_mac, ETH_ALEN);
	new_info.next_hop_if = recv_if_index;
	new_info.hop_count = data->hop_count;
	new_info.broadcast_id = data->broadcast_id;
	new_info.record_time = time(NULL);
	new_info.force_flg = data->force_flg;

	force_flg = data->force_flg;
	// look up route table 
	if((r_index=find_dstip_in_hash(g_route_table, src_ip))==NULL)
	{
		// no route index avaiable
		// insert new index into route table
		if(insert_data_into_hash(g_route_table, &new_info)<0)
		{
			printf("%s:%d: insert route table fail\n",__FUNCTION__, __LINE__);
		}
		//printf("rreq: insert new route index\n");
		//print_route_table();
	}
	else
	{
		//check if this route_index is time out
		time_t new = time(NULL);
		time_t old = r_index->data.record_time;
		if(g_timeout< difftime(new, old))
		{
			//delete old one and insert a new one 
			if(delete_data_from_hash(g_route_table, &(r_index->data))<0)
			{
				printf("%s:%d: delete route table fail\n", __FUNCTION__, __LINE__);
			}

			if(insert_data_into_hash(g_route_table, &new_info)<0)
			{
				printf("%s:%d: insert route table fail\n",__FUNCTION__, __LINE__);
			}
			//printf("rreq: update route table due to time out\n");
			//print_route_table();

		}
		else
		{
			//compare this two route index and choose a better one
			if(comp_route_info(&(r_index->data), &new_info,RREQ_TYPE))
			{
				// delete old one, and insert new one
				if(delete_data_from_hash(g_route_table, &(r_index->data))<0)
				{
					printf("%s:%d: delete route table fail\n", __FUNCTION__, __LINE__);
				}

				if(insert_data_into_hash(g_route_table, &new_info)<0)
				{
					printf("%s:%d: insert route table fail\n",__FUNCTION__, __LINE__);
				}
				//printf("rreq: udate route table due to better result\n");
				//print_route_table();
			}
			else
			{
				if(force_flg)
				{
					if(r_index->data.force_flg)
					{
						return;// to avoid rreq looping in circle
					}
					else
					{
						// delete old one, and insert new one
						if(delete_data_from_hash(g_route_table, &(r_index->data))<0)
						{
							printf("%s:%d: delete route table fail\n", __FUNCTION__, __LINE__);
						}

						if(insert_data_into_hash(g_route_table, &new_info)<0)
						{
							printf("%s:%d: insert route table fail\n",__FUNCTION__, __LINE__);
						}
						//printf("rreq: udate route table due to force flag result\n");
						//print_route_table();
					}
				}
				else
					return;
			}
		}
	}

	//check if the dst_ip is ip of the recv_if_index
	if(!memcmp(dst_ip, data->dst_ip, 4))
	{
		if(data->rrep_set)
		{
			//do nothing
			//printf("rreq: receive a rreq pkt with rrep_set and don't reply\n");
			return;
		}
		else
		{
			// send back a rrep
			//printf("rreq: sedn back a rrep throuth if_index: %d\n", recv_if_index);
			pkt_rrep  pkt_rrep;
			fill_rrep_pkt(&pkt_rrep, recv_if_index, data->src_ip,
						data->src_port, data->dst_port, force_flg);
			pf_send1(sock, &pkt_rrep, recv_if_index, RREP_TYPE, data->src_ip);
			//pf_send1(sock, &pkt_rrep, recv_if_index, RREP_TYPE, data->src_ip);
			return;
		}
	}
	else
	{
		pkt_rreq rq;
		pkt_rrep rp;
		struct hw_info *node = hw_list;
		if(!data->rrep_set)
		{
			//check if the route index is time out
			route_index *index = find_dstip_in_hash(g_route_table, data->dst_ip);
			if(force_flg)
			{
			// intermediate node do not reply rrep
			// do nothing herre
			//	printf("intermediate node find force_flg set and stay calm\n");
			}
			else if(index != NULL)
			{
			// if force_flg is not set, then check if intermediate node can
			// send a rrep 
				time_t new = time(NULL);
				time_t old = index->data.record_time;
				if(g_timeout> difftime(new, old))
				{
					//printf("rreq: send a rrep back and propagate rreq packet\n");
					rp.pkt_type  = RREP_TYPE;
					rp.hop_count = index->data.hop_count+1; 
					rp.force_flg = 0;

					memcpy(rp.dst_ip, data->src_ip, 4);
					memcpy(rp.src_ip, data->dst_ip, 4);
					rp.src_port = data->src_port;
					rp.dst_port = data->dst_port;
					pf_send1(sock, &rp, recv_if_index, RREP_TYPE, data->src_ip);
					//pf_send1(sock, &rp, recv_if_index, RREP_TYPE, data->src_ip);

					//send rreq with rrep_set
					memcpy(&rq, data, sizeof(pkt_rreq));
					rq.rrep_set = 1;
					for(node = hw_list; node != NULL; node = node->hw_next)
					{
						if(node->if_index!= recv_if_index)
						{
							relay_rreq_pkt(&rq, node->if_index);
							pf_send1(sock, &rq, node->if_index, RREQ_TYPE, rq.dst_ip);
						}
					}
				}
				else
				{
				// normal relay rreq without rrep_set
					memcpy(&rq, data, sizeof(pkt_rreq));
					for(node = hw_list; node != NULL; node = node->hw_next)
					{
						if(node->if_index!= recv_if_index)
						{
							//printf("rreq: send a rreq relay through if_index: %d\n", node->if_index);
							relay_rreq_pkt(&rq, node->if_index);
							pf_send1(sock, &rq, node->if_index, RREQ_TYPE, rq.dst_ip);
						}
					}
				}
			
				return;
			}
		}
		else
		{
			//printf("rreq: should send a rrep back but stop by rrep_set\n");
		}
		//relay rreq
		memcpy(&rq, data, sizeof(pkt_rreq));
		for(node = hw_list; node != NULL; node = node->hw_next)
		{
			if(node->if_index!= recv_if_index)
			{
				//printf("rreq: send a rreq relay through if_index: %d\n", node->if_index);
				relay_rreq_pkt(&rq, node->if_index);
				pf_send1(sock, &rq, node->if_index, RREQ_TYPE, rq.dst_ip);
			}
		}
		return;
	}
	return;
}

void rrep_op(int sock, void* buffer, unsigned char src_mac[], int recv_if_index)
{
	pkt_rrep* data = (pkt_rrep*)buffer;
	route_info   new_info;
	route_index *r_index;
	int if_index_error = 0;
	int force_flg = 0;
	unsigned char dst_ip[4],  src_ip[4],if_ip[4];

	if((recv_if_index !=3) && (recv_if_index !=4))
	{
		if_index_error = 0;
		//printf("recv rrep from error if_index\n");
		return;
	}

	memcpy(src_ip, data->src_ip, 4);

	get_ip_by_if_index(recv_if_index, dst_ip);

	//check if the rrep is sent by this node
	if(is_src_ip_local(src_ip))
	{
		//do nothing, discard the pkt
		//printf("receive duplicated rrep pkt\n");
		return;
	}
	//pkt_print(buffer);
	get_ip_by_if_index(recv_if_index, if_ip);
	//printf("recv rrep from if_index %d with ip %d.%d.%d.%d\n", recv_if_index,if_ip[0],if_ip[1],
	//	if_ip[2],if_ip[3]);
	//printf("                 dst_ip: %d.%d.%d.%d\n" ,data->dst_ip[0],data->dst_ip[1],
	//	data->dst_ip[2],data->dst_ip[3]);
	//printf("                 src_ip: %d.%d.%d.%d\n",data->src_ip[0],data->src_ip[1],
	//	data->src_ip[2],data->src_ip[3]);
	//build a new route index
	memcpy(new_info.dst_ip, src_ip,4);
	memcpy(new_info.next_hop_addr, src_mac, ETH_ALEN);
	new_info.next_hop_if = recv_if_index;
	new_info.hop_count = data->hop_count;
	new_info.broadcast_id = 0;// rrep has no broadcast_id
	new_info.record_time = time(NULL);

	force_flg = data->force_flg;
	// look up route table 
	if((r_index=find_dstip_in_hash(g_route_table, src_ip))==NULL)
	{
		// no route index avaiable
		// insert new index into route table
		if(insert_data_into_hash(g_route_table, &new_info)<0)
		{
			printf("%s:%d: insert route table fail\n",__FUNCTION__, __LINE__);
		}
		//printf("rrep: insert new route index\n");
		//print_route_table();
	}
	else
	{
		//check if this route_index is time out
		time_t new = time(NULL);
		time_t old = r_index->data.record_time;
		if(g_timeout< difftime(new, old))
		{
			//delete old one and insert a new one 
			if(delete_data_from_hash(g_route_table, &(r_index->data))<0)
			{
				printf("%s:%d: delete route table fail\n", __FUNCTION__, __LINE__);
			}

			if(insert_data_into_hash(g_route_table, &new_info)<0)
			{
				printf("%s:%d: insert route table fail\n",__FUNCTION__, __LINE__);
			}
			//printf("rrep: update route table due to time out\n");
			//print_route_table();
		}
		else
		{
			//compare this two route index and choose a better one
			if(force_flg || comp_route_info(&(r_index->data), &new_info,RREP_TYPE))
			{
				// delete old one, and insert new one
				if(delete_data_from_hash(g_route_table, &(r_index->data))<0)
				{
					printf("%s:%d: delete route table fail\n", __FUNCTION__, __LINE__);
				}

				if(insert_data_into_hash(g_route_table, &new_info)<0)
				{
					printf("%s:%d: insert route table fail\n",__FUNCTION__, __LINE__);
				}
				//printf("rrep: update route table due to better hop\n");
				//print_route_table();
			}

		}
	}

	//check if the dst_ip is ip of the recv_if_index
	if(!memcmp(dst_ip, data->dst_ip, 4))
	{
		//printf("received the rrep for my rreq \n");
		return;
	}
	else
	{

		pkt_rrep rp;

		memcpy(&rp, data, sizeof(pkt_rrep));
		relay_rrep_pkt(&rp);
		pf_send1(sock,&rp, 0/*no use*/, RREP_TYPE, rp.dst_ip);

		return;
	}
	return;
}

void payload_op(int sock, void* buffer, unsigned char src_mac[], int recv_if_index)
{
	pkt_payload* data = (pkt_payload*)buffer;
	route_info   new_info;
	route_index *r_index;
	int if_index_error = 0;

	unsigned char dst_ip[4],  src_ip[4],if_ip[4];

	if((recv_if_index !=3) && (recv_if_index !=4))
	{
		if_index_error = 0;
		//printf("recv payload from error if_index\n");
		return;
	}

	memcpy(src_ip, data->src_ip, 4);

	get_ip_by_if_index(recv_if_index, dst_ip);

	//check if the rrep is sent by this node
	if(is_src_ip_local(src_ip))
	{
		//do nothing, discard the pkt
		//printf("receive duplicated payload pkt\n");
		return;
	}
	//pkt_print(buffer);
	get_ip_by_if_index(recv_if_index, if_ip);
	//printf("recv payload from if_index %d with ip %d.%d.%d.%d\n", recv_if_index,if_ip[0],if_ip[1],
	//	if_ip[2],if_ip[3]);
	//printf("                 dst_ip: %d.%d.%d.%d\n" ,data->dst_ip[0],data->dst_ip[1],
	//	data->dst_ip[2],data->dst_ip[3]);
	//printf("                 src_ip: %d.%d.%d.%d\n",data->src_ip[0],data->src_ip[1],
	//	data->src_ip[2],data->src_ip[3]);
	//build a new route index
	memcpy(new_info.dst_ip, src_ip,4);
	memcpy(new_info.next_hop_addr, src_mac, ETH_ALEN);
	new_info.next_hop_if = recv_if_index;
	new_info.hop_count = data->hop_count;
	new_info.broadcast_id = 0;// payload has no broadcast_id
	new_info.record_time = time(NULL);

	// look up route table 
	if((r_index=find_dstip_in_hash(g_route_table, src_ip))==NULL)
	{
		// no route index avaiable
		// insert new index into route table
		if(insert_data_into_hash(g_route_table, &new_info)<0)
		{
			printf("%s:%d: insert route table fail\n",__FUNCTION__, __LINE__);
		}
		//printf("payload: insert new route index\n");
		//print_route_table();
	}
	else
	{
		//check if this route_index is time out
		time_t new = time(NULL);
		time_t old = r_index->data.record_time;
		if(g_timeout< difftime(new, old))
		{
			//delete old one and insert a new one 
			if(delete_data_from_hash(g_route_table, &(r_index->data))<0)
			{
				printf("%s:%d: delete route table fail\n", __FUNCTION__, __LINE__);
			}

			if(insert_data_into_hash(g_route_table, &new_info)<0)
			{
				printf("%s:%d: insert route table fail\n",__FUNCTION__, __LINE__);
			}
			//printf("payload: update route table due to time out\n");
			//print_route_table();
		}
		else
		{
			//compare this two route index and choose a better one
			if(comp_route_info(&(r_index->data), &new_info,RREP_TYPE))
			{
				// delete old one, and insert new one
				if(delete_data_from_hash(g_route_table, &(r_index->data))<0)
				{
					printf("%s:%d: delete route table fail\n", __FUNCTION__, __LINE__);
				}

				if(insert_data_into_hash(g_route_table, &new_info)<0)
				{
					printf("%s:%d: insert route table fail\n",__FUNCTION__, __LINE__);
				}
				//printf("payload: update route table due to better hop\n");
				//print_route_table();
			}

		}
	}

	//check if the dst_ip is ip of the recv_if_index
	if(!memcmp(dst_ip, data->dst_ip, 4))
	{
		//printf("received the payload for myself \n");
		unsigned char b[100]={};
		memcpy(b,buffer+sizeof(pkt_payload),data->len);
		//pkt_print(data);
		b[data->len]='\0';
		//printf("receved data in payload size %d\n", (data->len));
		int i = 0;
		//for(; i < data->len; i++)
		//{
		//	printf("%c", b[i]);
		//}
		//printf("\n");
		fflush(stdout);
		//handle the received msg

		write_to_sc(data->src_port, data->dst_port, data->src_ip, b, data->len+1);
		return;
	}
	else
	{
		// need to relay payload
		unsigned char buffer[ETH_FRAME_LEN];
		memcpy(buffer, data, sizeof(pkt_payload)+data->len);
		relay_payload_pkt(buffer);
		pf_send1(sock,buffer, 0/*no use*/, PAYLOAD_TYPE, data->dst_ip);

		return;
	}
	return;
}
void pkt_parse(int sock, void* buffer, int len, int recv_if_index)
{
	route_info  route_1;
	unsigned char *data = (unsigned char*)buffer+14;

	int pkt_type = *data;
	unsigned char src_mac[ETH_ALEN];
	memcpy(src_mac, buffer+6, ETH_ALEN);
	if(pkt_type == RREQ_TYPE)
	{
		//pkt_print(data);
		rreq_op(sock, data, src_mac, recv_if_index);
	}
	else if(pkt_type == RREP_TYPE)
	{
		//pkt_print(data);
		rrep_op(sock, data, src_mac, recv_if_index);
	}
	else if(pkt_type == PAYLOAD_TYPE)
	{
		//pkt_print(data);
		payload_op(sock, data, src_mac, recv_if_index);
	}

}

int pf_recv()
{
	int i,j,s;
	int maxfd = -1;
	int recv_if_index = 0;
	int length = 0; /*length of the received frame*/ 
	struct sockaddr_ll socket_address;
	socklen_t len;
	unsigned char buffer[ETH_FRAME_LEN]; /*Buffer for ethernet frame*/
	fd_set rset;


	FD_ZERO(&rset);

	for(i = 0; i < MAX_IF_NUM; i++)
	{
		if(if_sock_array[i]!=0)
		{
			FD_SET(if_sock_array[i], &rset);
			if(maxfd < if_sock_array[i])
				maxfd = if_sock_array[i];
			//printf("pf_recv: set sock %d for if %d\n", if_sock_array[i],i);
		}
	}

	Select(maxfd+1, &rset, NULL, NULL, NULL);
	for(i = 0; i < MAX_IF_NUM; i++)
	{
		if((s=if_sock_array[i])!=0)
		{
			if(FD_ISSET(s,&rset))
			{
			    length = recvfrom(s, buffer, ETH_FRAME_LEN, 0, NULL, NULL);
				if (length < 0) 
				{ 
					perror("recv pf pkt error"); 
				}
				//for(j = 0;j < length; j++)
				{
					//printf("%.2x ", *(buffer+j));
					//if((j-9) % 10 == 0) printf("\n");
					//fflush(stdout);
				}
				recv_if_index = i;
				//printf("receive pkt from if_index %d\n", i);
				pkt_parse(s,buffer, length, recv_if_index);
				return recv_if_index;
			}
		}
	}
	return 0;
}
void bind_all_if(void)
{
	struct hw_info *hw = NULL;
	struct sockaddr_ll socket_addr;
	int i = 0;
	int j = 0;

	int s1 = socket(AF_PACKET, SOCK_RAW, htons(ETH_ODR_TYPE));
    socket_addr.sll_ifindex = 3;
	socket_addr.sll_family = PF_PACKET;
	bind(s1,(struct sockaddr*)&socket_addr,sizeof(socket_addr));
	if_sock_array[3]=s1;

	if(hw_list->hw_next != NULL)
	{
		int s = socket(AF_PACKET, SOCK_RAW, htons(ETH_ODR_TYPE));
		socket_addr.sll_ifindex = 4;
		socket_addr.sll_family = PF_PACKET;
		bind(s,(struct sockaddr*)&socket_addr,sizeof(socket_addr));
		if_sock_array[4]=s;
	}



	//following code can not receive pkt after bind. I don't know why.
	#if 0
	//for(hw = hw_list; hw != NULL; hw= hw->hw_next)
	{
		int sock = 0;
		sock = Socket(AF_PACKET, SOCK_RAW, htons(ETH_ODR_TYPE));
		socket_addr.sll_ifindex = hw->if_index;
		socket_addr.sll_family = PF_PACKET;
		
		Bind(sock,(struct sockaddr*)&socket_addr,sizeof(socket_addr));
		//if_sock_array[hw->if_index] = sock;

		//printf("if_index %d bind to to sock %d\n", hw->if_index, sock);

	}
	#endif
	return;
}


int wait_for_rrep(int sock, unsigned char dst_ip[])
{
	int recv_if;
    int len;
    unsigned char recv_buff[ETH_FRAME_LEN]={};
    int i,j,s;
	int maxfd = -1;
	int recv_if_index = 0;
	int length = 0; /*length of the received frame*/ 
	struct sockaddr_ll socket_address;
	unsigned char buffer[ETH_FRAME_LEN]; /*Buffer for ethernet frame*/
	fd_set rset;
	struct timeval time_out;
	int result;

	FD_ZERO(&rset);
	time_out.tv_sec =2;
	time_out.tv_usec=0;

while(1)
{
	for(i = 0; i < MAX_IF_NUM; i++)
	{
		if(if_sock_array[i]!=0)
		{
			FD_SET(if_sock_array[i], &rset);
			if(maxfd < if_sock_array[i])
				maxfd = if_sock_array[i];
			//printf("wait_for_rrep: set sock %d for if %d\n", if_sock_array[i],i);
		}
	}
	result = select(maxfd+1, &rset, NULL, NULL, &time_out);
	if(result == 0)
	{
		//printf("wait for rrep time out\n");
		return -1;
	}
	for(i = 0; i < MAX_IF_NUM; i++)
	{
		if((s=if_sock_array[i])!=0)
		{
			if(FD_ISSET(s,&rset))
			{
			    length = recvfrom(s, buffer, ETH_FRAME_LEN, 0, NULL, NULL);
				if (length < 0) 
				{ 
					perror("recv pf pkt error"); 
				}
				//for(j = 0;j < length; j++)
				//{
				//	printf("%.2x ", *(buffer+j));
				//	if((j-9) % 10 == 0) printf("\n");
				//}
				recv_if_index = i;
				//printf("receive pkt from if_index %d\n", i);
				pkt_parse(s,buffer, length, recv_if_index);
				unsigned char *data = (unsigned char*)buffer+14;
				int pkt_type = *data;
				if(pkt_type == RREP_TYPE)
				{
					pkt_rrep* rp = (pkt_rrep*)data;
					if(!memcmp(dst_ip, rp->src_ip, 4))
					{
						//printf("wait_for_rrep: received the desired rrep\n");
						return;
					}
				}
			}
		}
	}
	return 0;
}

}

int pf_send1(int sock, void* pkt, int if_index, int pkt_type,unsigned char dst_ip[])
{
	//int sock = 0;
	int j = 0;
	/*target address*/
	struct sockaddr_ll socket_address;

	/*buffer for ethernet frame*/
	unsigned char buffer[ETH_FRAME_LEN]={};
 
	/*pointer to ethenet header*/
	unsigned char* etherhead = buffer;
	
	/*userdata in ethernet frame*/
	unsigned char* data = buffer + 14;
	
	/*another pointer to ethernet header*/
	struct ethhdr *eh = (struct ethhdr *)etherhead;
 
	int send_result = 0;
	int pkt_len = 0;
	unsigned char dest_mac[ETH_ALEN];
	unsigned char src_mac[ETH_ALEN];

	char hostname[128];
	unsigned char sip[30],dip[30];
	char *d_name, *s_name;
	//struct in_addr ipv4addr1, ipv4addr2;
	//struct hostent *he_d, *he_s;
	
	//printf("src mac %2x %2x %2x %2x %2x %2x\n", src_mac[0],src_mac[1],src_mac[2],src_mac[3],src_mac[4],src_mac[5]);
	/*other host MAC address*/

	if(pkt_type == RREQ_TYPE)
	{
		get_mac_by_if_index(if_index,src_mac);
		memset(dest_mac, 0xff,ETH_ALEN);

		//printf("pf_send: sedn a rreq  throuth if_index: %d\n", if_index);
	}
	else if(pkt_type == RREP_TYPE)
	{
		// find dst mac from route table
		route_index *index;
		route_index *new_index;
		pkt_rreq rq;
		pkt_rrep rp;
		struct hw_info *node = hw_list;

		index = find_dstip_in_hash(g_route_table, dst_ip);
		if(index == NULL)
		{
			// need to send rreq pkt and wait to receive it
			//printf("send back rrep but found no route index\n");
			//printf("starting send rreq first, and wair for reply...\n");
			for(node = hw_list; node != NULL; node = node->hw_next)
			{
				if(node->if_index!= 0)
				{
					fill_rreq_pkt(&rq, node->if_index, dst_ip,((pkt_rreq*)pkt)->src_port,
						((pkt_rreq*)pkt)->dst_port,0);
					pf_send1(sock, &rq, node->if_index, RREQ_TYPE, rq.dst_ip);
					//pf_send1(sock, &rq, node->if_index, RREQ_TYPE, rq.dst_ip);
				}
			}
			b_id++;// increase broadcast id
			// wait for rrep reply
			if(wait_for_rrep(sock, dst_ip)<0)
				return;
			//print_route_table();

			new_index = find_dstip_in_hash(g_route_table, dst_ip);
			if_index = new_index->data.next_hop_if;
			//printf("pf_send: sedn a rrep  throuth if_index: %d\n", if_index);
			get_mac_by_if_index(if_index,src_mac);
			memcpy(dest_mac, new_index->data.next_hop_addr,ETH_ALEN);
		}
		else
		{
			time_t new = time(NULL);
			time_t old = index->data.record_time;
			if(g_timeout> difftime(new, old))
			{
				memcpy(dest_mac, index->data.next_hop_addr,ETH_ALEN);
				if_index = index->data.next_hop_if;
				get_mac_by_if_index(if_index,src_mac);
				//printf("pf_send: sedn a rrep  throuth if_index: %d\n", if_index);
			}
			else
			{
				//printf("send back rrep but found the route index time out\n");
				//printf("starting send rreq first, and wair for reply...\n");

				for(node = hw_list; node != NULL; node = node->hw_next)
				{
					if(node->if_index!= 0)
					{
						fill_rreq_pkt(&rq, node->if_index, dst_ip,((pkt_rreq*)pkt)->src_port,
						((pkt_rreq*)pkt)->dst_port,0);
						pf_send1(sock, &rq, node->if_index, RREQ_TYPE, rq.dst_ip);
						//pf_send1(sock, &rq, node->if_index, RREQ_TYPE, rq.dst_ip);
					}
				}
				b_id++;// increase broadcast id
				// wait for rrep reply
				if(wait_for_rrep(sock, dst_ip)<0)
					return;
				//print_route_table();
				new_index = find_dstip_in_hash(g_route_table, dst_ip);
				if_index = new_index->data.next_hop_if;
				//printf("rrep: sedn a rrep  throuth if_index: %d\n", if_index);
				get_mac_by_if_index(if_index,src_mac);
				memcpy(dest_mac, new_index->data.next_hop_addr,ETH_ALEN);
			}
		}
	}
	else if(pkt_type == PAYLOAD_TYPE)//pkt for payload
	{	
		// find dst mac from route table
		route_index *index;
		route_index *new_index;
		pkt_rreq rq;
		pkt_rrep rp;
		struct hw_info *node = hw_list;

		index = find_dstip_in_hash(g_route_table, dst_ip);
		if(index == NULL)
		{
			// need to send rreq pkt and wait to receive it
			//printf("send payload but found no route index\n");
			//printf("starting send rreq first, and wair for reply...\n");
			for(node = hw_list; node != NULL; node = node->hw_next)
			{
				if(node->if_index!= 0)
				{
					fill_rreq_pkt(&rq, node->if_index, dst_ip,((pkt_payload*)pkt)->src_port,
						((pkt_payload*)pkt)->dst_port,((pkt_payload*)pkt)->force_flg);
					pf_send1(sock, &rq, node->if_index, RREQ_TYPE, rq.dst_ip);
					//pf_send1(sock, &rq, node->if_index, RREQ_TYPE, rq.dst_ip);
				}
			}
			b_id++;// increase broadcast id
			// wait for rrep reply
			if(wait_for_rrep(sock, dst_ip)<0)
				return;
			//print_route_table();

			new_index = find_dstip_in_hash(g_route_table, dst_ip);
			if_index = new_index->data.next_hop_if;
			//printf("pf_send: sedn a payload  throuth if_index: %d\n", if_index);
			get_mac_by_if_index(if_index,src_mac);
			//get_ip_by_if_index(if_index, ((pkt_payload*)pkt)->src_ip);
			memcpy(dest_mac, new_index->data.next_hop_addr,ETH_ALEN);
		}
		else
		{
			time_t new = time(NULL);
			time_t old = index->data.record_time;
			if(g_timeout> difftime(new, old))
			{
				memcpy(dest_mac, index->data.next_hop_addr,ETH_ALEN);
				if_index = index->data.next_hop_if;
				get_mac_by_if_index(if_index,src_mac);
				//get_ip_by_if_index(if_index, ((pkt_payload*)pkt)->src_ip);
				//printf("pf_send: sedn a payload  throuth if_index: %d\n", if_index);
			}
			else
			{
				//printf("send back payload but found the route index time out\n");
				//printf("starting send rreq first, and wair for reply...\n");

				for(node = hw_list; node != NULL; node = node->hw_next)
				{
					if(node->if_index!= 0)
					{
						fill_rreq_pkt(&rq, node->if_index, dst_ip,((pkt_payload*)pkt)->src_port,
						((pkt_payload*)pkt)->dst_port,((pkt_payload*)pkt)->force_flg);
						pf_send1(sock, &rq, node->if_index, RREQ_TYPE, rq.dst_ip);
						//pf_send1(sock, &rq, node->if_index, RREQ_TYPE, rq.dst_ip);
					}
				}
				b_id++;// increase broadcast id
				// wait for rrep reply
				if(wait_for_rrep(sock, dst_ip)<0)
				{
					return;
				}
				//print_route_table();
				new_index = find_dstip_in_hash(g_route_table, dst_ip);
				if_index = new_index->data.next_hop_if;
				//printf("payload: sedn a payload throuth if_index: %d\n", if_index);
				get_mac_by_if_index(if_index,src_mac);
				//get_ip_by_if_index(if_index, ((pkt_payload*)pkt)->src_ip);
				memcpy(dest_mac, new_index->data.next_hop_addr,ETH_ALEN);
			}
		}
	}
	/*RAW communication*/
	socket_address.sll_family   = PF_PACKET;


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
	socket_address.sll_addr[6]  = 0x00;/*not used*/
	socket_address.sll_addr[7]  = 0x00;/*not used*/


	/*set the frame header*/
	memcpy((void*)buffer, (void*)dest_mac, ETH_ALEN);
	memcpy((void*)(buffer+ETH_ALEN), (void*)src_mac, ETH_ALEN);
	eh->h_proto = htons(ETH_ODR_TYPE);
	/*fill the frame with some data*/
	if(pkt_type == RREQ_TYPE)
	{
		//fill_rreq_pkt(data, if_index, dst_ip, 0,0,0);
		memcpy(data, pkt,sizeof(pkt_rreq));
		pkt_len  = 14+ sizeof(pkt_rreq);
		//printf("pkt send\n");
		snprintf(sip,30, "%d.%d.%d.%d", ((pkt_rreq*)pkt)->src_ip[0],((pkt_rreq*)pkt)->src_ip[1],
				((pkt_rreq*)pkt)->src_ip[2],((pkt_rreq*)pkt)->src_ip[3]);

	}
	else if(pkt_type == RREP_TYPE)
	{
		memcpy(data, pkt,sizeof(pkt_rrep));
		pkt_len  = 14+ sizeof(pkt_rrep);
		snprintf(sip,30, "%d.%d.%d.%d", ((pkt_rrep*)pkt)->src_ip[0],((pkt_rrep*)pkt)->src_ip[1],
				((pkt_rrep*)pkt)->src_ip[2],((pkt_rrep*)pkt)->src_ip[3]);
	}
	else //if(pkt_type == PAYLOAD_TYPE)
	{
		
		memcpy(data, pkt,sizeof(pkt_payload)+((pkt_payload*)pkt)->len);
		pkt_len  = 14+ sizeof(pkt_payload)+((pkt_payload*)pkt)->len;
		//printf("send payload from source with len: %d data: %s\n", ((pkt_payload*)pkt)->len,data+sizeof(pkt_payload));
		//memcpy(data, pkt,sizeof(pkt_payload));
		//pkt_len  = 14+ sizeof(pkt_payload);
		snprintf(sip,30, "%d.%d.%d.%d", ((pkt_payload*)pkt)->src_ip[0],((pkt_payload*)pkt)->src_ip[1],
				((pkt_payload*)pkt)->src_ip[2],((pkt_payload*)pkt)->src_ip[3]);
	}

	//printf("pf_send: pkt_type %d, pkt_len %d\n", pkt_type,pkt_len);
	//for(j = 0;j < pkt_len; j++)
	//{
	//	printf("%.2x ", *((unsigned char*)buffer+j));
	//	if((j-9) % 10 == 0) printf("\n");
	//	fflush(stdout);
	//}
	gethostname(hostname, sizeof(hostname));

	snprintf(dip,30,"%d.%d.%d.%d",dst_ip[0],dst_ip[1],dst_ip[2],dst_ip[3]);

	// following code do not work get name by addr.
	//inet_pton(AF_INET, dip, &ipv4addr1);
	//he_d = gethostbyaddr(&ipv4addr1, sizeof(ipv4addr1), AF_INET);
	//inet_pton(AF_INET, sip, &ipv4addr2);
	//he_s = gethostbyaddr(&ipv4addr2, sizeof(ipv4addr2), AF_INET);

	d_name = find_vm_name(dip);
	s_name = find_vm_name(sip);
	//printf("dip %s, sip %s\n", dip, sip);
	//printf("d_name %s, s_name %s\n", d_name, s_name);
	printf("ODR at node %s: sending frame hdr src %s dest %.2x:%.2x:%.2x:%.2x:%.2x:%.2x ODR msg type %d src %s dest %s\n",
		hostname, hostname, dest_mac[0],dest_mac[1],dest_mac[2],dest_mac[3],dest_mac[4],dest_mac[5],pkt_type, s_name,d_name);
	/*send the packet*/
	send_result = sendto(sock, buffer, pkt_len/*ETH_FRAME_LEN*/, 0, 
	      (struct sockaddr*)&socket_address, sizeof(socket_address));
	if (send_result < 0) 
	{
		perror("send pf pkt error");
	}
	return 0;
}

int main (int argc, char **argv)
{

    int recv_if;
    int len;
    unsigned char recv_buff[ETH_FRAME_LEN]={};
    unsigned char send_buff[ETH_FRAME_LEN]={};
    unsigned char ip[4]={};

	if(argc < 2)
	{
		printf("usage error <staleness>\n");
		exit(1);
	}
	else
	{
		g_timeout = atoi(argv[1]);
		//printf("g_timeout = %d\n", g_timeout);
	}

	//initialize route table
	g_route_table = create_route_table();
	

	get_hw_if();

	bind_all_if();
	init_port_file();
	//struct sockaddr_ll socket_addr;
	int s1 = socket(AF_PACKET, SOCK_RAW, htons(ETH_ODR_TYPE));

    {
    	{
    		int maxfd = 0;	
			struct sockaddr_un odr_address;
			socklen_t odr_addr_len;
			int length;
			fd_set rset;
    		unlink(ODR_SERVICE_PATH);
			mkstemp(ODR_SERVICE_PATH);
			int i,j,s;

			domain_datagram_socket = Socket(AF_LOCAL, SOCK_DGRAM, 0);
			bzero(&odr_address, sizeof(odr_address));
			odr_address.sun_family = AF_LOCAL;
			strcpy(odr_address.sun_path, ODR_SERVICE_PATH);

			Bind(domain_datagram_socket, (SA *) &odr_address, sizeof(odr_address));
			for(;;)
			{
				FD_SET(domain_datagram_socket, &rset);
				for(i = 0; i < MAX_IF_NUM; i++)
				{
					if(if_sock_array[i]!=0)
					{
					FD_SET(if_sock_array[i], &rset);
					if(maxfd < if_sock_array[i])
						maxfd = if_sock_array[i];
					//printf("pf_recv: set sock %d for if %d\n", if_sock_array[i],i);
					}
				}
				maxfd = maxfd<domain_datagram_socket? domain_datagram_socket:maxfd;

				Select(maxfd+1, &rset, NULL, NULL, NULL);

				if(FD_ISSET(domain_datagram_socket, &rset))
				{

					bzero(recv_buff, sizeof(recv_buff));
					odr_addr_len = sizeof(odr_address);
					datamsg mm;
					length = Recvfrom(domain_datagram_socket, &mm, sizeof(mm), 0, &odr_address, &odr_addr_len);
		   			 //printf("Recieved from %s\n", odr_address.sun_path);
		   			 //recv_buff[length] = '\0';
		    		//printf("msg : %s\n",recv_buff);

		   			 unsigned char dst_ip[4];
		   			 unsigned char ip_buffer[30];
		    		unsigned char msg_buffer[ETH_FRAME_LEN];
		    		int dst_port = 0, src_port = 0;
		   			 int force_flg = 0;
		   			 datamsg *msg;

		   			 msg = (datamsg*)&mm;
		   			//sscanf(recv_buff, "%s    %d    ",ip_buffer, &dst_port);
		   			//memcpy(msg_buffer,recv_buff+l, 25);//25 represents the length of time format
		   			//sscanf(recv_buff+25+l, "%d",&force_flg);
		    		
		   			//printf("odr_recv: msg canonical ip %s\n", msg->canonical_ip);
					//printf("odr_recv: msg data %s\n", msg->data);
					//printf("odr_recv: port %d\n", msg->port);
					//printf("odr_recv: force_flg %d\n",msg->force_flg);
		   			strcpy(ip_buffer,msg->canonical_ip);
		   			strcpy(msg_buffer, msg->data);
		   			dst_port = msg->port;
		   			force_flg = msg->force_flg;
		   			sscanf(ip_buffer, "%d.%d.%d.%d", &dst_ip[0],&dst_ip[1],&dst_ip[2],&dst_ip[3]);
		   			find_temp_port(odr_address.sun_path, &src_port);
		   		    //printf("dstIP: %d.%d.%d.%d\n", dst_ip[0],dst_ip[1],dst_ip[2],dst_ip[3]);
		   		    //printf("msg buffer %s and force_flg %d\n", msg_buffer, force_flg);
		    		//printf("get temp src_port %d\n", src_port);
		    		fill_payload_pkt(send_buff, msg_buffer, dst_ip, src_port, dst_port, strlen(msg_buffer)+1,force_flg);
		    		pf_send1(s1,send_buff,0/*no use*/,PAYLOAD_TYPE,dst_ip);
		   		    
				}
				for(i = 0; i < MAX_IF_NUM; i++)
				{
					if((s=if_sock_array[i])!=0)
					{
						if(FD_ISSET(s,&rset))
						{
						    length = recvfrom(s, recv_buff, ETH_FRAME_LEN, 0, NULL, NULL);
							if (length < 0) 
							{ 
								perror("recv pf pkt error"); 
							}
							//for(j = 0;j < length; j++)
							//{
							//	printf("%.2x ", *(recv_buff+j));
							//	if((j-9) % 10 == 0) printf("\n");
							//	fflush(stdout);
							//}
							recv_if = i;
							//printf("receive pkt from if_index %d\n", i);
							pkt_parse(s,recv_buff, length, recv_if);
						}
					}
				}
			}
    	}
    	
	}
	exit(0);
}



