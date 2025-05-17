#ifndef DTN_ROUTING_H
#define DTN_ROUTING_H

#include "dtn_module.h"
#include "lwip/ip6_addr.h"
#include <stdbool.h>
#include <time.h>

// Contact opportunity
typedef struct Contact_Info {
    ip6_addr_t node_addr;         // Node address
    ip6_addr_t next_hop;          // Next hop to reach this node
    u32_t start_time_ms;          // Contact start time (ms since boot)
    u32_t end_time_ms;            // Contact end time (ms since boot)
    bool is_dtn_node;             // Whether this is a DTN-capable node
    struct Contact_Info *next;    // Next contact in list
} Contact_Info;

typedef struct Routing_Function {
    DTN_Module* parent_module;
    char* routing_algorithm_name;
    
    Contact_Info* contact_list_head;
} Routing_Function;

Routing_Function* dtn_routing_create(DTN_Module* parent);

void dtn_routing_destroy(Routing_Function* routing);

bool dtn_routing_is_dtn_destination(Routing_Function* routing, const ip6_addr_t* dest_ip);

int dtn_routing_get_dtn_next_hop(Routing_Function* routing, const ip6_addr_t* dest_ip, ip6_addr_t* next_hop_ip);

// Add a contact to the routing function
int dtn_routing_add_contact(Routing_Function* routing, 
                          const ip6_addr_t* node_addr, 
                          const ip6_addr_t* next_hop,
                          u32_t start_time_ms, 
                          u32_t end_time_ms,
                          bool is_dtn_node);

// Remove a contact from the routing function
int dtn_routing_remove_contact(Routing_Function* routing, const ip6_addr_t* node_addr);

// Update contact status based on current time
void dtn_routing_update_contacts(Routing_Function* routing);

// Check if a specific destination has an active contact
bool dtn_routing_has_active_contact(Routing_Function* routing, const ip6_addr_t* dest_ip);

#endif