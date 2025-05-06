#include "dtn_routing.h"
#include <stdlib.h> // malloc, free
#include <stdio.h>  // printf

Routing_Function* dtn_routing_create(DTN_Module* parent) {
    Routing_Function* routing = (Routing_Function*)malloc(sizeof(Routing_Function));
    if (routing) {
        routing->parent_module = parent;
        routing->routing_algorithm_name = "Placeholder (e.g., CGR)";
        printf("DTN Routing Function created (Algorithm: %s).\n", routing->routing_algorithm_name);
    } else {
        perror("Failed to allocate memory for Routing_Function");
    }
    return routing;
}

void dtn_routing_destroy(Routing_Function* routing) {
    if (!routing) return;
    printf("Destroying DTN Routing Function...\n");
    // Free any routing-specific resources later
    free(routing);
}