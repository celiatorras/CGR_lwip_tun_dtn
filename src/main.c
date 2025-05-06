/**
 * ============================================================================
 * File: main.c
 * Description: Main application integrating LwIP/TUN with modular DTN components.
 * Uses address assignment logic from original code attempt.
 * REQUIRES: LWIP_NETIF_IPV6_ADDR_GEN_AUTO 0 in lwipopts.h
 * ============================================================================
 */

// Standard/system headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/time.h>

// LwIP headers
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/pbuf.h"
#include "lwip/timeouts.h"
#include "lwip/ip6.h"
#include "lwip/ip6_addr.h"
#include "lwip/ip.h"        // May be needed for ip6_input definition with certain configs

// DTN Module headers
#include "dtn_module.h"
#include "dtn_controller.h" // For dtn_controller_process_incoming

// Constants
#define TUN_IFNAME "tun0"
#define PACKET_BUF_SIZE 2048

// --- Global DTN Module Pointer ---
// We need access to the controller from tunif_input
static DTN_Module* global_dtn_module = NULL;
// -------------------------------

/**
 * Allocate and configure a TUN interface.
 * (Code from previous working version)
 */
int tun_alloc(char *dev_name, int max_len) {
    struct ifreq ifr;
    int fd = open("/dev/net/tun", O_RDWR);
    if (fd < 0) { perror("Opening /dev/net/tun"); return fd; }
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
    if (dev_name && *dev_name) { strncpy(ifr.ifr_name, dev_name, IFNAMSIZ); ifr.ifr_name[IFNAMSIZ - 1] = '\0'; }
    if (ioctl(fd, TUNSETIFF, (void *)&ifr) < 0) { perror("ioctl(TUNSETIFF)"); close(fd); return -1; }
    strncpy(dev_name, ifr.ifr_name, max_len); dev_name[max_len - 1] = '\0';
    return fd;
}

/**
 * Send data from lwIP (pbuf) to the TUN device (file descriptor).
 * (Code from previous working version)
 */
err_t tunif_output(struct netif *netif, struct pbuf *p) {
    if (!netif || !netif->state || !p) { return ERR_ARG; }
    int tun_fd = *(int *)netif->state;
    char buffer[PACKET_BUF_SIZE];
    if (p->tot_len > sizeof(buffer)) { fprintf(stderr, "Packet too large for output buffer\n"); return ERR_MEM; }
    if (pbuf_copy_partial(p, buffer, p->tot_len, 0) != p->tot_len) { return ERR_BUF; }
    //printf("LwIP sending packet of %d bytes via TUN\n", p->tot_len); // Optional log
    ssize_t written = write(tun_fd, buffer, p->tot_len);
    if (written < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) { return ERR_WOULDBLOCK; }
        perror("TUN write failed"); return ERR_IF;
    }
    if ((size_t)written != p->tot_len) { fprintf(stderr, "TUN write short: wrote %zd vs %d\n", written, p->tot_len); return ERR_IF; }
    return ERR_OK;
}

/**
 * Read data from the TUN device (file descriptor) and feed it into the DTN Controller.
 * MODIFIED to call DTN Controller.
 */
err_t tunif_input(struct netif *netif) {
     if (!netif || !netif->state) { return ERR_ARG; }
     // Ensure the global module (and thus controller) is initialized
     if (!global_dtn_module || !global_dtn_module->controller) {
         fprintf(stderr, "tunif_input: DTN Module not initialized!\n");
         // Read and discard to prevent busy-loop? Requires care.
         char discard_buf[100];
         read(*(int *)netif->state, discard_buf, sizeof(discard_buf));
         return ERR_IF; // Indicate an issue
     }

     int tun_fd = *(int *)netif->state;
     char buf[PACKET_BUF_SIZE];
     ssize_t len = read(tun_fd, buf, sizeof(buf));

     if (len < 0) { if (errno == EAGAIN || errno == EWOULDBLOCK) { return ERR_OK; } perror("TUN read error"); return ERR_IF; }
     if (len == 0) { printf("TUN read 0 bytes, tunnel closed?\n"); return ERR_CONN; }

     struct pbuf *p = pbuf_alloc(PBUF_IP, len, PBUF_POOL);
     if (!p) { fprintf(stderr, "Failed to allocate pbuf for incoming packet of size %zd\n", len); return ERR_MEM; }

     err_t copy_err = pbuf_take(p, buf, len);
     if (copy_err != ERR_OK) { fprintf(stderr, "Failed to copy buffer to pbuf (%d)\n", copy_err); pbuf_free(p); return copy_err; }

     // --- MODIFIED: Call DTN Controller ---
     // Pass ownership of pbuf 'p' to the controller function
     dtn_controller_process_incoming(global_dtn_module->controller, p, netif);
     // --- END MODIFICATION ---

     return ERR_OK;
}


/**
 * IPv6 output wrapper function for the netif structure.
 * (Code from previous working version)
 */
err_t tunif_ip6_output(struct netif *netif, struct pbuf *p, const ip6_addr_t *ipaddr) {
    LWIP_UNUSED_ARG(ipaddr);
    return netif->linkoutput(netif, p);
}


/**
 * Initialize the TUN network interface structure within lwIP.
 * (Code from previous working version)
 */
err_t tunif_init(struct netif *netif) {
    if (!netif) { return ERR_ARG; }
    netif->name[0] = 't'; netif->name[1] = 'n';
    netif->output_ip6 = tunif_ip6_output;
    netif->linkoutput = tunif_output;
    netif->mtu = 1500;
    netif->flags = NETIF_FLAG_LINK_UP | NETIF_FLAG_UP;
    return ERR_OK;
}


/**
 * Main entry point.
 * Initializes LwIP, DTN Module, TUN interface, and runs main loop.
 * Uses address assignment logic from the first user code example.
 * REQUIRES LWIP_NETIF_IPV6_ADDR_GEN_AUTO 0 in lwipopts.h
 */
int main() {
    lwip_init();

    // Initialize the DTN Module
    global_dtn_module = dtn_module_init();
    if (!global_dtn_module) {
         fprintf(stderr, "Failed to initialize DTN Module\n");
         exit(EXIT_FAILURE);
    }

    struct netif tun_netif;
    memset(&tun_netif, 0, sizeof(tun_netif));

    // Allocate and configure the TUN interface
    char tun_name[IFNAMSIZ];
    strncpy(tun_name, TUN_IFNAME, IFNAMSIZ -1);
    tun_name[IFNAMSIZ - 1] = '\0';

    int tun_fd = tun_alloc(tun_name, sizeof(tun_name));
    if (tun_fd < 0) {
        fprintf(stderr, "TUN device allocation failed\n");
        dtn_module_cleanup(global_dtn_module);
        exit(EXIT_FAILURE);
    }
    printf("TUN device '%s' created successfully (fd: %d).\n", tun_name, tun_fd);


    // Make TUN interface non-blocking
    int flags = fcntl(tun_fd, F_GETFL, 0);
    if (flags == -1) { perror("fcntl F_GETFL"); close(tun_fd); dtn_module_cleanup(global_dtn_module); exit(EXIT_FAILURE); }
    if (fcntl(tun_fd, F_SETFL, flags | O_NONBLOCK) == -1) { perror("fcntl F_SETFL O_NONBLOCK"); close(tun_fd); dtn_module_cleanup(global_dtn_module); exit(EXIT_FAILURE); }


    // Add interface to LwIP - Use corrected signature
    if (!netif_add(&tun_netif, &tun_fd, tunif_init, ip6_input)) {
         fprintf(stderr, "Failed to add netif to lwIP\n");
         close(tun_fd);
         dtn_module_cleanup(global_dtn_module);
         exit(EXIT_FAILURE);
    }

    netif_set_default(&tun_netif);

    // --- Using address assignment from first example ---
    // 1. Set interface UP first
    netif_set_up(&tun_netif);
    printf("Interface set UP.\n");

    // 2. Add Static Address, passing NULL for index pointer, NO error check
    ip6_addr_t ip6addr_static;
    if (!ip6addr_aton("fd00::2", &ip6addr_static)) {
        fprintf(stderr, "Failed to parse static IPv6 address\n");
        netif_remove(&tun_netif); close(tun_fd); dtn_module_cleanup(global_dtn_module);
        exit(EXIT_FAILURE);
    }
    netif_add_ip6_address(&tun_netif, &ip6addr_static, NULL); // No return check, as per original implicit behavior
    printf("Static address fd00::2 add attempted.\n");


    // 3. Explicitly create Link-Local Address
    netif_create_ip6_linklocal_address(&tun_netif, 1); // 1 = TENTATIVE state
    printf("Link-local address creation requested.\n");

    // 4. Set state of the *first* address (index 0, presumed static)
    #ifndef IP6_ADDR_PREFERRED
        #warning "IP6_ADDR_PREFERRED not defined, check lwip/ip6_addr.h state definitions. Falling back."
        #ifdef IP6_ADDR_VALID
             #define IP6_ADDR_PREFERRED IP6_ADDR_VALID
        #else
             #error "Neither IP6_ADDR_PREFERRED nor IP6_ADDR_VALID are defined!"
        #endif
    #endif
    netif_ip6_addr_set_state(&tun_netif, 0, IP6_ADDR_PREFERRED);
    printf("State for address at index 0 set to PREFERRED (or VALID).\n");
    // --- End address assignment ---


    printf("Waiting for addresses...\n");
    sleep(1); // Give LwIP time

    printf("LwIP stack started. Interface %s (LwIP: %c%c) is up and configured.\n",
           tun_name, tun_netif.name[0], tun_netif.name[1]);


    // Main loop: Read from TUN, handle LwIP timeouts
    printf("Entering main loop...\n");
    while (1) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(tun_fd, &readfds);

        struct timeval tv;
        u32_t timeout_ms = sys_timeouts_sleeptime();

        if (timeout_ms == SYS_TIMEOUTS_SLEEPTIME_INFINITE) {
             tv.tv_sec = 5; tv.tv_usec = 0;
        } else {
             tv.tv_sec = timeout_ms / 1000; tv.tv_usec = (timeout_ms % 1000) * 1000;
        }

        int ret = select(tun_fd + 1, &readfds, NULL, NULL, &tv);

        if (ret < 0) { if (errno == EINTR) { continue; } perror("select error"); break; }

        if (FD_ISSET(tun_fd, &readfds)) {
            if (tunif_input(&tun_netif) == ERR_CONN) { fprintf(stderr, "TUN closed. Exiting.\n"); break; }
        }
        sys_check_timeouts();
    }

    // --- Cleanup ---
    printf("Shutting down...\n");
    netif_set_down(&tun_netif);
    netif_remove(&tun_netif);
    close(tun_fd);
    dtn_module_cleanup(global_dtn_module); // Cleanup DTN module

    printf("Shutdown complete.\n");
    return 0;
}