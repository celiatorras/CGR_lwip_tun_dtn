#ifndef DTN_ROUTING_H
#define DTN_ROUTING_H

#include "dtn_module.h" // Include main module struct definition

// Routing function structure
typedef struct Routing_Function {
    DTN_Module* parent_module;
    char* routing_algorithm_name;
    // Add routing-specific state later (e.g., contact graph)
} Routing_Function;

// Function to initialize the routing component
Routing_Function* dtn_routing_create(DTN_Module* parent);

// Function to destroy the routing component
void dtn_routing_destroy(Routing_Function* routing);

// Add placeholder API functions for routing logic later, e.g.:
// struct ip6_addr* dtn_routing_get_next_hop(Routing_Function* routing, const struct ip6_addr* dest);

#endif // DTN_ROUTING_H