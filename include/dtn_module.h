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
} DTN_Module;

DTN_Module* dtn_module_init(void);

//Cleans up all resources allocated by the DTN module.
void dtn_module_cleanup(DTN_Module* module);

#endif