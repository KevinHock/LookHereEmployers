#include "unp.h"
#include <setjmp.h>

#define WELLKNOWN_PATH "/tmp/sum0therd1r"
#define ODR_SERVICE_PATH "/tmp/yoyoyoDomainDatagramSocket"
#define FROMCLI 1
#define FROMSERV 0



#define SERVER_PORT 31330
void msg_send(int sockfd, char *canonical_dest_ip, int dest_port, char *sent_msg, int force_rediscovery_flag,int from_cli);
void msg_recv(int sockfd, char *canonical_src_ip, int *src_port, char *recieved_msg, int from_cli);

// Signals

typedef struct datamsg
{
	char data[MAXLINE];
	char canonical_ip[30];
	int  port;
	int  force_flg;
}datamsg;

struct sigaction act_open;
sigset_t signals_open;
// Signals