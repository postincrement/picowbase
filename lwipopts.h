#ifndef LWIPOPTS_H
#define LWIPOPTS_H

// Basic LWIP configuration
#define NO_SYS                      1
#define LWIP_SOCKET                 0
#define LWIP_NETCONN               0
#define LWIP_NETIF_API             0
#define LWIP_NETIF_STATUS_CALLBACK 0
#define LWIP_NETIF_LINK_CALLBACK   0
#define LWIP_NETIF_HWADDRHINT      0
#define LWIP_NETIF_LOOPBACK        0
#define LWIP_HAVE_LOOPIF           0

// Memory configuration
#define MEM_ALIGNMENT              4
#define MEM_SIZE                   (20 * 1024)
#define MEMP_NUM_PBUF             16
#define MEMP_NUM_RAW_PCB          4
#define MEMP_NUM_UDP_PCB          4
#define MEMP_NUM_TCP_PCB          4
#define MEMP_NUM_TCP_PCB_LISTEN   4
#define MEMP_NUM_TCP_SEG          8
#define MEMP_NUM_REASSDATA        2
#define MEMP_NUM_ARP_QUEUE        8
#define MEMP_NUM_IGMP_GROUP       8
#define MEMP_NUM_SYS_TIMEOUT      8

// IP configuration
#define LWIP_IPV4                  1
#define LWIP_IPV6                  0
#define IP_FORWARD                 0
#define IP_OPTIONS_ALLOWED         0
#define IP_REASSEMBLY              1
#define IP_FRAG                    1
#define IP_REASS_MAX_PBUFS        10
#define IP_FRAG_MAX_MTU           1500
#define IP_DEFAULT_TTL            255

// ICMP configuration
#define LWIP_ICMP                  1
#define LWIP_BROADCAST_PING        0
#define LWIP_MULTICAST_PING        0

// DHCP configuration
#define LWIP_DHCP                  1
#define DHCP_DOES_ARP_CHECK        0

// UDP configuration
#define LWIP_UDP                   1
#define LWIP_UDPLITE              0
#define UDP_TTL                   255

// TCP configuration
#define LWIP_TCP                   1
#define TCP_TTL                   255
#define TCP_WND                   (4 * TCP_MSS)
#define TCP_SND_BUF               (2 * TCP_MSS)
#define TCP_SND_QUEUELEN           (4 * (TCP_SND_BUF/TCP_MSS))
#define TCP_SNDLOWAT              (TCP_SND_BUF/2)
#define TCP_LISTEN_BACKLOG        1
#define TCP_DEFAULT_LISTEN_BACKLOG 1

// Debug configuration
#define LWIP_DEBUG                 0
#define LWIP_STATS                 0
#define LWIP_STATS_DISPLAY         0

// Checksum configuration
#define LWIP_CHECKSUM_CTRL_PER_NETIF 1
#define CHECKSUM_GEN_IP           1
#define CHECKSUM_GEN_UDP          1
#define CHECKSUM_GEN_TCP          1
#define CHECKSUM_GEN_ICMP         1
#define CHECKSUM_CHECK_IP         1
#define CHECKSUM_CHECK_UDP        1
#define CHECKSUM_CHECK_TCP        1
#define CHECKSUM_CHECK_ICMP       1

#endif /* LWIPOPTS_H */ 