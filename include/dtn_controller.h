#ifndef DTN_CONTROLLER_H
#define DTN_CONTROLLER_H

#include "lwip/pbuf.h"   // struct pbuf
#include "lwip/netif.h"  // struct netif
#include "dtn_module.h" // Include main module struct definition

// Forward declarations if needed for internal pointers
struct Routing_Function;
struct Storage_Function;

// Controller structure
typedef struct DTN_Controller {
    DTN_Module* parent_module; // Pointer back to the main module
    // Add controller-specific state later
} DTN_Controller;

// Function to initialize the controller component
DTN_Controller* dtn_controller_create(DTN_Module* parent);

// Function to destroy the controller component
void dtn_controller_destroy(DTN_Controller* controller);

/**
 * @brief Entry point for processing incoming packets via the DTN Controller.
 * This function intercepts packets before they hit the standard lwIP input.
 * It takes ownership of the pbuf 'p' and must free it if not passed on.
 *
 * @param controller Pointer to the initialized controller instance.
 * @param p The received packet buffer.
 * @param inp_netif The network interface the packet was received on.
 */
void dtn_controller_process_incoming(DTN_Controller* controller, struct pbuf *p, struct netif *inp_netif);

#endif // DTN_CONTROLLER_H