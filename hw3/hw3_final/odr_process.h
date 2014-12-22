#ifndef __ODR_PROCESS_H__
#define __ODR_PROCESS_H__

#include "hw_addrs.h"
#include <time.h>

#define PRO_TYPE_LEN 2
#define LO_NAME     "lo"
#define ETH_0_NAME  "eth0"
#define ETH_ODR_TYPE 0x8182

#define RREQ_TYPE    0
#define RREP_TYPE    1
#define PAYLOAD_TYPE 2

#define MAX_IF_NUM   10

#define ROUTE_TABLE_SIZE 10

#define PORT_FILE_NUM  100
typedef struct eth_header
{
	unsigned char dst_mac[IF_HADDR];
	unsigned char src_mac[IF_HADDR];
	unsigned char pro_type[PRO_TYPE_LEN];
}eth_header;

struct hw_info
{
  char    if_name[IF_NAME];	/* interface name, null terminated */
  char    if_haddr[IF_HADDR];	/* hardware address */
  int     if_index;		/* interface index */
  struct  sockaddr  ip_addr;	/* IP address */
  struct  hw_info  *hw_next;	/* next of these structures */
};

typedef struct pkt_rreq
{
	unsigned char pkt_type; // rreq = 0; rrep = 1; payload =2
	unsigned char rrep_set;
	unsigned char hop_count;
	unsigned char force_flg;
	unsigned char dst_ip[4];
	unsigned char src_ip[4];
	unsigned short dst_port;
	unsigned short src_port;
    int        broadcast_id;
}pkt_rreq;

typedef struct pkt_rrep
{
	unsigned char pkt_type; // rreq = 0; rrep = 1; payload =2

	unsigned char hop_count;
	unsigned char force_flg;
	unsigned char pad;      // unused byte for alignment
	unsigned char dst_ip[4];
	unsigned char src_ip[4];
	unsigned short dst_port;
	unsigned short src_port;
}pkt_rrep;

typedef struct pkt_payload
{
	unsigned char pkt_type; // rreq = 0; rrep = 1; payload =2
	unsigned char hop_count;
	unsigned short len;
	unsigned char dst_ip[4];
	unsigned char src_ip[4];
	unsigned short dst_port;
	unsigned short src_port;
	int force_flg;
}pkt_payload;

typedef struct route_info
{
	unsigned char dst_ip[4];
	unsigned char next_hop_if;
	unsigned char hop_count;
	unsigned char next_hop_addr[6];
    int           broadcast_id;
	time_t        record_time;
	int           force_flg;
}route_info;

typedef struct route_index
{
	route_info  data;
	struct route_index *next;
}route_index;

typedef struct route_table
{
	route_index* value[ROUTE_TABLE_SIZE];
}route_table;

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



typedef struct ip_vm
{
	unsigned char ip[30];
	char name[64];
}ip_vm;





#endif // defien __ODR_PROCESS_H__
