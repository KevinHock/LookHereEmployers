#ifndef __ARP_H__
#define __ARP_H__

typedef struct arp_req_msg_
{
	unsigned char dst_ip[4];
	int           sll_ifindex;
	unsigned short 	sll_hatype;
	unsigned char sll_halen;
	unsigned char sll_addr[8];
}arp_msg;

struct hwaddr 
{
    int 			sll_ifindex;
    unsigned short 	sll_hatype;
    unsigned char 	sll_halen;
    unsigned char 	sll_addr[8];
};

#endif