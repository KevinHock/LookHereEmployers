// nothing here
// take 1 argument
// ...
#include "unp.h"
#include <linux/if_packet.h>
#include <linux/if_ether.h>
//#include <linux/if_arp.h>
#include <stdlib.h>
#include "hw_addrs.h"
#include "odr_process.h"
#include "asgn3.h"

#define ETH_FRAME_LEN 100
struct hw_info* hw_list = NULL;
route_table* g_route_table = NULL;
unsigned char local_ip[4];
static int b_id = 1;//broadcast_id 
//unsigned char local_mac[ETH_ALEN];

int if_sock_array[MAX_IF_NUM];
int g_timeout;
int domain_datagram_socket;

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
		printf("%s :%s", hwb->if_name, "\n");
		
		if ( (sa = &(hwb->ip_addr)) != NULL)
			printf("         IP addr = %s\n", Sock_ntop_host(sa, sizeof(*sa)));
				
		prflag = 0;
		i = 0;
		do {
			if (hwb->if_haddr[i] != '\0') {
				prflag = 1;
				break;
			}
		} while (++i < IF_HADDR);

		if (prflag) {
			printf("         HW addr = ");
			ptr = hwb->if_haddr;
			i = IF_HADDR;
			do {
				printf("%.2x%s", *ptr++ & 0xff, (i == 1) ? " " : ":");
			} while (--i > 0);
		}

		printf("\n         interface index = %d\n\n", hwb->if_index);
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
		unsigned short dst_port, unsigned short len)
{
	pkt_payload *pkt = (pkt_payload*)data;

	pkt->pkt_type = PAYLOAD_TYPE;
	pkt->hop_count = 1;
	pkt->len = len;

	memcpy(pkt->dst_ip, dst_ip, 4);
	pkt->src_port = src_port;
	pkt->dst_port = dst_port;

	memcpy(data+sizeof(pkt_payload),load,len);
#if 0
	route_index *index;
	route_index *new_index;
	pkt_rreq rq;
	pkt_rrep rp;
	int if_index;
	struct hw_info *node = hw_list;

	index = find_dstip_in_hash(g_route_table, dst_ip);
	if(index == NULL)
	{
		// need to send rreq pkt and wait to receive it
		printf("send payload but found no route index\n");
		printf("starting send rreq first, and wair for reply...\n");
		for(node = hw_list; node != NULL; node = node->hw_next)
		{
			if(node->if_index!= 0)
			{
				fill_rreq_pkt(&rq, node->if_index, dst_ip,((pkt_payload*)pkt)->src_port,
					((pkt_payload*)pkt)->dst_port,0);
				pf_send1(sock, &rq, node->if_index, RREQ_TYPE, rq.dst_ip);
				//pf_send1(sock, &rq, node->if_index, RREQ_TYPE, rq.dst_ip);
			}
		}
		// wait for rrep reply
		wait_for_rrep(sock, dst_ip);
		//print_route_table();

		new_index = find_dstip_in_hash(g_route_table, dst_ip);
		if_index = new_index->data.next_hop_if;
		//fill in src_ip
		get_ip_by_if_index(if_index, ((pkt_payload*)pkt)->src_ip);
	}
	else
	{
		time_t new = time(NULL);
		time_t old = index->data.record_time;
		if(g_timeout> difftime(new, old))
		{
			if_index = index->data.next_hop_if;
			//fill in src_ip
			get_ip_by_if_index(if_index, ((pkt_payload*)pkt)->src_ip);
		}
		else
		{
			printf("send payload but found the route index time out\n");
			printf("starting send rreq first, and wair for reply...\n");

			for(node = hw_list; node != NULL; node = node->hw_next)
			{
				if(node->if_index!= 0)
				{
					fill_rreq_pkt(&rq, node->if_index, dst_ip,((pkt_payload*)pkt)->src_port,
					((pkt_payload*)pkt)->dst_port,0);
					pf_send1(sock, &rq, node->if_index, RREQ_TYPE, rq.dst_ip);
					//pf_send1(sock, &rq, node->if_index, RREQ_TYPE, rq.dst_ip);
				}
			}
			// wait for rrep reply
			wait_for_rrep(sock, dst_ip);
			//print_route_table();
			new_index = find_dstip_in_hash(g_route_table, dst_ip);
			if_index = new_index->data.next_hop_if;
			//fill in src_ip
			get_ip_by_if_index(if_index, ((pkt_payload*)pkt)->src_ip);
		}
	}	
#endif
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
		printf("recv rreq from error if_index\n");
		return;
	}

	memcpy(src_ip, data->src_ip, 4);

	get_ip_by_if_index(recv_if_index, dst_ip);

	//check if the rreq is sent by this node
	if(is_src_ip_local(src_ip))
	{
		//do nothing, discard the pkt
		printf("receive duplicated rreq pkt\n");
		return;
	}
	//pkt_print(buffer);

	get_ip_by_if_index(recv_if_index, if_ip);
	printf("recv rreq from if_index %d with ip %d.%d.%d.%d\n", recv_if_index,if_ip[0],if_ip[1],
		if_ip[2],if_ip[3]);
	printf("                 dst_ip: %d.%d.%d.%d\n" ,data->dst_ip[0],data->dst_ip[1],
		data->dst_ip[2],data->dst_ip[3]);
	printf("                 src_ip: %d.%d.%d.%d\n",data->src_ip[0],data->src_ip[1],
		data->src_ip[2],data->src_ip[3]);	//build a new route index
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
		printf("rreq: insert new route index\n");
		print_route_table();
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
			printf("rreq: update route table due to time out\n");
			print_route_table();

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
				printf("rreq: udate route table due to better result\n");
				print_route_table();
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
						printf("rreq: udate route table due to force flag result\n");
						print_route_table();
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
			printf("rreq: receive a rreq pkt with rrep_set and don't reply\n");
			return;
		}
		else
		{
			// send back a rrep
			printf("rreq: sedn back a rrep throuth if_index: %d\n", recv_if_index);
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
				printf("intermediate node find force_flg set and stay calm\n");
			}
			else if(index != NULL)
			{
			// if force_flg is not set, then check if intermediate node can
			// send a rrep 
				time_t new = time(NULL);
				time_t old = index->data.record_time;
				if(g_timeout> difftime(new, old))
				{
					printf("rreq: send a rrep back and propagate rreq packet\n");
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
							printf("rreq: send a rreq relay through if_index: %d\n", node->if_index);
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
			printf("rreq: should send a rrep back but stop by rrep_set\n");
		}
		//relay rreq
		memcpy(&rq, data, sizeof(pkt_rreq));
		for(node = hw_list; node != NULL; node = node->hw_next)
		{
			if(node->if_index!= recv_if_index)
			{
				printf("rreq: send a rreq relay through if_index: %d\n", node->if_index);
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
		printf("recv rrep from error if_index\n");
		return;
	}

	memcpy(src_ip, data->src_ip, 4);

	get_ip_by_if_index(recv_if_index, dst_ip);

	//check if the rrep is sent by this node
	if(is_src_ip_local(src_ip))
	{
		//do nothing, discard the pkt
		printf("receive duplicated rrep pkt\n");
		return;
	}
	//pkt_print(buffer);
	get_ip_by_if_index(recv_if_index, if_ip);
	printf("recv rrep from if_index %d with ip %d.%d.%d.%d\n", recv_if_index,if_ip[0],if_ip[1],
		if_ip[2],if_ip[3]);
	printf("                 dst_ip: %d.%d.%d.%d\n" ,data->dst_ip[0],data->dst_ip[1],
		data->dst_ip[2],data->dst_ip[3]);
	printf("                 src_ip: %d.%d.%d.%d\n",data->src_ip[0],data->src_ip[1],
		data->src_ip[2],data->src_ip[3]);
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
		printf("rrep: insert new route index\n");
		print_route_table();
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
			printf("rrep: update route table due to time out\n");
			print_route_table();
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
				printf("rrep: update route table due to better hop\n");
				print_route_table();
			}

		}
	}

	//check if the dst_ip is ip of the recv_if_index
	if(!memcmp(dst_ip, data->dst_ip, 4))
	{
		printf("received the rrep for my rreq \n");
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
		printf("recv payload from error if_index\n");
		return;
	}

	memcpy(src_ip, data->src_ip, 4);

	get_ip_by_if_index(recv_if_index, dst_ip);

	//check if the rrep is sent by this node
	if(is_src_ip_local(src_ip))
	{
		//do nothing, discard the pkt
		printf("receive duplicated payload pkt\n");
		return;
	}
	//pkt_print(buffer);
	get_ip_by_if_index(recv_if_index, if_ip);
	printf("recv payload from if_index %d with ip %d.%d.%d.%d\n", recv_if_index,if_ip[0],if_ip[1],
		if_ip[2],if_ip[3]);
	printf("                 dst_ip: %d.%d.%d.%d\n" ,data->dst_ip[0],data->dst_ip[1],
		data->dst_ip[2],data->dst_ip[3]);
	printf("                 src_ip: %d.%d.%d.%d\n",data->src_ip[0],data->src_ip[1],
		data->src_ip[2],data->src_ip[3]);
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
		printf("payload: insert new route index\n");
		print_route_table();
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
			printf("payload: update route table due to time out\n");
			print_route_table();
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
				printf("payload: update route table due to better hop\n");
				print_route_table();
			}

		}
	}

	//check if the dst_ip is ip of the recv_if_index
	if(!memcmp(dst_ip, data->dst_ip, 4))
	{
		printf("received the payload for myself \n");
		unsigned char b[100]={};
		memcpy(b,buffer+sizeof(pkt_payload),data->len);
		//pkt_print(data);
		b[data->len]='\0';
		printf("receved data in payload size %d\n", (data->len));
		int i = 0;
		for(; i < data->len; i++)
		{
			printf("%c", b[i]);
		}
		printf("\n");
		fflush(stdout);
		//handle the received msg
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
		rreq_op(sock, data, src_mac, recv_if_index);
	}
	else if(pkt_type == RREP_TYPE)
	{
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
				for(j = 0;j < length; j++)
				{
					printf("%.2x ", *(buffer+j));
					if((j-9) % 10 == 0) printf("\n");
					fflush(stdout);
				}
				recv_if_index = i;
				printf("receive pkt from if_index %d\n", i);
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


void wait_for_rrep(int sock, unsigned char dst_ip[])
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


	FD_ZERO(&rset);


while(1)
{
	for(i = 0; i < MAX_IF_NUM; i++)
	{
		if(if_sock_array[i]!=0)
		{
			FD_SET(if_sock_array[i], &rset);
			if(maxfd < if_sock_array[i])
				maxfd = if_sock_array[i];
			printf("wait_for_rrep: set sock %d for if %d\n", if_sock_array[i],i);
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
				//{
				//	printf("%.2x ", *(buffer+j));
				//	if((j-9) % 10 == 0) printf("\n");
				//}
				recv_if_index = i;
				printf("receive pkt from if_index %d\n", i);
				pkt_parse(s,buffer, length, recv_if_index);
				printf("adfffffff\n");
				unsigned char *data = (unsigned char*)buffer+14;
				int pkt_type = *data;
				if(pkt_type == RREP_TYPE)
				{
					pkt_rrep* rp = (pkt_rrep*)data;
					if(!memcmp(dst_ip, rp->src_ip, 4))
					{
						printf("wait_for_rrep: received the desired rrep\n");
						return;
					}
				}
			}
		}
	}
}

#if 0
    while(1)
    {
		pf_recv1(sock,&recv_if,recv_buff, &len);

		pkt_parse(sock,recv_buff, len, recv_if);

		unsigned char *data = (unsigned char*)recv_buff+14;
		int pkt_type = *data;
		if(pkt_type == RREP_TYPE)
		{
			pkt_rrep* rp = (pkt_rrep*)data;
			if(!memcmp(dst_ip, rp->src_ip, 4))
			{
				printf("wait_for_rrep: received the desired rrep\n");
				return;
			}
		}
		// may need some timout mechanism to avoid infinite loop
	}
#endif
}

#if 0
void wait_for_payload(int sock, unsigned char dst_ip[])
{
	int recv_if;
    int len;
    unsigned char recv_buff[ETH_FRAME_LEN]={};

    while(1)
    {
		pf_recv1(sock,&recv_if,recv_buff, &len);

		pkt_parse(sock,recv_buff, len, recv_if);

		unsigned char *data = (unsigned char*)recv_buff+14;
		int pkt_type = *data;
		if(pkt_type == PAYLOAD_TYPE)
		{
			pkt_payload* rp = (pkt_payload*)data;
			if(!memcmp(dst_ip, rp->src_ip, 4))
			{
				printf("wait_for_payload: received the desired rrep\n");
				return;
			}
		}
		// may need some timout mechanism to avoid infinite loop
	}
}	
#endif
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

	
	//printf("src mac %2x %2x %2x %2x %2x %2x\n", src_mac[0],src_mac[1],src_mac[2],src_mac[3],src_mac[4],src_mac[5]);
	/*other host MAC address*/

	if(pkt_type == RREQ_TYPE)
	{
		get_mac_by_if_index(if_index,src_mac);
		memset(dest_mac, 0xff,ETH_ALEN);
		//dest_mac[0]=0;
		//dest_mac[1]=0x0c;
		//dest_mac[2]=0x29;
		//dest_mac[3]=0x9e;
		//dest_mac[4] =0x80;
		//dest_mac[5]=0x87;
		printf("pf_send: sedn a rreq  throuth if_index: %d\n", if_index);
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
			printf("send back rrep but found no route index\n");
			printf("starting send rreq first, and wair for reply...\n");
			for(node = hw_list; node != NULL; node = node->hw_next)
			{
				if(node->if_index!= 0)
				{
					fill_rreq_pkt(&rq, node->if_index, dst_ip,((pkt_rrep*)pkt)->src_port,
						((pkt_rrep*)pkt)->dst_port,0);
					pf_send1(sock, &rq, node->if_index, RREQ_TYPE, rq.dst_ip);
					//pf_send1(sock, &rq, node->if_index, RREQ_TYPE, rq.dst_ip);
				}
			}
			b_id++;// increase broadcast id
			// wait for rrep reply
			wait_for_rrep(sock, dst_ip);
			print_route_table();

			new_index = find_dstip_in_hash(g_route_table, dst_ip);
			if_index = new_index->data.next_hop_if;
			printf("pf_send: sedn a rrep  throuth if_index: %d\n", if_index);
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
				printf("pf_send: sedn a rrep  throuth if_index: %d\n", if_index);
			}
			else
			{
				printf("send back rrep but found the route index time out\n");
				printf("starting send rreq first, and wair for reply...\n");

				for(node = hw_list; node != NULL; node = node->hw_next)
				{
					if(node->if_index!= 0)
					{
						fill_rreq_pkt(&rq, node->if_index, dst_ip,((pkt_rrep*)pkt)->src_port,
						((pkt_rrep*)pkt)->dst_port,0);
						pf_send1(sock, &rq, node->if_index, RREQ_TYPE, rq.dst_ip);
						//pf_send1(sock, &rq, node->if_index, RREQ_TYPE, rq.dst_ip);
					}
				}
				b_id++;// increase broadcast id
				// wait for rrep reply
				wait_for_rrep(sock, dst_ip);
				print_route_table();
				new_index = find_dstip_in_hash(g_route_table, dst_ip);
				if_index = new_index->data.next_hop_if;
				printf("rrep: sedn a rrep  throuth if_index: %d\n", if_index);
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
			printf("send payload but found no route index\n");
			printf("starting send rreq first, and wair for reply...\n");
			for(node = hw_list; node != NULL; node = node->hw_next)
			{
				if(node->if_index!= 0)
				{
					fill_rreq_pkt(&rq, node->if_index, dst_ip,((pkt_payload*)pkt)->src_port,
						((pkt_payload*)pkt)->dst_port,0);
					pf_send1(sock, &rq, node->if_index, RREQ_TYPE, rq.dst_ip);
					//pf_send1(sock, &rq, node->if_index, RREQ_TYPE, rq.dst_ip);
				}
			}
			b_id++;// increase broadcast id
			// wait for rrep reply
			wait_for_rrep(sock, dst_ip);
			print_route_table();

			new_index = find_dstip_in_hash(g_route_table, dst_ip);
			if_index = new_index->data.next_hop_if;
			printf("pf_send: sedn a payload  throuth if_index: %d\n", if_index);
			get_mac_by_if_index(if_index,src_mac);
			get_ip_by_if_index(if_index, ((pkt_payload*)pkt)->src_ip);
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
				get_ip_by_if_index(if_index, ((pkt_payload*)pkt)->src_ip);
				printf("pf_send: sedn a payload  throuth if_index: %d\n", if_index);
			}
			else
			{
				printf("send back payload but found the route index time out\n");
				printf("starting send rreq first, and wair for reply...\n");

				for(node = hw_list; node != NULL; node = node->hw_next)
				{
					if(node->if_index!= 0)
					{
						fill_rreq_pkt(&rq, node->if_index, dst_ip,((pkt_payload*)pkt)->src_port,
						((pkt_payload*)pkt)->dst_port,0);
						pf_send1(sock, &rq, node->if_index, RREQ_TYPE, rq.dst_ip);
						//pf_send1(sock, &rq, node->if_index, RREQ_TYPE, rq.dst_ip);
					}
				}
				b_id++;// increase broadcast id
				// wait for rrep reply
				wait_for_rrep(sock, dst_ip);
				print_route_table();
				new_index = find_dstip_in_hash(g_route_table, dst_ip);
				if_index = new_index->data.next_hop_if;
				printf("payload: sedn a payload throuth if_index: %d\n", if_index);
				get_mac_by_if_index(if_index,src_mac);
				get_ip_by_if_index(if_index, ((pkt_payload*)pkt)->src_ip);
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
	}
	else if(pkt_type == RREP_TYPE)
	{
		memcpy(data, pkt,sizeof(pkt_rrep));
		pkt_len  = 14+ sizeof(pkt_rrep);
	}
	else //if(pkt_type == PAYLOAD_TYPE)
	{
		
		memcpy(data, pkt,sizeof(pkt_payload)+((pkt_payload*)pkt)->len);
		pkt_len  = 14+ sizeof(pkt_payload)+((pkt_payload*)pkt)->len;
		printf("send payload from source with len: %d data: %s\n", ((pkt_payload*)pkt)->len,data+sizeof(pkt_payload));
		//memcpy(data, pkt,sizeof(pkt_payload));
		//pkt_len  = 14+ sizeof(pkt_payload);
	}

	printf("pf_send: pkt_type %d, pkt_len %d\n", pkt_type,pkt_len);
	//for(j = 0;j < pkt_len; j++)
	//{
	//	printf("%.2x ", *((unsigned char*)buffer+j));
	//	if((j-9) % 10 == 0) printf("\n");
	//	fflush(stdout);
	//}
	/*send the packet*/
	send_result = sendto(sock, buffer, pkt_len/*ETH_FRAME_LEN*/, 0, 
	      (struct sockaddr*)&socket_address, sizeof(socket_address));
	if (send_result < 0) 
	{
		perror("send pf pkt error");
	}
	return 0;
}

int send_payload(int sock, void* data, unsigned char dst_ip[])
{
	return 0;	
}

#if 0
int pf_recv1(int s, int *recv_if_index, unsigned char *data, int *data_len)
{
	struct sockaddr_ll socket_address;
	socklen_t len;
	int i = 0;
	//unsigned char* buffer = (unsigned char*)malloc(ETH_FRAME_LEN); /*Buffer for ethernet frame*/
	int length = 0; /*length of the received frame*/ 

	*data_len = recvfrom(s, data, ETH_FRAME_LEN, 0, (struct sockaddr*)&socket_address, &len);
	if (*data_len < 0) 
	{ 
		perror("recv pf pkt error"); 
	}
	//for(i = 0;i < *data_len; i++)
	//{
	//	printf("%.2x ", *(data+i));
	//	if((i-9) % 10 == 0) printf("\n");
	//}
	//printf("pkt ifindex %d\n", socket_address.sll_ifindex);
	*recv_if_index = socket_address.sll_ifindex;
	return 0;
}
#endif

#define PORT_FILE_NUM  100
typedef struct port_file
{
	unsigned char name[64];
	int port;
	time_t tt;
	int in_use;
}port_file;

typedef struct port_file_head
{
	port_file pf[PORT_FILE_NUM];
	int next_pos;
}port_file_head;

port_file_head g_port_file;

void find_temp_port(const char* s, int* port)
{
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

		g_port_file.next_pos++;
	}
}


void init_port_file(void)
{
	int i;
	g_port_file.next_pos = 0;
	for(i = 0; i < PORT_FILE_NUM; i++)
	{
		g_port_file.pf[i].port = 22000+i;
		g_port_file.pf[i].tt = 0;
		g_port_file.pf[i].in_use = 0;
		memset(g_port_file.pf[i].name, 0, 64);
	}

	return;
}

int recv_all(int domain_datagram_socket, int sendsock)
{
	int recv_if;
    int length;
    int i,j,s;
    unsigned char recv_buff[ETH_FRAME_LEN]={};
    unsigned char send_buff[ETH_FRAME_LEN]={};

	int maxfd = 0;	
	struct sockaddr_un odr_address;
	//struct sockaddr_ll raw_address;  
	socklen_t odr_addr_len;

	fd_set rset;

	FD_ZERO(&rset);

	{

		printf("domian sock %d\n",domain_datagram_socket);
		#if 1
		for(i = 0; i < MAX_IF_NUM; i++)
		{
			if(if_sock_array[i]!=0)
			{
				FD_SET(if_sock_array[i], &rset);
				if(maxfd < if_sock_array[i])
					maxfd = if_sock_array[i];
				printf("pf_recv: set sock %d for if %d\n", if_sock_array[i],i);
			}
		}
		#endif
		FD_SET(domain_datagram_socket, &rset);
		maxfd = maxfd<domain_datagram_socket? domain_datagram_socket:maxfd;

		if(select(maxfd+1, &rset, NULL, NULL, NULL) < 0){
			/* (never returns 0 on timeout) */
			perror("select less than 0");
			//if(errno == EINTR)
			//	return -1;
			//err_sys("select failed");
		}
		printf("after select\n");
		#if 1
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
					//pkt_parse(s,recv_buff, length, recv_if);
				}
			}
		}
		#endif
		#if 0
		if(FD_ISSET(sendsock, &rset))
		{
			length = recvfrom(sendsock, recv_buff, ETH_FRAME_LEN, 0, NULL, NULL);
					if (length < 0) 
					{ 
						perror("recv pf pkt error"); 
					}
					for(j = 0;j < length; j++)
					{
						printf("%.2x ", *(recv_buff+j));
						if((j-9) % 10 == 0) printf("\n");
						fflush(stdout);
					}
					recv_if = i;
					printf("receive pkt from if_index %d\n", i);
					pkt_parse(s,recv_buff, length, recv_if);
		}
		#endif
		if(FD_ISSET(domain_datagram_socket, &rset))
		{

			bzero(recv_buff, sizeof(recv_buff));
			odr_addr_len = sizeof(odr_address);
			length = Recvfrom(domain_datagram_socket, recv_buff, sizeof(recv_buff), 0, &odr_address, &odr_addr_len);
		    printf("Recieved from %s\n", odr_address.sun_path);
		    recv_buff[length] = '\0';
		    printf("msg : %s\n",recv_buff);

		    unsigned char dst_ip[4];
		    unsigned char ip_buffer[30];
		    unsigned char msg_buffer[ETH_FRAME_LEN];
		    int dst_port = 0, src_port = 0;
		    int force_flg = 0;
		    sscanf(recv_buff, "%s    %d    %s    %d",ip_buffer, &dst_port,msg_buffer, &force_flg);
		    sscanf(ip_buffer, "%d.%d.%d.%d", &dst_ip[0],&dst_ip[1],&dst_ip[2],&dst_ip[3]);
		    find_temp_port(odr_address.sun_path, &src_port);
		    //printf("dstIP: %d.%d.%d.%d\n", dst_ip[0],dst_ip[1],dst_ip[2],dst_ip[3]);
		    //printf("msg buffer %s\n", msg_buffer);
		    printf("get temp src_port %d\n", src_port);
		    fill_payload_pkt(send_buff, msg_buffer, dst_ip, src_port, dst_port, force_flg);
		    pf_send1(sendsock,send_buff,0/*no use*/,PAYLOAD_TYPE,dst_ip);
		}
	}
}

int main (int argc, char **argv)
{


#if 1// test for odr function
    int recv_if;
    int len;
    unsigned char recv_buff[ETH_FRAME_LEN]={};
    unsigned char send_buff[ETH_FRAME_LEN]={};
    unsigned char ip[4]={};

	if(argc < 2)
	{
		printf("usage error\n");
		exit(1);
	}
	else
	{
		g_timeout = atoi(argv[1]);
		printf("g_timeout = %d\n", g_timeout);
	}

	//initialize route table
	g_route_table = create_route_table();
	

	get_hw_if();

	bind_all_if();

	//struct sockaddr_ll socket_addr;
	int s1 = socket(AF_PACKET, SOCK_RAW, htons(ETH_ODR_TYPE));
#if 0
    if(argc == 3)
    {
    	sscanf(argv[2], "%d.%d.%d.%d", &ip[0],&ip[1],&ip[2],&ip[3]);
    	//while(1)
    	{
    		#if 0
    	    fill_rreq_pkt(send_buff,3,ip,0,0,0);
    		pf_send1(s1,send_buff, 3,RREQ_TYPE,ip);
    		//pf_send1(s1,send_buff, 3,RREQ_TYPE,ip);
    		fill_rreq_pkt(send_buff,4,ip,0,0,0);
    		pf_send1(s1,send_buff, 4,RREQ_TYPE,ip);
    		//pf_send1(s1,send_buff, 4,RREQ_TYPE,ip);

			#endif

    		#if 1
    		const char ss[]="ssss";
    		fill_payload_pkt(send_buff, ss, ip,0,0,sizeof(ss));
    		
    		//printf("payload size %d , %s\n", ((pkt_payload*)send_buff)->len, send_buff+sizeof(pkt_payload));
    		//pkt_print(send_buff);
    		pf_send1(s1,send_buff,0/*no use*/,PAYLOAD_TYPE,ip);
    		#endif
    	}
    	while(1)
    	{
			pf_recv();
    		//pkt_parse(s1,recv_buff, len, recv_if);
    	}
    }
    else
 #endif
    {
    	if(fork() == 0)
    	{
    		int maxfd = 0;	
			struct sockaddr_un odr_address;
			//struct sockaddr_ll raw_address;  
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
				printf("pf_recv: set sock %d for if %d\n", if_sock_array[i],i);
				}
				}
				maxfd = maxfd<domain_datagram_socket? domain_datagram_socket:maxfd;

				Select(maxfd+1, &rset, NULL, NULL, NULL);

				if(FD_ISSET(domain_datagram_socket, &rset))
				{

				bzero(recv_buff, sizeof(recv_buff));
				odr_addr_len = sizeof(odr_address);
				length = Recvfrom(domain_datagram_socket, recv_buff, sizeof(recv_buff), 0, &odr_address, &odr_addr_len);
		   		 printf("Recieved from %s\n", odr_address.sun_path);
		   		 recv_buff[length] = '\0';
		    	printf("msg : %s\n",recv_buff);

		   		 unsigned char dst_ip[4];
		   		 unsigned char ip_buffer[30];
		    		unsigned char msg_buffer[ETH_FRAME_LEN];
		    		int dst_port = 0, src_port = 0;
		   		 int force_flg = 0;
		   		 sscanf(recv_buff, "%s    %d    %s    %d",ip_buffer, &dst_port,msg_buffer, &force_flg);
		    		sscanf(ip_buffer, "%d.%d.%d.%d", &dst_ip[0],&dst_ip[1],&dst_ip[2],&dst_ip[3]);
		   		 //find_temp_port(odr_address.sun_path, &src_port);
		   		 //printf("dstIP: %d.%d.%d.%d\n", dst_ip[0],dst_ip[1],dst_ip[2],dst_ip[3]);
		   		 //printf("msg buffer %s\n", msg_buffer);
		    		printf("get temp src_port %d\n", src_port);
		    		fill_payload_pkt(send_buff, msg_buffer, dst_ip, src_port, dst_port, force_flg);
		    		pf_send1(s1,send_buff,0/*no use*/,PAYLOAD_TYPE,dst_ip);
		   		    //fill_rreq_pkt(send_buff,3,dst_ip,0,0,0);
    				//pf_send1(s1,send_buff, 3,RREQ_TYPE,dst_ip);
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
					for(j = 0;j < length; j++)
					{
						printf("%.2x ", *(recv_buff+j));
						if((j-9) % 10 == 0) printf("\n");
						fflush(stdout);
					}
					recv_if = i;
					printf("receive pkt from if_index %d\n", i);
					pkt_parse(s,recv_buff, length, recv_if);
				}
			}
		}
			}
    	}
    	else{
    	for(;;)
    	{
			//pf_recv();
			//pkt_parse(s1,recv_buff, len, recv_if);
		}
		}
	}
#endif

#if 0
	int recv_if;
    int length;
    int i,j;
    int s, sendsock;
    unsigned char recv_buff[ETH_FRAME_LEN]={};
    unsigned char send_buff[ETH_FRAME_LEN]={};

	int maxfd = 0;	
	struct sockaddr_un odr_address;
	//struct sockaddr_ll raw_address;  
	socklen_t odr_addr_len;

	fd_set rset;

	if(argc!=2)
		err_quit("usage: odr_process <staleness in seconds>");
	g_timeout = atoi(argv[1]);
	printf("stainless time %d\n", g_timeout);

	//initialize route table
	g_route_table = create_route_table();
	

	get_hw_if();
	bind_all_if();
	//init_port_file();
	//struct sockaddr_ll socket_addr;
	sendsock = socket(AF_PACKET, SOCK_RAW, htons(ETH_ODR_TYPE));

#if 0
	if(fork()==0)
	{
		unlink(ODR_SERVICE_PATH);
		mkstemp(ODR_SERVICE_PATH);

		domain_datagram_socket = Socket(AF_LOCAL, SOCK_DGRAM, 0);
		bzero(&odr_address, sizeof(odr_address));
		odr_address.sun_family = AF_LOCAL;
		strcpy(odr_address.sun_path, ODR_SERVICE_PATH);

		Bind(domain_datagram_socket, (SA *) &odr_address, sizeof(odr_address));
		for(;;)
		{
				FD_SET(domain_datagram_socket, &rset);
				maxfd = maxfd<domain_datagram_socket? domain_datagram_socket:maxfd;

			if(select(maxfd+1, &rset, NULL, NULL, NULL) < 0){
			/* (never returns 0 on timeout) */
			perror("select less than 0");
			//if(errno == EINTR)
			//	return -1;
			//err_sys("select failed");
			}
			if(FD_ISSET(domain_datagram_socket, &rset))
			{

			bzero(recv_buff, sizeof(recv_buff));
			odr_addr_len = sizeof(odr_address);
			length = Recvfrom(domain_datagram_socket, recv_buff, sizeof(recv_buff), 0, &odr_address, &odr_addr_len);
		    printf("Recieved from %s\n", odr_address.sun_path);
		    recv_buff[length] = '\0';
		    printf("msg : %s\n",recv_buff);

		    unsigned char dst_ip[4];
		    unsigned char ip_buffer[30];
		    unsigned char msg_buffer[ETH_FRAME_LEN];
		    int dst_port = 0, src_port = 0;
		    int force_flg = 0;
		    sscanf(recv_buff, "%s    %d    %s    %d",ip_buffer, &dst_port,msg_buffer, &force_flg);
		    sscanf(ip_buffer, "%d.%d.%d.%d", &dst_ip[0],&dst_ip[1],&dst_ip[2],&dst_ip[3]);
		    //find_temp_port(odr_address.sun_path, &src_port);
		    //printf("dstIP: %d.%d.%d.%d\n", dst_ip[0],dst_ip[1],dst_ip[2],dst_ip[3]);
		    //printf("msg buffer %s\n", msg_buffer);
		    printf("get temp src_port %d\n", src_port);
		    //fill_payload_pkt(send_buff, msg_buffer, dst_ip, src_port, dst_port, force_flg);
		    //pf_send1(sendsock,send_buff,0/*no use*/,PAYLOAD_TYPE,dst_ip);
		    fill_rreq_pkt(send_buff,3,dst_ip,0,0,0);
    		pf_send1(sendsock,send_buff, 3,RREQ_TYPE,dst_ip);
			}
		}
		exit(0);
	}
	else
#endif
	//while(1)
	{
		//recv_all(domain_datagram_socket,sendsock);
		//recv_all(domain_datagram_socket,sendsock);
		printf("waiting\n");
		pf_recv();
		#if 0
		printf("\n");
		FD_SET(domain_datagram_socket, &rset);
		//FD_SET(4, &rset);
		//FD_SET(5, &rset);
		
		#if 0
		for(i = 0; i < MAX_IF_NUM; i++)
		{
			if(if_sock_array[i]!=0)
			{
				FD_SET(if_sock_array[i], &rset);
				if(maxfd < if_sock_array[i])
					maxfd = if_sock_array[i];
				printf("pf_recv: set sock %d for if %d\n", if_sock_array[i],i);
			}
		}
		#endif
		//maxfd = 5;
		printf("\n");
		maxfd = maxfd<domain_datagram_socket? domain_datagram_socket:maxfd;
		printf("\n");
		if(select(maxfd+1, &rset, NULL, NULL, NULL) < 0){
			/* (never returns 0 on timeout) */
			perror("select less than 0");
			if(errno == EINTR)
				continue;
			err_sys("select failed");
		}
		printf("after select\n");
		#if 0
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
					for(j = 0;j < length; j++)
					{
						printf("%.2x ", *(recv_buff+j));
						if((j-9) % 10 == 0) printf("\n");
						fflush(stdout);
					}
					recv_if = i;
					printf("receive pkt from if_index %d\n", i);
					pkt_parse(s,recv_buff, length, recv_if);
				}
			}
		}
		#endif
		#if 0
		if(FD_ISSET(sendsock, &rset))
		{
			length = recvfrom(sendsock, recv_buff, ETH_FRAME_LEN, 0, NULL, NULL);
					if (length < 0) 
					{ 
						perror("recv pf pkt error"); 
					}
					for(j = 0;j < length; j++)
					{
						printf("%.2x ", *(recv_buff+j));
						if((j-9) % 10 == 0) printf("\n");
						fflush(stdout);
					}
					recv_if = i;
					printf("receive pkt from if_index %d\n", i);
					pkt_parse(s,recv_buff, length, recv_if);
		}
		#endif
		if(FD_ISSET(domain_datagram_socket, &rset))
		{

			bzero(recv_buff, sizeof(recv_buff));
			odr_addr_len = sizeof(odr_address);
			length = Recvfrom(domain_datagram_socket, recv_buff, sizeof(recv_buff), 0, &odr_address, &odr_addr_len);
		    printf("Recieved from %s\n", odr_address.sun_path);
		    recv_buff[length] = '\0';
		    printf("msg : %s\n",recv_buff);

		    unsigned char dst_ip[4];
		    unsigned char ip_buffer[30];
		    unsigned char msg_buffer[ETH_FRAME_LEN];
		    int dst_port = 0, src_port = 0;
		    int force_flg = 0;
		    sscanf(recv_buff, "%s    %d    %s    %d",ip_buffer, &dst_port,msg_buffer, &force_flg);
		    sscanf(ip_buffer, "%d.%d.%d.%d", &dst_ip[0],&dst_ip[1],&dst_ip[2],&dst_ip[3]);
		    find_temp_port(odr_address.sun_path, &src_port);
		    //printf("dstIP: %d.%d.%d.%d\n", dst_ip[0],dst_ip[1],dst_ip[2],dst_ip[3]);
		    //printf("msg buffer %s\n", msg_buffer);
		    printf("get temp src_port %d\n", src_port);
		    fill_payload_pkt(send_buff, msg_buffer, dst_ip, src_port, dst_port, force_flg);
		    pf_send1(sendsock,send_buff,0/*no use*/,PAYLOAD_TYPE,dst_ip);
		}
		#endif
	}



#endif
	exit(0);
}



