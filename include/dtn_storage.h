#ifndef DTN_STORAGE_H
#define DTN_STORAGE_H

#include <stddef.h> // For size_t
#include "dtn_module.h" // Include main module struct definition

// Storage function structure
typedef struct Storage_Function {
    DTN_Module* parent_module;
    size_t stored_packets_count;
    size_t max_storage_bytes; // Example configuration
    // Add storage-specific state later (e.g., data structures for stored packets)
} Storage_Function;

// Function to initialize the storage component
Storage_Function* dtn_storage_create(DTN_Module* parent);

// Function to destroy the storage component
void dtn_storage_destroy(Storage_Function* storage);

// Add placeholder API functions for storage logic later, e.g.:
// int dtn_storage_store_packet(Storage_Function* storage, struct pbuf* p);
// struct pbuf* dtn_storage_retrieve_packet(Storage_Function* storage, const struct ip6_addr* next_hop);

#endif // DTN_STORAGE_H