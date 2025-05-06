#include "dtn_controller.h"
#include "dtn_routing.h"    // Include routing/storage if controller needs direct access later
#include "dtn_storage.h"
#include "lwip/ip6.h"       // For ip6_hdr, IP6_HLEN
#include "lwip/ip6_addr.h"  // For ip6addr_ntoa_r
#include <stdlib.h>         // malloc, free
#include <stdio.h>          // printf
#include <string.h>         // memcpy

DTN_Controller* dtn_controller_create(DTN_Module* parent) {
    DTN_Controller* controller = (DTN_Controller*)malloc(sizeof(DTN_Controller));
    if (controller) {
        controller->parent_module = parent;
        printf("DTN Controller created.\n");
    } else {
        perror("Failed to allocate memory for DTN_Controller");
    }
    return controller;
}

void dtn_controller_destroy(DTN_Controller* controller) {
    if (!controller) return;
    printf("Destroying DTN Controller...\n");
    // Free any controller-specific resources later
    free(controller);
}

void dtn_controller_process_incoming(DTN_Controller* controller, struct pbuf *p, struct netif *inp_netif) {
     // Basic check: Ensure we have a packet and the controller is valid
    if (!p || !controller) {
        fprintf(stderr, "DTN Controller: Invalid arguments.\n");
        if (p) pbuf_free(p); // Don't leak pbuf
        return;
    }

    // Check if it's an IPv6 packet
    if (p->len < 1) { pbuf_free(p); return; } // Too small check
    u8_t ip_version = ((u8_t *)p->payload)[0] >> 4;
    if (ip_version != 6) { pbuf_free(p); return; } // Not IPv6
    if (p->len < IP6_HLEN) { pbuf_free(p); return; } // Runt check

    const struct ip6_hdr *ip6hdr = (const struct ip6_hdr *)p->payload;
    ip6_addr_t temp_src_addr;
    ip6_addr_t temp_dest_addr;
    memcpy(&temp_src_addr, &ip6hdr->src, sizeof(ip6_addr_t));
    memcpy(&temp_dest_addr, &ip6hdr->dest, sizeof(ip6_addr_t));

    char src_addr_str[IP6ADDR_STRLEN_MAX];
    char dst_addr_str[IP6ADDR_STRLEN_MAX];
    ip6addr_ntoa_r(&temp_src_addr, src_addr_str, sizeof(src_addr_str));
    ip6addr_ntoa_r(&temp_dest_addr, dst_addr_str, sizeof(dst_addr_str));

    printf("DTN Controller: Intercepted packet [%s] -> [%s] (Proto: %d, Len: %d)\n",
           src_addr_str, dst_addr_str, IP6H_NEXTH(ip6hdr), p->tot_len);

    // --- Placeholder ---
    // For now, pass ALL packets to LwIP's input
    printf("DTN Controller: Passing packet to lwIP ip6_input.\n");
    err_t err = ip6_input(p, inp_netif); // ip6_input takes ownership of pbuf 'p'
    if (err != ERR_OK) {
        fprintf(stderr, "DTN Controller: ip6_input returned error %d\n", err);
        // Note: pbuf 'p' is likely freed by ip6_input even on error in most cases.
    }
}