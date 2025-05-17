#include "dtn_controller.h"
#include "dtn_routing.h"
#include "dtn_storage.h"
#include "dtn_icmpv6.h"
#include "lwip/ip6.h"
#include "lwip/ip6_addr.h"
#include "lwip/pbuf.h"
#include "lwip/err.h"
#include "lwip/netif.h"
#include "lwip/icmp6.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "raw_socket.h"
#include "lwip/sys.h" 

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
    free(controller);
}

int dtn_controller_process_icmpv6(DTN_Controller* controller, struct pbuf *p, struct netif *inp_netif) {
    if (!p || !controller || !controller->parent_module) {
        return 0;
    }
    
    // Check if this is a DTN ICMPv6 message
    return dtn_icmpv6_process(p, inp_netif);
}

void dtn_controller_process_incoming(DTN_Controller* controller, struct pbuf *p, struct netif *inp_netif) {
    if (!p || !controller || !controller->parent_module || 
        !controller->parent_module->routing || !controller->parent_module->storage) {
        fprintf(stderr, "DTN Controller: Invalid arguments or uninitialized components for incoming.\n");
        if (p) pbuf_free(p);
        return;
    }
    
    if (p->len < IP6_HLEN) {
        fprintf(stderr, "DTN Controller: Packet too small for IPv6 header.\n");
        pbuf_free(p);
        return;
    }
    
    const struct ip6_hdr *ip6hdr = (const struct ip6_hdr *)p->payload;
    if (IP6H_V(ip6hdr) != 6) {
        fprintf(stderr, "DTN Controller: Packet is not IPv6 (version %d).\n", IP6H_V(ip6hdr));
        pbuf_free(p);
        return;
    }
    
    ip6_addr_t temp_src_addr, temp_dest_addr;
    memcpy(&temp_src_addr, &ip6hdr->src, sizeof(ip6_addr_t));
    memcpy(&temp_dest_addr, &ip6hdr->dest, sizeof(ip6_addr_t));
    
    char src_addr_str[IP6ADDR_STRLEN_MAX] = {0};
    char dst_addr_str[IP6ADDR_STRLEN_MAX] = {0};
    
    Routing_Function* routing = controller->parent_module->routing;
    Storage_Function* storage = controller->parent_module->storage;
    
    // Check if this is ICMPv6 and process it if it's a DTN ICMPv6 message
    if (IP6H_NEXTH(ip6hdr) == IP6_NEXTH_ICMP6) {
        struct pbuf *q = pbuf_alloc(PBUF_RAW, p->tot_len, PBUF_RAM);
        if (!q) {
            fprintf(stderr, "DTN Controller: Failed to allocate pbuf for ICMPv6 processing.\n");
            pbuf_free(p);
            return;
        }
        
        if (pbuf_copy(q, p) != ERR_OK) {
            fprintf(stderr, "DTN Controller: Failed to copy pbuf for ICMPv6 processing.\n");
            pbuf_free(q);
            pbuf_free(p);
            return;
        }
        
        // Skip IPv6 header to get to ICMPv6 header
        if (pbuf_header(q, -IP6_HLEN) != 0) {
            fprintf(stderr, "DTN Controller: Failed to adjust pbuf header for ICMPv6 processing.\n");
            pbuf_free(q);
            pbuf_free(p);
            return;
        }
        
        // Process the ICMPv6 message
        if (dtn_controller_process_icmpv6(controller, q, inp_netif)) {
            pbuf_free(q);
            pbuf_free(p);
            return;
        }
        
        // Not a DTN ICMPv6 message, continue normal processing
        pbuf_free(q);
    }
    
    // Get destination address in string format for logging
    ip6addr_ntoa_r(&temp_src_addr, src_addr_str, sizeof(src_addr_str));
    ip6addr_ntoa_r(&temp_dest_addr, dst_addr_str, sizeof(dst_addr_str));

    // Check if it's for this LwIP stack
    bool is_for_this_lwip_stack = false;
    ip6_addr_t local_lwip_addr;
    if (ip6addr_aton("fd00::2", &local_lwip_addr)) {
        ip6_addr_t dest_addr_nozone = temp_dest_addr;
        #if LWIP_IPV6_SCOPES
        ip6_addr_set_zone(&dest_addr_nozone, IP6_NO_ZONE);
        ip6_addr_set_zone(&local_lwip_addr, IP6_NO_ZONE);
        #endif
        if (ip6_addr_cmp(&dest_addr_nozone, &local_lwip_addr)) {
            is_for_this_lwip_stack = true;
        }
    }

    if (is_for_this_lwip_stack) {
        err_t err = ip6_input(p, inp_netif);
        if (err != ERR_OK) {
            fprintf(stderr, "DTN Controller: ip6_input returned error %d for local stack packet.\n", err);
        }
        return;
    }

    // Nnot for the local stack
    bool is_dtn_dest = dtn_routing_is_dtn_destination(routing, &temp_dest_addr);
    
    if (is_dtn_dest) {
        printf("DTN Controller: Destination %s. Checking contact...\n", dst_addr_str);
        ip6_addr_t next_hop_ip;
        int contact_available = dtn_routing_get_dtn_next_hop(routing, &temp_dest_addr, &next_hop_ip);
        
        if (contact_available) {
            printf("DTN Controller: Contact OPEN for Node %s. Forwarding via raw socket.\n", dst_addr_str);
            
            // Create a copy of the packet for DTN-PCK-FORWARDED message
            struct pbuf *p_copy = pbuf_alloc(PBUF_RAW, p->tot_len, PBUF_RAM);
            if (p_copy != NULL) {
                if (pbuf_copy(p_copy, p) == ERR_OK) {
                    // Send DTN-PCK-FORWARDED message
                    dtn_icmpv6_send_pck_forwarded(inp_netif, p_copy, ICMP6_CODE_DTN_NO_INFO);
                }
                pbuf_free(p_copy);
            }
            
            err_t err = raw_socket_send_ipv6(p, &temp_dest_addr) == 0 ? ERR_OK : ERR_IF;
            if (err == ERR_OK) {
                printf("DTN Controller: Packet for %s successfully sent via raw socket.\n", dst_addr_str);
            } else {
                fprintf(stderr, "DTN Controller: Error sending packet for %s via raw socket: %d.\n", dst_addr_str, err);
            }
            pbuf_free(p);
            return;
        } else {
            printf("DTN Controller: Contact CLOSED for Node %s. Attempting to store.\n", dst_addr_str);
            if (dtn_storage_store_packet(storage, p, &temp_dest_addr)) {
                printf("DTN Controller: Packet for %s stored.\n", dst_addr_str);
                
                // Create a copy of the packet for DTN-PCK-RECEIVED message
                struct pbuf *p_copy = pbuf_alloc(PBUF_RAW, p->tot_len, PBUF_RAM);
                if (p_copy != NULL) {
                    if (pbuf_copy(p_copy, p) == ERR_OK) {
                        // Send DTN-PCK-RECEIVED message
                        dtn_icmpv6_send_pck_received(inp_netif, p_copy, ICMP6_CODE_DTN_NO_CONTACT);
                    }
                    pbuf_free(p_copy);
                }
                return;
            } else {
                fprintf(stderr, "DTN Controller: Failed to store packet for %s (e.g., storage full). Freeing.\n", dst_addr_str);
                
                // Create a copy of the packet for DTN-PCK-DELETED message
                struct pbuf *p_copy = pbuf_alloc(PBUF_RAW, p->tot_len, PBUF_RAM);
                if (p_copy != NULL) {
                    if (pbuf_copy(p_copy, p) == ERR_OK) {
                        // Send DTN-PCK-DELETED message
                        dtn_icmpv6_send_pck_deleted(inp_netif, p_copy, ICMP6_CODE_DTN_DEPLETED_STORE, 0);
                    }
                    pbuf_free(p_copy);
                }
                
                pbuf_free(p);
                return;
            }
        }
    } else {
        err_t err = raw_socket_send_ipv6(p, &temp_dest_addr) == 0 ? ERR_OK : ERR_IF;
        if (err == ERR_OK) {
        } else {
            fprintf(stderr, "DTN Controller: Error sending packet for %s via raw socket: %d.\n", dst_addr_str, err);
        }
        pbuf_free(p);
        return;
    }
}

void dtn_controller_attempt_forward_stored(DTN_Controller* controller, struct netif *netif_out) {
    if (!controller || !controller->parent_module || !controller->parent_module->storage || 
        !controller->parent_module->routing || !netif_out) {
        return;
    }
    
    Storage_Function* storage = controller->parent_module->storage;
    Routing_Function* routing = controller->parent_module->routing;
    
    // Update routing contacts based on current time
    dtn_routing_update_contacts(routing);
    
    // Process all contacts with available storage
    Contact_Info* contact = routing->contact_list_head;
    u32_t current_time = sys_now();
    
    while (contact != NULL) {
        // If contact is active
        if (contact->is_dtn_node && 
            current_time >= contact->start_time_ms && 
            current_time <= contact->end_time_ms) {
                
            char node_addr_str[IP6ADDR_STRLEN_MAX];
            ip6addr_ntoa_r(&contact->node_addr, node_addr_str, sizeof(node_addr_str));
            
            // Attempt to forward any packets for this destination
            Stored_Packet_Entry* retrieved_entry = 
                dtn_storage_retrieve_packet_for_dest(storage, &contact->node_addr);
                
            if (retrieved_entry && retrieved_entry->p) {
                printf("DTN Controller: Contact available for %s. Processing retrieved packet.\n", 
                       node_addr_str);
                       
                char retrieved_dest_str[IP6ADDR_STRLEN_MAX];
                ip6addr_ntoa_r(&retrieved_entry->original_dest, retrieved_dest_str, sizeof(retrieved_dest_str));
                struct pbuf *p_to_fwd = retrieved_entry->p;
                
                bool is_for_this_lwip_stack = false;
                ip6_addr_t local_lwip_addr;
                if (ip6addr_aton("fd00::2", &local_lwip_addr)) {
                    ip6_addr_t retrieved_dest_nozone;
                    memcpy(&retrieved_dest_nozone, &retrieved_entry->original_dest, sizeof(ip6_addr_t));
                    #if LWIP_IPV6_SCOPES
                    ip6_addr_set_zone(&retrieved_dest_nozone, IP6_NO_ZONE);
                    ip6_addr_set_zone(&local_lwip_addr, IP6_NO_ZONE);
                    #endif
                    if (ip6_addr_cmp(&retrieved_dest_nozone, &local_lwip_addr)) {
                        is_for_this_lwip_stack = true;
                    }
                }
                
                if (is_for_this_lwip_stack) {
                    // Create a copy of the packet for DTN-PCK-DELIVERED message
                    struct pbuf *p_copy = pbuf_alloc(PBUF_RAW, p_to_fwd->tot_len, PBUF_RAM);
                    if (p_copy != NULL) {
                        if (pbuf_copy(p_copy, p_to_fwd) == ERR_OK) {
                            // Send DTN-PCK-DELIVERED message
                            dtn_icmpv6_send_pck_delivered(netif_out, p_copy, ICMP6_CODE_DTN_NO_INFO);
                        }
                        pbuf_free(p_copy);
                    }
                    
                    err_t err = ip6_input(p_to_fwd, netif_out);
                    if (err == ERR_OK) {
                    } else {
                        pbuf_free(p_to_fwd);
                    }
                } else {
                    printf("DTN Controller: Retrieved packet for remote Node %s. Forwarding via raw socket.\n",
                           retrieved_dest_str);
                    
                    // Create a copy of the packet for DTN-PCK-FORWARDED message
                    struct pbuf *p_copy = pbuf_alloc(PBUF_RAW, p_to_fwd->tot_len, PBUF_RAM);
                    if (p_copy != NULL) {
                        if (pbuf_copy(p_copy, p_to_fwd) == ERR_OK) {
                            // Send DTN-PCK-FORWARDED message
                            dtn_icmpv6_send_pck_forwarded(netif_out, p_copy, ICMP6_CODE_DTN_NO_INFO);
                        }
                        pbuf_free(p_copy);
                    }
                    
                    err_t err = raw_socket_send_ipv6(p_to_fwd, &contact->node_addr) == 0 ? ERR_OK : ERR_IF;
                    if (err == ERR_OK) {
                        printf("DTN Controller: Packet for %s successfully sent via raw socket.\n", retrieved_dest_str);
                    } else {
                        fprintf(stderr, "DTN Controller: Error sending stored packet for %s via raw socket: %d.\n",
                                retrieved_dest_str, err);
                    }
                    pbuf_free(p_to_fwd);
                }
                dtn_storage_free_retrieved_entry_struct(retrieved_entry);
            }
        }
        contact = contact->next;
    }
}