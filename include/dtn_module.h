#ifndef DTN_MODULE_H
#define DTN_MODULE_H

// Forward declarations
struct DTN_Controller;
struct Routing_Function;
struct Storage_Function;

// Main DTN Module structure
typedef struct DTN_Module {
    struct DTN_Controller* controller;
    struct Routing_Function* routing;
    struct Storage_Function* storage;
    // Add overall module state if needed
} DTN_Module;

/**
 * @brief Initializes the overall DTN module and its components.
 * Allocates memory for the main struct and sub-components.
 *
 * @return Pointer to the initialized DTN_Module on success, NULL on failure.
 */
DTN_Module* dtn_module_init(void);

/**
 * @brief Cleans up all resources allocated by the DTN module.
 * Frees sub-components and the main module struct.
 *
 * @param module Pointer to the DTN_Module to clean up.
 */
void dtn_module_cleanup(DTN_Module* module);

#endif // DTN_MODULE_H