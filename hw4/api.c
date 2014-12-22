//#include "tour.h"
#include "arp.h"
#include "unp.h"

#define  WELLKNOWN_PATH   "/tmp/some0th3rd1r"


int areq(struct sockaddr *IPaddr, socklen_t sockaddrlen, struct hwaddr *HWaddr){
	int 				unix_sock;
	struct sockaddr_un 	unixaddr;
	arp_msg 		    msg;
	//char 				IPstr[INET_ADDRSTRLEN];
	fd_set 				rset;
	struct 				timeval timeout;

	unix_sock = Socket(AF_UNIX, SOCK_STREAM, 0);
	
	bzero(&unixaddr, sizeof(unixaddr));
	unixaddr.sun_family = AF_UNIX;
	strcpy(unixaddr.sun_path, WELLKNOWN_PATH);
	Connect(unix_sock, (struct sockaddr*)&unixaddr, SUN_LEN(&unixaddr));

	// ghiTODO host/network byte order ghiTODO
	//inet_ntop(AF_INET, &(IPaddr->sa_data), IPstr, INET_ADDRSTRLEN);
	//DEBUG("DID THIS WORK?");
	// IPaddr->sa_data may not contain the sin_addr and so we can memcpy it to a sockaddr_in if needed
	//printf("areq() has been called for IP %s\n", IPstr);
	//DEBUG("DID THIS WORK?");
	//snprintf(msg, sizeof(msg), "%s  %d  %u  %hu", IPstr, HWaddr->sll_ifindex, HWaddr->sll_halen, HWaddr->sll_hatype);
	struct sockaddr_in *sockaddr = (struct sockaddr_in*)IPaddr;
	unsigned char *p = (unsigned char*)&(sockaddr->sin_addr.s_addr);
	//printf("%d.%d.%d.%d\n", p[0],p[1],p[2],p[3]);
	msg.dst_ip[0] = p[0];
	msg.dst_ip[1] = p[1];
	msg.dst_ip[2] = p[2];
	msg.dst_ip[3] = p[3];
	//printf("sin_addr %d\n", sockaddr->sin_addr.s_addr);
	//printf("sock ip %s", Sock_ntop_host(IPaddr, sizeof(*IPaddr)));
	//sscanf(Sock_ntop_host(IPaddr, sizeof(*IPaddr)), "%d.%d.%d.%d",&msg.dst_ip[0],&msg.dst_ip[1],
								//&msg.dst_ip[2],&msg.dst_ip[3]);
	//printf("areq: request hardware add for %d.%d.%d.%d\n", msg.dst_ip[0],msg.dst_ip[1],msg.dst_ip[2],
		//msg.dst_ip[3]);
	Send(unix_sock, &msg, sizeof(msg),0);

	FD_ZERO(&rset);
	timeout.tv_sec  = 1000;
	timeout.tv_usec = 0;
	FD_SET(unix_sock, &rset);
	Select(unix_sock+1, &rset, NULL, NULL, &timeout);

	if(FD_ISSET(unix_sock, &rset)){
		if(0 == recv(unix_sock, &msg, sizeof(msg), 0))
		{
			//printf("opposite node close connection\n");
			close(unix_sock);
			return -1;
		}
		memcpy(HWaddr->sll_addr, (msg.sll_addr), sizeof(HWaddr->sll_addr));

		//printf("The HW address returned is ");
		//for(i=0; i<sizeof(msg); i++){
		//	printf("%x", msg[i]);
		//	if(i != sizeof(msg)-1)
		//		printf(":");
		//}
		//printf("%.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n",HWaddr->sll_addr[0],HWaddr->sll_addr[1],HWaddr->sll_addr[2],
		//		HWaddr->sll_addr[3],HWaddr->sll_addr[4],HWaddr->sll_addr[5]);
		//printf("\n");

		close(unix_sock);
		return 0;
	}else{
		//printf("Failure, Select() timed out\n");
		close(unix_sock);
		return -1;
	}
}

int Areq(struct sockaddr *IPaddr, socklen_t sockaddrlen, struct hwaddr *HWaddr){
	int return_value;
	if((return_value = areq(IPaddr, sockaddrlen, HWaddr)) < 0){
		//printf("areq api return failed");
		exit(0);
	}
	return return_value;
}

#if 0
int main(int argc, char **argv)
{
	char *ip="130.245.156.22";
	struct sockaddr_in sockaddr;
	struct hwaddr HWaddr;
	socklen_t socklen = sizeof(struct sockaddr_in);
	inet_pton(AF_INET, ip, &sockaddr.sin_addr);
	//printf("%s\n", inet_ntoa(sockaddr.sin_addr));
	Areq((SA*)&sockaddr, socklen, &HWaddr);
	printf("%.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n",HWaddr.sll_addr[0],HWaddr.sll_addr[1],HWaddr.sll_addr[2],
				HWaddr.sll_addr[3],HWaddr.sll_addr[4],HWaddr.sll_addr[5]);
	while(1);

}
#endif

