#include "dtn_storage.h"
#include <stdlib.h> // malloc, free
#include <stdio.h>  // printf

Storage_Function* dtn_storage_create(DTN_Module* parent) {
    Storage_Function* storage = (Storage_Function*)malloc(sizeof(Storage_Function));
    if (storage) {
        storage->parent_module = parent;
        storage->stored_packets_count = 0;
        storage->max_storage_bytes = 1024 * 1024; // Example: 1MB limit
        printf("DTN Storage Function created (Max: %zu bytes).\n", storage->max_storage_bytes);
    } else {
        perror("Failed to allocate memory for Storage_Function");
    }
    return storage;
}

void dtn_storage_destroy(Storage_Function* storage) {
    if (!storage) return;
    printf("Destroying DTN Storage Function...\n");
    // Free any storage-specific resources later (e.g., clear packet queue)
    free(storage);
}