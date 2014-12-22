#include "unp.h"
#include "hw_addrs.h"
#include <assert.h>
#include <netinet/ip.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>

#define DEBUG 			   if ( debug ) printf
#define WELLKNOWN_PATH     "/tmp/some0th3rd1r"
#define MAX_VISITS 		   100
#define MULTICAST_PORT     31337
#define MULTICAST_ADDRESS  "226.226.226.226"
#define MY_IP_PROTO        0xde
#define MY_IP_ID           0xF182
#define MY_ICMP_ID         0xD087
#define MY_ICMP_ECHO       8
#define ETH_0_NAME  	   "eth0"


typedef struct previous_ping_targets{
	struct in_addr previous_senders[MAX_VISITS];
	int 	index;
} ping_targets;

typedef struct packet
{
	struct in_addr  payload[MAX_VISITS];
	struct in_addr  multicast_addr;
	unsigned short 	 multicast_port;
	unsigned char	 index;
	unsigned char   total;
} tour;

struct my_eth_header
{
	unsigned char	h_dest[ETH_ALEN];	
	unsigned char	h_source[ETH_ALEN];
	unsigned short	h_proto;	
} __attribute__((packed));
typedef struct my_eth_header my_eth_header;

typedef struct my_ip_header
{
	
	//uint8_t   version:4;	  
	//uint8_t   ihl:4;	
	uint8_t   v_hl;
	uint8_t   tos;	
	uint16_t  tot_len;	
	uint16_t  id;	
	uint16_t  fragment;	
	uint8_t   ttl;	
	uint8_t   protocol;	
	uint16_t  check;	
	uint32_t  saddr;	
	uint32_t  daddr;
}my_ip_header;

typedef struct my_icmp_header
{
	unsigned char      icmph_type;
	unsigned char      icmph_code;
	unsigned short int icmph_checksum;
	unsigned short int icmph_id;
	unsigned short int icmph_sequence;
}my_icmp_header;

struct hwaddr 
{
    int 			sll_ifindex;
    unsigned short 	sll_hatype;
    unsigned char 	sll_halen;
    unsigned char 	sll_addr[8];
};
typedef struct ip_vm
{
	unsigned char ip[30];
	char name[64];
}ip_vm;

void fill_buff_with_ip_of_hostname(char *name);
void join_mcast(char *address_to_join, int port_to_bind);
void send_raw_tour_packet(tour *tour_pkt);
void send_multicast_message(char *msg,int len);
void send_raw_echo_request_message(unsigned char *dst_mac);
void terminate(int sig);
int areq(struct sockaddr *IPaddr, socklen_t sockaddrlen, struct hwaddr *HWaddr);
int Areq(struct sockaddr *IPaddr, socklen_t sockaddrlen, struct hwaddr *HWaddr);
uint32_t get_uint_destination_of_tour(tour *tour_pkt);
void tv_sub(struct timeval* out, struct timeval *in);
void proc_v4(char *ptr, ssize_t len, struct msghdr *msg, struct timeval *tvrecv);
void send_icmp_request(unsigned char *dst_mac);
void sig_alrm(int signo);


long 		     debug = 1;
char		     our_hostname[128];
char		     ip_static_buff[128];
int			     rt;
int		   	     pg;
int 		     request_sock;
int 		     multicast_sock;
int              multicast_recv;
int 		     already_here;
int              first_term =0;
int 		     i;
int 		     pinging;
int 		     stop_pinging;
int 		     we_dont_need_to_send_echo_req;
int 			 count_of_replies;
int 			 final_destination;
int              icmpdata = 56;
struct in_addr 	     dest_of_echo_req;
struct in_addr 	     for_comparisons;
//struct hwa_info  my_saved_eth0;
unsigned char    eth0_mac[6];
int              eth0_if;
ping_targets     prev_sender_list;
struct sigaction act_open;
