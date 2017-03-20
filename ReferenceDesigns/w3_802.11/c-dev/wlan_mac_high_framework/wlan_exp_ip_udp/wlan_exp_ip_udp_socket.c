/** @file  wlan_exp_ip_udp_socket.c
 *  @brief Mango wlan_exp IP/UDP Library (Socket)
 *
 *  @copyright Copyright 2014-2017, Mango Communications. All rights reserved.
 *          Distributed under the WARP license  (http://warpproject.org/license)
 */

/***************************** Include Files *********************************/

// Xilinx / Standard library includes
#include <xparameters.h>
#include <xstatus.h>
#include <xil_io.h>
#include <stdlib.h>
#include <stdio.h>


// Mango wlan_exp IP/UDP Library includes
#include "wlan_exp_ip_udp.h"
#include "wlan_exp_ip_udp_internal.h"


/*************************** Constant Definitions ****************************/


/*********************** Global Variable Definitions *************************/


/*************************** Variable Definitions ****************************/


/*************************** Function Prototypes *****************************/

int                          socket_check_socket(int socket_index);
wlan_exp_ip_udp_socket         * socket_get_socket(int socket_index);
wlan_exp_ip_udp_socket         * socket_alloc_socket();

/******************************** Functions **********************************/


/*****************************************************************************/
/**
 * Allocate a socket from the library
 *
 * @param   domain       - Communications domain in which a socket is to be created
 *                             - AF_INET    - Inter-network: UDP, TCP, etc. (only supported value)
 * @param   type         - Type of socket to be created
 *                             - SOCK_DGRAM - Socket datagram (connectionless) (UDP) (only supported value)
 * @param   protocol     - Particular protocol to be used with the socket
 *                             - 0 - Use default protocol appropriate for the requested socket type (only supported value)
 *
 * @return  int          - Socket Index
 *                             - WLAN_EXP_IP_UDP_FAILURE if there was an error
 *
 * @note    Only UDP sockets are supported:  socket_socket(AF_INET, SOCK_DGRAM, 0);
 *
 *****************************************************************************/
int socket_socket(int domain, int type, int protocol) {

    wlan_exp_ip_udp_socket   * socket       = NULL;

    // Check this is an appropriate socket
    if ((domain == AF_INET) && (type == SOCK_DGRAM) && (protocol == 0)) {
    
        // Allocate the socket from the library
        socket = socket_alloc_socket();
        
        if (socket != NULL) {
            // Record values in the socket
            socket->sin_family = domain;
        
            // Return the index of the socket
            return socket->index;
        }
        
    } else {
    
        // !!! TBD !!! - Print error message
    }
    
    return WLAN_EXP_IP_UDP_FAILURE;
}



/*****************************************************************************/
/**
 * Bind the socket to an Ethernet device
 *
 * @param   socket_index - Index of the socket
 * @param   type         - The type of socket to be created
 * @param   protocol     - The particular protocol to be used with the socket
 *
 * @return  int          - Status
 *                             - WLAN_EXP_IP_UDP_SUCCESS if socket is bound
 *                             - WLAN_EXP_IP_UDP_FAILURE if there was an error
 *
 *****************************************************************************/
int socket_bind_eth(int socket_index, u32 eth_dev_num, u16 port) {

    wlan_exp_ip_udp_socket   * socket;

    // Get the socket from the socket index
    socket = socket_get_socket(socket_index);
    
    if (socket != NULL) {
        // Populate the socket
        socket->state       = SOCKET_OPEN;
        socket->eth_dev_num = eth_dev_num;
        socket->sin_port    = port;

        // Populate all the header fields with static information
        eth_init_header(&(socket->hdr->eth_hdr), eth_device[eth_dev_num].hw_addr);
        ipv4_init_header(&(socket->hdr->ip_hdr), eth_device[eth_dev_num].ip_addr);
        udp_init_header(&(socket->hdr->udp_hdr), port);

        socket->sin_addr    = socket->hdr->ip_hdr.src_ip_addr;   // NOTE:  This is the big endian version of the IP address
    } else {
    
        // !!! TBD !!! - Print error message
        
        return WLAN_EXP_IP_UDP_FAILURE;
    }
    
    return WLAN_EXP_IP_UDP_SUCCESS;
}



/*****************************************************************************/
/**
 * Close a socket
 *
 * @param   socket_index - Index of socket to send the message on
 * @param   to           - Pointer to socket address structure to send the data
 * @param   buffers      - Array of wlan_exp_ip_udp_buffer describing data to send
 * @param   num_buffers  - Number of wlan_exp_ip_udp_buffer in array
 *
 * @return  int          - Socket Index
 *                             WLAN_EXP_IP_UDP_FAILURE if there was an error
 *
 *****************************************************************************/
void socket_close(int socket_index) {

    wlan_exp_ip_udp_socket   * socket = NULL;

    // Get the socket from the socket index
    socket = socket_get_socket(socket_index);
    
    if (socket != NULL) {
        // Mark the socket as CLOSED
        socket->state       = SOCKET_CLOSED;
        socket->eth_dev_num = WLAN_EXP_IP_UDP_INVALID_ETH_DEVICE;
        
    } else {
    
        // !!! TBD !!! - Print error message
        
    }
}



/*****************************************************************************/
/**
 * Send the given data to the given socket
 *
 * @param   socket_index - Index of socket to send the message on
 * @param   to           - Pointer to socket address structure to send the data
 * @param   buffers      - Array of wlan_exp_ip_udp_buffer describing data to send
 * @param   num_buffers  - Number of wlan_exp_ip_udp_buffer in array
 *
 * @return  int          - Number of bytes sent
 *                             WLAN_EXP_IP_UDP_FAILURE if there was an error
 *
 *****************************************************************************/
int socket_sendto(int socket_index, struct sockaddr * to, wlan_exp_ip_udp_buffer ** buffers, u32 num_buffers) {

    u32                      i;
    int                      status;
    wlan_exp_ip_udp_socket     * socket;
    
    u32                      eth_dev_num;
    u8                       dest_hw_addr[ETH_ADDR_LEN];
    u32                      dest_ip_addr = ((struct sockaddr_in*)to)->sin_addr.s_addr;    // NOTE:  Value big endian
    u16                      dest_port    = ((struct sockaddr_in*)to)->sin_port;

    // Length variables for the various headers
    u16                      ip_length    = WLAN_EXP_IP_UDP_DELIM_LEN + UDP_HEADER_LEN + IP_HEADER_LEN_BYTES;
    u16                      udp_length   = WLAN_EXP_IP_UDP_DELIM_LEN + UDP_HEADER_LEN;
    u16                      data_length  = 0;
    
    // Get the socket
    socket = socket_get_socket(socket_index);
    
    if (socket == NULL) {
    
        // !!! TBD !!! - Print error message

        return WLAN_EXP_IP_UDP_FAILURE;
    }

    // Get the Ethernet device from the socket
    eth_dev_num = socket->eth_dev_num;
    
    // Look up destination HW address in ARP table
    status = arp_get_hw_addr(eth_dev_num, dest_hw_addr, (u8 *)(&dest_ip_addr));
    
    if (status != XST_SUCCESS) {
    
        // !!! TBD !!! - Print error message

        return WLAN_EXP_IP_UDP_FAILURE;
    }
    
    // Compute total length of packet data
    for (i = 0; i < num_buffers; i++) {
        data_length += buffers[i]->size;
    }
    
    // Add the data_length to the other header lengths
    ip_length  += data_length;
    udp_length += data_length;
    
    // Update the UDP header
    //     NOTE:  Requires dest_port to be big-endian; udp_length to be little-endian
    //
    udp_update_header(&(socket->hdr->udp_hdr), dest_port, udp_length);

    // Update the IPv4 header
    //     NOTE:  Requires dest_ip_addr to be big-endian; ip_length to be little-endian
    //
    ipv4_update_header(&(socket->hdr->ip_hdr), dest_ip_addr, ip_length, IP_PROTOCOL_UDP);

    // Update the Ethernet header
    //     NOTE:  dest_hw_addr must be big-endian; ethertype must be little-endian
    //
    eth_update_header(&(socket->hdr->eth_hdr), dest_hw_addr, ETHERTYPE_IP_V4);

    // Send the Ethernet frame using the socket header
    return eth_send_frame(eth_dev_num, socket, buffers, num_buffers, 0x1);
}



int socket_sendto_raw(int socket_index, wlan_exp_ip_udp_buffer ** buffers, u32 num_buffers) {

    wlan_exp_ip_udp_socket     * socket;
    u32                      eth_dev_num;

    // Get the socket
    socket = socket_get_socket(socket_index);

    if (socket == NULL) {

        // !!! TBD !!! - Print error message

        return WLAN_EXP_IP_UDP_FAILURE;
    }

    // Get the Ethernet device from the socket
    eth_dev_num = socket->eth_dev_num;

    // Send the Ethernet frame using the socket header
    return eth_send_frame(eth_dev_num, socket, buffers, num_buffers, 0x0);
}



/*****************************************************************************/
/**
 * Try to receive on the socket
 *
 * @param   eth_dev_num      - Ethernet device number
 * @param   socket_index     - Pointer to Socket index where the data is from
 * @param   from             - Pointer to socket address structure where the data is from
 * @param   buffer           - Mango wlan_exp IP/UDP Buffer describing data (will be filled in)
 *
 * @return  int              - Number of bytes of data in the processed packet
 *                                 0 if there was no data in the packet
 *                                -1 if there was a library failure
 *
 * @note    socket_recv will return a wlan_exp_ip_udp_buffer such that:
 *              state      = WLAN_EXP_IP_UDP_BUFFER_IN_USE                                               (do not change)
 *              max_size   = Send buffer size                                                 (do not change)
 *              size       = Full size of the packet                                          (do not change)
 *              data       = Pointer to start of the packet                                   (do not change)
 *              offset     = Pointer to start of UDP packet data
 *              length     = Length of UDP packet data
 *              descriptor = Pointer to buffer descriptor associated with the data buffer     (do not change) 
 *
 * @note    The library internally allocates receive buffers, but it is the requirement
 *          of user code to free the buffer to indicate to the library it can re-use
 *          the memory.  This allows the library to pre-allocate all of the buffer
 *          descriptors used by the Ethernet DMA.
 *
 *****************************************************************************/
int socket_recvfrom_eth(u32 eth_dev_num, int * socket_index, struct sockaddr * from, wlan_exp_ip_udp_buffer * buffer) {

    wlan_exp_ip_udp_header     * header;
    int                      recv_bytes;
    struct sockaddr_in     * socket_addr;
    
    // Try to receive an Ethernet frame
    recv_bytes = eth_recv_frame(eth_dev_num, buffer);

    // Fill in the socket information
    if (recv_bytes > 0) {
        // If there were received bytes, buffer->data is a pointer to the beginning of the packet
        header = (wlan_exp_ip_udp_header *) buffer->data;
        
        // Get the socket index for the socket this packet was intended
        *socket_index                = socket_find_index_by_eth(eth_dev_num, Xil_Ntohs((header->udp_hdr).dest_port));
    
        // Since this is a valid packet, use the header to fill in the socket information
        socket_addr                  = (struct sockaddr_in *) from;
        socket_addr->sin_family      = AF_INET;
        socket_addr->sin_port        = header->udp_hdr.src_port;
        socket_addr->sin_addr.s_addr = header->ip_hdr.src_ip_addr;

    } else if(recv_bytes < 0) {
    	xil_printf("eth_recv_frame returned error: %d\n", recv_bytes);
    }
    
    return recv_bytes;
}



/*****************************************************************************/
/**
 * Allocate a send buffer from the library
 *
 * @param   None
 *
 * @return  wlan_exp_ip_udp_buffer *  - Pointer to Mango wlan_exp IP/UDP Buffer allocated
 *                                  NULL if buffer was not allocated
 *
 * @note    alloc_send_buffer will return a wlan_exp_ip_udp_buffer such that:
 *              state      = WLAN_EXP_IP_UDP_BUFFER_IN_USE              (do not change)
 *              max_size   = Send buffer size                (do not change)
 *              size       = 0
 *              data       = Pointer to start of the packet  (do not change)
 *              offset     = data
 *              length     = 0
 *              descriptor = NULL                            (do not change)
 *
 * @note    The size of the send buffer must be set to the total number of bytes
 *          in the buffer for it to be processed correctly.  The offset and length
 *          can be changed by the user but will not be used by the send framework.
 *
 * @note    Currently, the max_size of the buffers is fixed at WLAN_EXP_IP_UDP_ETH_*_BUF_SIZE
 *          bytes.  This could be modified in the future to have a pool of bytes and
 *          have this code allocate bytes out of that pool similar to malloc for more 
 *          efficient memory usage.
 *
 *****************************************************************************/
wlan_exp_ip_udp_buffer * socket_alloc_send_buffer() {

    u32                      i;
    wlan_exp_ip_udp_buffer     * buffer         = NULL;
    
    // If we have allocated less than the number of buffers, then allocate one
    if (ETH_allocated_send_buffers < WLAN_EXP_IP_UDP_ETH_NUM_SEND_BUF) {
        
        // Find the first buffer that is free
        for (i = 0; i < WLAN_EXP_IP_UDP_ETH_NUM_SEND_BUF; i++) {
            if (ETH_send_buffers[i].state == WLAN_EXP_IP_UDP_BUFFER_FREE) { break; }
        }
        
        buffer = &(ETH_send_buffers[i]);
        
        // Initialize the buffer (see documentation above):
        buffer->state      = WLAN_EXP_IP_UDP_BUFFER_IN_USE;
        buffer->size       = 0;
        buffer->offset     = buffer->data;
        buffer->length     = 0;
        buffer->descriptor = NULL;
        
        // Increment number of allocated buffers
        ETH_allocated_send_buffers += 1;
        
    } else {
    
        // All buffers are in use, return NULL
        
        xil_printf("ERROR - All Buffers in Use!\n");
        // !!! TBD !!! - Print error message
        
    }
    
    return buffer;
}



/*****************************************************************************/
/**
 * Free a send buffer for reuse
 *
 * @param   buffer           - Pointer to Mango wlan_exp IP/UDP buffer to be freed
 *
 * @return  None
 *
 *****************************************************************************/
void socket_free_send_buffer(wlan_exp_ip_udp_buffer * buffer) {

    // Check that we are not un-allocating a buffer in error
    if (ETH_allocated_send_buffers > 0) {
    
        // Free the buffer (will be re-initialized on the next allocation)
        buffer->state      = WLAN_EXP_IP_UDP_BUFFER_FREE;
        
        // Decrement number of allocated buffers
        ETH_allocated_send_buffers -= 1;
        
    } else {
    
        // !!! TBD !!! - Print error message
    }
}



/*****************************************************************************/
/**
 * Free a receive buffer for reuse
 *
 * @param   socket_index     - Socket index
 * @param   buffer           - Pointer to Mango wlan_exp IP/UDP buffer to be freed
 *
 * @return  None
 *
 * @note    The library internally allocates receive buffers, but it is the requirement
 *          of user code to free the buffer to indicate to the library it can re-use
 *          the memory.  This allows the library to pre-allocate all of the buffer
 *          descriptors used by the Ethernet DMA.
 *
 *****************************************************************************/
void socket_free_recv_buffer(int socket_index, wlan_exp_ip_udp_buffer * buffer) {

    u32                      eth_dev_num;
    wlan_exp_ip_udp_socket     * socket         = NULL;

    // Get the socket from the socket index
    socket = socket_get_socket(socket_index);
    
    if (socket != NULL) {
        eth_dev_num = socket->eth_dev_num;
        
        // Free the receive buffer descriptor to be re-used
        if (eth_free_recv_buffers(eth_dev_num, buffer->descriptor, 1) != XST_SUCCESS) {
        
            // !!! TBD !!! - Print error message
        }
        
    } else {
    
        // !!! TBD !!! - Print error message
    }
}



/*****************************************************************************/
/**
 * Find the socket_index by Ethernet device & Port
 *
 * @param   eth_dev_num      - Ethernet device number
 * @param   port             - Port of the socket
 *
 * @return  int              - Socket Index
 *                                 WLAN_EXP_IP_UDP_FAILURE if there was an error
 *
 *****************************************************************************/
int socket_find_index_by_eth(u32 eth_dev_num, u16 port) {

    u32 i;
    int socket_index   = SOCKET_INVALID_SOCKET;

    // Search through socket pool
    for (i = 0; i < WLAN_EXP_IP_UDP_NUM_SOCKETS; i++) {

        // Check socket parameters    
        if ((ETH_sockets[i].state       == SOCKET_OPEN) &&
            (ETH_sockets[i].eth_dev_num == eth_dev_num) &&
            (ETH_sockets[i].sin_port    == port       ) ) {
            
            socket_index = i;
            break;
        }
    }
    
    return socket_index;
}



/*****************************************************************************/
/**
 * Find the Ethernet device from the socket_index
 *
 * @param   socket_index     - Index of the socket
 *
 * @return  u32              - Ethernet device number
 *                                 WLAN_EXP_IP_UDP_INVALID_ETH_DEVICE if there was an error
 *
 *****************************************************************************/
u32 socket_get_eth_dev_num(int socket_index) {

    wlan_exp_ip_udp_socket     * socket         = NULL;

    socket = socket_get_socket(socket_index);

    if (socket == NULL) {
        return WLAN_EXP_IP_UDP_INVALID_ETH_DEVICE;
    } else {
        return (socket->eth_dev_num);
    }
}



/*****************************************************************************/
/**
 * Get a pointer to the wlan_exp_ip_udp_header of socket from the socket_index
 *
 * @param   socket_index          - Index of the socket
 *
 * @return  wlan_exp_ip_udp_header *  - Ethernet device number
 *                                      NULL if there was an error
 *
 *****************************************************************************/
wlan_exp_ip_udp_header * socket_get_wlan_exp_ip_udp_header(int socket_index) {

    wlan_exp_ip_udp_socket     * socket         = NULL;

    socket = socket_get_socket(socket_index);

    if (socket == NULL) {
        return NULL;
    } else {
        return (socket->hdr);
    }
}




/**********************************************************************************************************************/
/**
 * @brief Internal Methods
 *
 **********************************************************************************************************************/

/******************************************************************************/
/**
 * Allocate a socket from the global pool
 *
 * @param   None
 *
 * @return  wlan_exp_ip_udp_socket *       - Pointer to the socket
 *                                           NULL if not able to get the socket
 *
 *****************************************************************************/
wlan_exp_ip_udp_socket * socket_alloc_socket() {

    u32                      i;
    int                      socket_index   = SOCKET_INVALID_SOCKET;
    wlan_exp_ip_udp_socket     * socket         = NULL;

    // Search through socket pool for a closed socket
    for (i = 0; i < WLAN_EXP_IP_UDP_NUM_SOCKETS; i++) {
        if (ETH_sockets[i].state == SOCKET_CLOSED) { 
            socket_index = i;
            break;
        }
    }

    if (socket_index != SOCKET_INVALID_SOCKET) {
        // Get the socket from the global structure
        socket = &(ETH_sockets[socket_index]);
        
        // Set the socket state to "ALLOCATED"
        socket->state = SOCKET_ALLOCATED;
        
    } else {
    
        // !!! TBD !!! - Print error message
    }
    
    return socket;
} 


 
/******************************************************************************/
/**
 * Get a pointer to the socket from the socket index
 *
 * @param   socket_index               - Index of the socket
 *
 * @return  wlan_exp_ip_udp_socket *       - Pointer to the socket
 *                                           NULL if not able to get the socket
 *
 *****************************************************************************/
wlan_exp_ip_udp_socket * socket_get_socket(int socket_index) {

    wlan_exp_ip_udp_socket     * socket = NULL;

    // Check the socket index
    if (socket_check_socket(socket_index) == XST_SUCCESS) {
    
        // Get the socket from the global structure
        socket = &(ETH_sockets[socket_index]);
        
        // Check the socket status; if it is closed, then return NULL
        if (socket->state == SOCKET_CLOSED) {
        
            // !!! TBD !!! - Print error message
            
            socket = NULL;
        }
    }
    
    return socket;
}



/*****************************************************************************/
/**
 * Check the socket index
 *
 * @param   socket_index     - Index of the socket
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS - Command completed successfully
 *                                 XST_FAILURE - There was an error in the command
 *
 *****************************************************************************/
int socket_check_socket(int socket_index) {

    // Check that the socket_index is valid
    if ((socket_index < 0) || (socket_index > WLAN_EXP_IP_UDP_NUM_SOCKETS)) {
    
        // !!! TBD !!! - Print error message
        
        return XST_FAILURE;
    }

    return XST_SUCCESS;
}



