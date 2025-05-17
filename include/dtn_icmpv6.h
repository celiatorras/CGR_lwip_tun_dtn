#ifndef DTN_ICMPV6_H
#define DTN_ICMPV6_H

#include "lwip/pbuf.h"
#include "lwip/ip6_addr.h"
#include "lwip/icmp6.h"
#include "lwip/netif.h"
#include "dtn_module.h"

// DTN ICMPv6 message types
#define ICMP6_TYPE_DTN_PCK_RECEIVED    200
#define ICMP6_TYPE_DTN_PCK_FORWARDED   201
#define ICMP6_TYPE_DTN_PCK_DELIVERED   202
#define ICMP6_TYPE_DTN_PCK_DELETED     203

// DTN ICMPv6 message codes
#define ICMP6_CODE_DTN_NO_INFO         0
#define ICMP6_CODE_DTN_LIFETIME_EXP    1
#define ICMP6_CODE_DTN_UNI_LINK        2
#define ICMP6_CODE_DTN_TX_CANCELLED    3
#define ICMP6_CODE_DTN_DEPLETED_STORE  4
#define ICMP6_CODE_DTN_NO_DEST         5
#define ICMP6_CODE_DTN_NO_ROUTE        6
#define ICMP6_CODE_DTN_NO_CONTACT      7
#define ICMP6_CODE_DTN_HOP_LIMIT       9
#define ICMP6_CODE_DTN_TRAFFIC_PARED   10

void dtn_icmpv6_send_pck_received(struct netif *netif, struct pbuf *p, u8_t code);
void dtn_icmpv6_send_pck_forwarded(struct netif *netif, struct pbuf *p, u8_t code);
void dtn_icmpv6_send_pck_delivered(struct netif *netif, struct pbuf *p, u8_t code);
void dtn_icmpv6_send_pck_deleted(struct netif *netif, struct pbuf *p, u8_t code, u8_t reason);

u8_t dtn_icmpv6_process(struct pbuf *p, struct netif *inp_netif);

#endif