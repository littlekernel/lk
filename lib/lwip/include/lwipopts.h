#ifndef __LWIPOPTS_H
#define __LWIPOPTS_H

#include <kernel/thread.h>

#define LWIP_DHCP 1

#define MEM_ALIGNMENT 4

#define LWIP_PROVIDE_ERRNO 1

/* heap */
#define MEM_SIZE 8192
#define MEMP_NUM_PBUF 64

/* dhcp uses 2 extra ones that push the default 3 over the limit */
#define MEMP_NUM_SYS_TIMEOUT 5

/* some tcp tweakage */
#define TCP_MSS 1024
#define TCP_SND_BUF 2048

/* tcp worker thread priorities */
#define DEFAULT_THREAD_PRIO DEFAULT_PRIORITY
#define TCPIP_THREAD_PRIO DEFAULT_PRIORITY

#endif

