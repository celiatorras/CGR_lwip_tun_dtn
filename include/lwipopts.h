#ifndef LWIPOPTS_H
#define LWIPOPTS_H

// Core System Configuration
#define NO_SYS 1
#define LWIP_TIMERS 1
#define LWIP_TIMEVAL_PRIVATE 0
#define SYS_LIGHTWEIGHT_PROT 1

// Protocol Support Configuration
#define LWIP_IPV4 0                      // Disable IPv4
#define LWIP_ARP 0                       // No ARP needed
#define LWIP_ETHERNET 0                  // No Ethernet
#define LWIP_TCP 0                       // Disable TCP for now
#define LWIP_UDP 0                       // Disable UDP for now
#define LWIP_ICMP 0                      // No ICMP for IPv4

// IPv6 Configuration
#define LWIP_IPV6 1
#define LWIP_ICMP6 1                     // Needed for DTN signaling
#define LWIP_IPV6_REASS 0
#define LWIP_ND6_QUEUEING 0
#define LWIP_NETIF_IPV6_ADDR_GEN_AUTO 0
#define LWIP_IPV6_NUM_ADDRESSES 3 // Or more

// Memory Configuration
#define MEM_SIZE (16 * 1024)                     
#define MEMP_NUM_PBUF 10                 
#define PBUF_POOL_SIZE 10                 
#define PBUF_POOL_BUFSIZE 1536
#define MEM_LIBC_MALLOC                  0
#define MEM_ALIGNMENT                    4

// Network Interface Callbacks
#define LWIP_NETIF_LINK_CALLBACK 1
#define LWIP_NETIF_STATUS_CALLBACK 1
#define LWIP_TCPIP_CORE_LOCKING 0

// API Support
#define LWIP_SOCKET 0
#define LWIP_NETCONN 0

#endif