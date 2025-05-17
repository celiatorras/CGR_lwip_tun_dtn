#ifndef RAW_SOCKET_H
#define RAW_SOCKET_H

#include <netinet/in.h>
#include "lwip/ip6_addr.h"
#include "lwip/pbuf.h"

// Socket handles for the two interfaces
extern int raw_socket_enp0s8;
extern int raw_socket_enp0s9;

int raw_socket_init(const char* if_name_1, const char* if_name_2);

// Send an IPv6 packet through a raw socket on the appropriate interface
// Returns 0 on success, -1 on failure
int raw_socket_send_ipv6(struct pbuf *p, const ip6_addr_t *dest_addr);

void raw_socket_cleanup(void);

#endif