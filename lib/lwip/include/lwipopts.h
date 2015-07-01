#ifndef __LIB_LWIP_LWIPOPTS_H
#define __LIB_LWIP_LWIPOPTS_H

#include <errno.h>
#include <malloc.h>
#include <kernel/thread.h>

#ifdef WITH_TARGET_LWIPOPTS
#include <target/lwipopts.h>
#else

// use lk's libc malloc
#define MEM_LIBC_MALLOC 1

// use mem_malloc() which calls malloc()
// instead of creating static memory pools
#define MEMP_MEM_MALLOC 1

// these don't actually affect anything
// unless MEMP_MEM_MALLOC is 0
#define MEM_SIZE (256 * 1024 * 1024)
#define MEMP_NUM_UDP_PCB 128
#define MEMP_NUM_TCP_PCB 128
#define MEMP_NUM_TCP_PCB_LISTEN 128
#define MEMP_NUM_NETBUF 32
#define MEMP_NUM_NETCONN 32
#define MEMP_NUM_NETDB 32

#define LWIP_COMPAT_SOCKETS 0

#define LWIP_DHCP 1
#define LWIP_AUTOIP 1
#define LWIP_DHCP_AUTOIP_COOP 1

#define LWIP_DNS 1

#define LWIP_NETIF_HOSTNAME 1
#define LWIP_NETIF_API 1
#define LWIP_NETIF_STATUS_CALLBACK 1
#define LWIP_NETIF_HWADDRHINT 1
#define LWIP_NETIF_LOOPBACK 1

#define LWIP_HAVE_LOOPIF 1

#define TCPIP_THREAD_STACKSIZE DEFAULT_STACK_SIZE
#define TCPIP_THREAD_PRIO DEFAULT_PRIORITY

#define TCPIP_MBOX_SIZE 16

#define DEFAULT_THREAD_STACKSIZE DEFAULT_STACK_SIZE

#define DEFAULT_UDP_RECVMBOX_SIZE 16
#define DEFAULT_TCP_RECVMBOX_SIZE 16
#define DEFAULT_ACCEPTMBOX_SIZE 16

#define LWIP_STATS_DISPLAY 0

#endif
#endif

