#include "unp.h"
#include <setjmp.h>

#define WELLKNOWN_PATH "/tmp/sum0therd1r"
#define ODR_SERVICE_PATH "/tmp/yoyoyoDomainDatagramSocket"
#define FROMCLI 1
#define FROMSERV 0

void msg_send(int sockfd, char *canonical_dest_ip, int dest_port, char *sent_msg, int force_rediscovery_flag);
void msg_recv(int sockfd, char *canonical_src_ip, int *src_port, char *recieved_msg, int from_cli);

// Signals

struct sigaction act_open;
sigset_t signals_open;
// Signals