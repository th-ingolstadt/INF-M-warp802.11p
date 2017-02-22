/** @file  wlan_exp_ip_udp_eth.c
 *  @brief Mango wlan_exp IP/UDP Library (Ethernet)
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

// Xilinx Hardware includes
#include <xaxidma.h>
#include <xaxiethernet.h>

// Mango wlan_exp IP/UDP Library includes
#include "wlan_exp_ip_udp.h"
#include "wlan_exp_ip_udp_internal.h"


/*************************** Constant Definitions ****************************/

// Ethernet Device Error Message Numbers
#define WLAN_EXP_IP_UDP_ETH_ERROR_NUM_DEV                      0                   // Ethernet device out of range
#define WLAN_EXP_IP_UDP_ETH_ERROR_INTIALIZED                   1                   // Ethernet device not initialized
#define WLAN_EXP_IP_UDP_ETH_ERROR_CODE                         2                   // Other error (see error code for more information)


// Ethernet Error Codes
#define ETH_ERROR_CODE_ETH_DEVICE_INIT                     0x00000000          // Error initializing Ethernet device
#define ETH_ERROR_CODE_ETH_CFG_INIT                        0x00000001          // Error initializing Ethernet config
#define ETH_ERROR_CODE_ETH_CLR_OPT                         0x00000002          // Error clearing Ethernet options
#define ETH_ERROR_CODE_ETH_SET_OPT                         0x00000003          // Error setting Ethernet options

#define ETH_ERROR_CODE_DMA_INIT                            0x00000100          // Error initializing DMA
#define ETH_ERROR_CODE_DMA_CFG_INIT                        0x00000101          // Error initializing DMA config

#define ETH_ERROR_CODE_DMA_RX_ERROR                        0x00000110          // Error in DMA RX BD
#define ETH_ERROR_CODE_DMA_RX_BD_RING_CREATE               0x00000111          // Error creating RX BD ring
#define ETH_ERROR_CODE_DMA_RX_BD_RING_CLONE                0x00000112          // Error cloning RX BD ring
#define ETH_ERROR_CODE_DMA_RX_BD_RING_ALLOC                0x00000113          // Error allocating RX BD ring
#define ETH_ERROR_CODE_DMA_RX_BD_RING_TO_HW                0x00000114          // Error submitting RX BD ring to hardware
#define ETH_ERROR_CODE_DMA_RX_BD_RING_START                0x00000115          // Error starting RX BD ring
#define ETH_ERROR_CODE_DMA_RX_BD_RING_FREE                 0x00000116          // Error freeing RX BD ring

#define ETH_ERROR_CODE_DMA_TX_ERROR                        0x00000120          // Error in DMA TX BD
#define ETH_ERROR_CODE_DMA_TX_BD_RING_CREATE               0x00000121          // Error creating TX BD ring
#define ETH_ERROR_CODE_DMA_TX_BD_RING_CLONE                0x00000122          // Error cloning TB BD ring
#define ETH_ERROR_CODE_DMA_TX_BD_RING_ALLOC                0x00000123          // Error allocating TX BD ring
#define ETH_ERROR_CODE_DMA_TX_BD_RING_TO_HW                0x00000124          // Error submitting TX BD ring to hardware
#define ETH_ERROR_CODE_DMA_TX_BD_RING_START                0x00000125          // Error starting TX BD ring
#define ETH_ERROR_CODE_DMA_TX_BD_RING_FREE                 0x00000126          // Error freeing TX BD ring

#define ETH_ERROR_CODE_DMA_BD_SET_BUF_ADDR                 0x00000130          // Error setting BD address
#define ETH_ERROR_CODE_DMA_BD_SET_LENGTH                   0x00000131          // Error setting BD length

#define ETH_ERROR_CODE_TX_BD_CNT                           0x00000200          // Not enough TX BDs to complete transfer
#define ETH_ERROR_CODE_TX_HANG                             0x00000201          // Hang in the DMA while trying to transmit buffer
#define ETH_ERROR_CODE_TX_DESCRIPTOR_ERR                   0x00000202          // Error trying to allocate Tx descriptors
#define ETH_ERROR_CODE_TX_LENGTH_MISMATCH                  0x00000203          // Length of processed descriptors does not match length of submitted descriptors




/*********************** Global Variable Definitions *************************/



/*************************** Variable Definitions ****************************/

volatile eth_int_enable_func_ptr_t     interrupt_enable_callback;              // Interrupt enable callback
volatile eth_int_disable_func_ptr_t    interrupt_disable_callback;             // Interrupt disable callback


/*************************** Function Prototypes *****************************/

int           eth_init_dma(u32 eth_dev_num, u32 verbose);
int           eth_init_device(u32 eth_dev_num, u32 verbose);

int           eth_null_interrupt_callback(int param);

int           eth_check_device(u32 eth_dev_num);
void          eth_check_dma(u32 eth_dev_num);

inline int    eth_process_tx_descriptors(u32 eth_dev_num, XAxiDma_BdRing * dma_tx_ring_ptr);

void          eth_print_err_msg(u32 eth_dev_num, u32 msg_num, u32 error_code, void * data, u32 data_length);


#if _DEBUG_

void print_pkt(u8 * buf, int size);
void print_XAxiDma_Bd(XAxiDma_Bd * bd_ptr);

#endif

/******************************** Functions **********************************/


/*****************************************************************************/
/**
 * Initialize the Ethernet subsystem
 *
 * @param   eth_dev_num      - Ethernet device number
 * @param   hw_addr          - u8 pointer of MAC address of the Ethernet device
 * @param   ip_addr          - u8 pointer of IP address of the Ethernet device
 * @param   verbose          - Print initialization message(s)
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS - Command completed successfully
 *                                 XST_FAILURE - There was an error in the command
 *
 ******************************************************************************/
int eth_init(u32 eth_dev_num, u8 * hw_addr, u8 * ip_addr, u32 verbose) {

    int                 status;
    
    // NOTE:  In this instance we need to split the eth_device_check() since we 
    //        are initializing the Ethernet device info that is used as part of the check
    
    // Check to see if we are sending on a valid interface
    if (eth_dev_num >= WLAN_EXP_IP_UDP_NUM_ETH_DEVICES) {
        eth_print_err_msg(eth_dev_num, WLAN_EXP_IP_UDP_ETH_ERROR_NUM_DEV, 0, NULL, 0);
        return XST_FAILURE;
    }

    // Initialize callbacks
    interrupt_enable_callback  = (eth_int_enable_func_ptr_t)  eth_null_interrupt_callback;
    interrupt_disable_callback = (eth_int_disable_func_ptr_t) eth_null_interrupt_callback;
    
    // Initialize the Ethernet device structure
    eth_init_device_info(eth_dev_num);
    
    // Check if Ethernet device has been initialized
    if (!eth_device[eth_dev_num].initialized) {
        eth_print_err_msg(eth_dev_num, WLAN_EXP_IP_UDP_ETH_ERROR_INTIALIZED, 0, NULL, 0);
        return XST_FAILURE;
    }

    // Set the IP / HW address information in the Ethernet device structure
    eth_set_ip_addr(eth_dev_num, ip_addr);
    eth_set_hw_addr(eth_dev_num, hw_addr);

    // Print initialization information (if required)
    if (verbose) {
        u32 num_recv_buffers = eth_device[eth_dev_num].num_recv_buffers;
        
        xil_printf("  Configuring ETH %c with %d byte buffers (%d receive, %d send)\n",
                   wlan_exp_conv_eth_dev_num(eth_dev_num), WLAN_EXP_IP_UDP_ETH_BUF_SIZE, num_recv_buffers, WLAN_EXP_IP_UDP_ETH_NUM_SEND_BUF);
    }
    
    // Initialize the DMA
    status = eth_init_dma(eth_dev_num, verbose);
    
    if(status != XST_SUCCESS) {
        eth_print_err_msg(eth_dev_num, WLAN_EXP_IP_UDP_ETH_ERROR_CODE, ETH_ERROR_CODE_DMA_INIT, &status, 1);
        return XST_FAILURE;
    }
    
    // Initialize the Ethernet device
    status = eth_init_device(eth_dev_num, verbose);

    if(status != XST_SUCCESS) {
        eth_print_err_msg(eth_dev_num, WLAN_EXP_IP_UDP_ETH_ERROR_CODE, ETH_ERROR_CODE_ETH_DEVICE_INIT, &status, 1);
        return XST_FAILURE;
    }
    
    return XST_SUCCESS;
}



/*****************************************************************************/
/**
 * Initialize the Ethernet DMA
 *
 * @param   eth_dev_num      - Ethernet device number
 * @param   verbose          - Print initialization message(s)
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS - Command completed successfully
 *                                 XST_FAILURE - There was an error in the command
 *
 ******************************************************************************/
int eth_init_dma(u32 eth_dev_num, u32 verbose) {

    int                      i;
    int                      status;
    
    XAxiDma                * dma_ptr;
    XAxiDma_Config         * dma_config_ptr;
    
    XAxiDma_BdRing         * dma_rx_ring_ptr;
    u32                      dma_rx_bd_ptr;
    u32                      dma_rx_bd_cnt;

    XAxiDma_BdRing         * dma_tx_ring_ptr;    
    u32                      dma_tx_bd_ptr;
    u32                      dma_tx_bd_cnt;

    XAxiDma_Bd               bd_template;
    XAxiDma_Bd            ** bd_set_ptr          = NULL;
    XAxiDma_Bd             * bd_ptr              = NULL;
    
    u32                      num_recv_buffers;
    wlan_exp_ip_udp_buffer     * recv_buffers;

    // Get DMA info from Ethernet device structure
    dma_ptr           = (XAxiDma *) eth_device[eth_dev_num].dma_ptr;
    dma_config_ptr    = (XAxiDma_Config *) eth_device[eth_dev_num].dma_cfg_ptr;
    
    dma_rx_ring_ptr   = (XAxiDma_BdRing *) eth_device[eth_dev_num].dma_rx_ring_ptr;
    dma_rx_bd_ptr     = (u32) eth_device[eth_dev_num].dma_rx_bd_ptr;
    dma_rx_bd_cnt     = eth_device[eth_dev_num].dma_rx_bd_cnt;
    
    dma_tx_ring_ptr   = (XAxiDma_BdRing *) eth_device[eth_dev_num].dma_tx_ring_ptr;
    dma_tx_bd_ptr     = (u32) eth_device[eth_dev_num].dma_tx_bd_ptr;
    dma_tx_bd_cnt     = eth_device[eth_dev_num].dma_tx_bd_cnt;
    
    num_recv_buffers  = eth_device[eth_dev_num].num_recv_buffers;
    recv_buffers      = eth_device[eth_dev_num].recv_buffers;
    
    // Initialize AXI DMA engine. AXI DMA engine must be initialized before AXI Ethernet.
    // During AXI DMA engine initialization, AXI DMA hardware is reset, and since AXI DMA
    // reset line is connected to the AXI Ethernet, this would ensure a reset of the AXI 
    // Ethernet.
    //
    status = XAxiDma_CfgInitialize(dma_ptr, dma_config_ptr);
    
    if(status != XST_SUCCESS) {
        eth_print_err_msg(eth_dev_num, WLAN_EXP_IP_UDP_ETH_ERROR_CODE, ETH_ERROR_CODE_DMA_CFG_INIT, &status, 1);
        return XST_FAILURE;
    }

    // Set up the buffer descriptor template that will be copied to all the buffer descriptors
    // in the TX / RX rings
    //
    XAxiDma_BdClear(&bd_template);
    
    // Setup the RX Buffer Descriptor space:
    //   - RX buffer descriptor space is a properly aligned area of memory
    //   - No MMU is being used so the physical and virtual addresses are the same.
    //
    // Create the RX ring
    status = XAxiDma_BdRingCreate(dma_rx_ring_ptr, dma_rx_bd_ptr, dma_rx_bd_ptr, WLAN_EXP_IP_UDP_BD_ALIGNMENT, dma_rx_bd_cnt);
    
    if (status != XST_SUCCESS) {
        eth_print_err_msg(eth_dev_num, WLAN_EXP_IP_UDP_ETH_ERROR_CODE, ETH_ERROR_CODE_DMA_RX_BD_RING_CREATE, &status, 1);
        return XST_FAILURE;
    }

    // Initialize the RX ring using the descriptor template
    status = XAxiDma_BdRingClone(dma_rx_ring_ptr, &bd_template);
    
    if (status != XST_SUCCESS) {
        eth_print_err_msg(eth_dev_num, WLAN_EXP_IP_UDP_ETH_ERROR_CODE, ETH_ERROR_CODE_DMA_RX_BD_RING_CLONE, &status, 1);
        return XST_FAILURE;
    }

    // Setup the TX Buffer Descriptor space:
    //   - TX buffer descriptor space is a properly aligned area of memory
    //   - No MMU is being used so the physical and virtual addresses are the same.
    //
    // Create the TX BD ring
    status = XAxiDma_BdRingCreate(dma_tx_ring_ptr, dma_tx_bd_ptr, dma_tx_bd_ptr, WLAN_EXP_IP_UDP_BD_ALIGNMENT, dma_tx_bd_cnt);
    
    if (status != XST_SUCCESS) {
        eth_print_err_msg(eth_dev_num, WLAN_EXP_IP_UDP_ETH_ERROR_CODE, ETH_ERROR_CODE_DMA_TX_BD_RING_CREATE, &status, 1);
        return XST_FAILURE;
    }

    // Initialize the TX ring using the descriptor template
    status = XAxiDma_BdRingClone(dma_tx_ring_ptr, &bd_template);
    
    if (status != XST_SUCCESS) {
        eth_print_err_msg(eth_dev_num, WLAN_EXP_IP_UDP_ETH_ERROR_CODE, ETH_ERROR_CODE_DMA_TX_BD_RING_CLONE, &status, 1);
        return XST_FAILURE;
    }

    // Initialize RX descriptor space:
    //   - Allocate 1 buffer descriptor for each receive buffer
    //   - Set up each descriptor to use a portion of the allocated receive buffer
    //
    // Allocate receive buffers
    status = XAxiDma_BdRingAlloc(dma_rx_ring_ptr, num_recv_buffers, bd_set_ptr);
    
    if (status != XST_SUCCESS) {
        eth_print_err_msg(eth_dev_num, WLAN_EXP_IP_UDP_ETH_ERROR_CODE, ETH_ERROR_CODE_DMA_RX_BD_RING_ALLOC, &status, 1);
        return XST_FAILURE;
    }

    // Set up the buffer descriptors
    bd_ptr = bd_set_ptr[0];
    
    for ( i = 0; i < num_recv_buffers; i++ ){    
        XAxiDma_BdSetBufAddr(bd_ptr, (u32)(recv_buffers[i].data));
        XAxiDma_BdSetLength(bd_ptr, WLAN_EXP_IP_UDP_ETH_BUF_SIZE, dma_rx_ring_ptr->MaxTransferLen);
        XAxiDma_BdSetCtrl(bd_ptr, 0);
        XAxiDma_BdSetId(bd_ptr, (u32)(recv_buffers[i].data));

        bd_ptr = (XAxiDma_Bd*)XAxiDma_BdRingNext(dma_rx_ring_ptr, bd_ptr);
    }

    // Enqueue buffer descriptors to hardware
    status = XAxiDma_BdRingToHw(dma_rx_ring_ptr, num_recv_buffers, bd_set_ptr[0]);
    
    if (status != XST_SUCCESS) {
        eth_print_err_msg(eth_dev_num, WLAN_EXP_IP_UDP_ETH_ERROR_CODE, ETH_ERROR_CODE_DMA_RX_BD_RING_TO_HW, &status, 1);
        return XST_FAILURE;
    }

    //    
    // NOTE:  We do not need to do any additional setup for the transmit buffer descriptors
    //        since those will be allocated and processed as part of the send process.
    //

    return XST_SUCCESS;  
}



/*****************************************************************************/
/**
 * Initialize the Ethernet device
 *
 * @param   eth_dev_num      - Ethernet device number
 * @param   verbose          - Print initialization message(s)
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS - Command completed successfully
 *                                 XST_FAILURE - There was an error in the command
 *
 ******************************************************************************/
int eth_init_device(u32 eth_dev_num, u32 verbose) {

    int                      status;
    
    XAxiEthernet           * eth_ptr;
    XAxiEthernet_Config    * eth_config_ptr;

    // Get Ethernet info from Ethernet device structure
    eth_ptr           = (XAxiEthernet *) eth_device[eth_dev_num].eth_ptr;
    eth_config_ptr    = (XAxiEthernet_Config *) eth_device[eth_dev_num].eth_cfg_ptr;
    
    // Initialize Ethernet Device
    status = XAxiEthernet_CfgInitialize(eth_ptr, eth_config_ptr, eth_config_ptr->BaseAddress);
    
    if (status != XST_SUCCESS) {
        eth_print_err_msg(eth_dev_num, WLAN_EXP_IP_UDP_ETH_ERROR_CODE, ETH_ERROR_CODE_ETH_CFG_INIT, &status, 1);
        return XST_FAILURE;
    }

    // Disable Ethernet options: (found in xaxiethernet.h)
    //     - XAE_LENTYPE_ERR_OPTION specifies the Axi Ethernet device to enable Length/Type error checking (mismatched type/length field) for received frames. 
    //     - XAE_FLOW_CONTROL_OPTION specifies the Axi Ethernet device to recognize received flow control frames.
    //     - XAE_FCS_STRIP_OPTION specifies the Axi Ethernet device to strip FCS and PAD from received frames.
    //
    status = XAxiEthernet_ClearOptions(eth_ptr, XAE_LENTYPE_ERR_OPTION | XAE_FLOW_CONTROL_OPTION | XAE_FCS_STRIP_OPTION);
    
    if (status != XST_SUCCESS) {
        eth_print_err_msg(eth_dev_num, WLAN_EXP_IP_UDP_ETH_ERROR_CODE, ETH_ERROR_CODE_ETH_CLR_OPT, &status, 1);
        return XST_FAILURE;
    }

    // Enable Ethernet options: (found in xaxiethernet.h)
    //     - XAE_PROMISC_OPTION specifies the Axi Ethernet device to accept all incoming packets.
    //     - XAE_MULTICAST_OPTION specifies the Axi Ethernet device to receive frames sent to Ethernet addresses that are programmed into the Multicast Address Table (MAT).
    //     - XAE_BROADCAST_OPTION specifies the Axi Ethernet device to receive frames sent to the broadcast Ethernet address.
    //     - XAE_RECEIVER_ENABLE_OPTION specifies the Axi Ethernet device receiver to be enabled.
    //     - XAE_TRANSMITTER_ENABLE_OPTION specifies the Axi Ethernet device transmitter to be enabled.
    //     - XAE_JUMBO_OPTION specifies the Axi Ethernet device to accept jumbo frames for transmit and receive.
    //    
    status = XAxiEthernet_SetOptions(eth_ptr, XAE_PROMISC_OPTION | XAE_MULTICAST_OPTION | XAE_BROADCAST_OPTION | XAE_RECEIVER_ENABLE_OPTION | XAE_TRANSMITTER_ENABLE_OPTION | XAE_JUMBO_OPTION);
    
    if (status != XST_SUCCESS) {
        eth_print_err_msg(eth_dev_num, WLAN_EXP_IP_UDP_ETH_ERROR_CODE, ETH_ERROR_CODE_ETH_SET_OPT, &status, 1);
        return XST_FAILURE;
    }

    return XST_SUCCESS;
}



/*****************************************************************************/
/**
 * Start the Ethernet device
 *
 * @param   eth_dev_num      - Ethernet device number
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS - Command completed successfully
 *                                 XST_FAILURE - There was an error in the command
 *
 ******************************************************************************/
int eth_start_device(u32 eth_dev_num) {

    int                      status;

    XAxiEthernet           * eth_ptr;
    XAxiDma_BdRing         * dma_rx_ring_ptr;
    
    // Check the Ethernet device
    if (eth_check_device(eth_dev_num) == XST_FAILURE) {
        return XST_FAILURE;
    }

    // Get Ethernet / DMA info from Ethernet device structure
    eth_ptr           = (XAxiEthernet *) eth_device[eth_dev_num].eth_ptr;
    dma_rx_ring_ptr   = (XAxiDma_BdRing *) eth_device[eth_dev_num].dma_rx_ring_ptr;

    // Start the Ethernet device
    XAxiEthernet_Start(eth_ptr);
    
    // Start DMA RX channel
    status = XAxiDma_BdRingStart(dma_rx_ring_ptr);
    
    if (status != XST_SUCCESS) {
        eth_print_err_msg(eth_dev_num, WLAN_EXP_IP_UDP_ETH_ERROR_CODE, ETH_ERROR_CODE_DMA_RX_BD_RING_START, &status, 1);
        return XST_FAILURE;
    }

    // NOTE:  Now Ethernet subsystem is ready to receive data

    return XST_SUCCESS;
}



/********************************************************************
 * Ethernet Null Callback
 *
 * This function is part of the callback system for processing Ethernet packets.
 *
 * @param   int  param       - Parameters for the callback
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS  - Command successful
 *
 ********************************************************************/
int eth_null_interrupt_callback(int param){
    return XST_SUCCESS;
}



/*****************************************************************************/
/**
 * Set the Ethernet callbacks
 *
 * @param   void * callback  - Pointer to the callback function
 *
 * @return  None
 *
 *****************************************************************************/
void eth_set_interrupt_enable_callback(void(*callback)()){
    interrupt_enable_callback = (eth_int_enable_func_ptr_t) callback;
}


void eth_set_interrupt_disable_callback(void(*callback)()){
    interrupt_disable_callback = (eth_int_disable_func_ptr_t) callback;
}



/*****************************************************************************/
/**
 * Send Ethernet frame
 *
 * @note    Sending Ethernet frames is not interrupt safe.  Since the library does not know
 *          if it will be included in a system that contains interrupts, the library implements
 *          two callback functions to control interrupts.  These functions follow the following
 *          conventions:
 *              status = interrupt_disable_callback();
 *              interrupt_enable_callback(status);
 *          where the interrupt disable callback returns a status integer that is then used to
 *          selectively re-enable interrupts.  If these callbacks are not set in user code, then
 *          they will just call the eth_null_interrupt_callback and do nothing.
 *
 * @param   eth_dev_num       - Ethernet device number
 * @param   socket            - Socket used for this transmission
 * @param   buffers           - Pointer to an array of IP/UDP buffers for the transmission
 * @param   num_buffers       - Number IP/UDP buffers in the transmission
 * @param   use_socket_header - Use the header in the socket or ignore it (because IP/UDP buffers contain the header)
 *
 * @return  int               - Number of bytes transmitted
 *                                 WLAN_EXP_IP_UDP_FAILURE - There was an error in the command
 *
 *****************************************************************************/
int eth_send_frame(u32 eth_dev_num, wlan_exp_ip_udp_socket * socket, wlan_exp_ip_udp_buffer ** buffers, u32 num_buffers, u32 use_socket_header) {

    int                      i;
    int                      status;
    int                      int_status;
    
    wlan_exp_ip_udp_buffer       socket_hdr;
    wlan_exp_ip_udp_buffer       padding_buffer;
    u32                      eth_frame_length              = 0;
    
    wlan_exp_ip_udp_buffer     * buffers_to_process[WLAN_EXP_IP_UDP_TXBD_CNT + 2];
    u32                      total_buffers                 = 0;
    u32                      bd_count;

    u32                      buffer_addr;
    u32                      buffer_size;
    
    XAxiDma_BdRing         * dma_tx_ring_ptr               = NULL;
    XAxiDma_Bd             * bd_set_ptr                    = NULL;
    XAxiDma_Bd             * bd_ptr                        = NULL;
    
    // Check the Ethernet device
    if (eth_check_device(eth_dev_num) != XST_SUCCESS) {
        return WLAN_EXP_IP_UDP_FAILURE; 
    }
    
    // Try to disable the interrupts through the callback
    int_status = interrupt_disable_callback();
    
    // Get info from Ethernet device structure
    dma_tx_ring_ptr = (XAxiDma_BdRing *) eth_device[eth_dev_num].dma_tx_ring_ptr;

    // Create the array of IP/UDP buffers to be processed:
    //     - If the socket header is used, then put that as buffer[0], otherwise skip
    //     - Process the command arguments as normal
    //     - If the packet is not long enough, then append an additional buffer with zeros
    //
    if (use_socket_header == 1) {
        // Create a IP/UDP buffer for the header (only need to fill in data and size)
        socket_hdr.size         = sizeof(wlan_exp_ip_udp_header);
        socket_hdr.data         = (u8 *) socket->hdr;

        // Add the buffer to the array of buffers to be processed
        buffers_to_process[0]   = &socket_hdr;
        
        // Increment counters / indexes
        eth_frame_length       += sizeof(wlan_exp_ip_udp_header);
        total_buffers          += 1;
    }

    // Process the command line arguments
    for (i = 0; i < num_buffers; i++) {
        // Only add buffers that have a non-zero length
        if (buffers[i]->size > 0) {
            // Add the buffer to the array of buffers to be processed
            buffers_to_process[total_buffers] = buffers[i];

            // Increment counters / indexes
            eth_frame_length       += buffers[i]->size;
            total_buffers          += 1;
        }
    }

    // If the packet is less than the ETH_MIN_FRAME_LEN, then we need to pad the 
    // transaction with zeros.  To do this, we use the dummy minimum Ethernet frame
    // provided by the driver.
    //
    if (eth_frame_length < ETH_MIN_FRAME_LEN) {
        // Create a IP/UDP buffer for the padding (only need to fill in data and size)
        padding_buffer.size     = ETH_MIN_FRAME_LEN - eth_frame_length;
        padding_buffer.data     = ETH_dummy_frame;
        
        // Add the buffer to the array of buffers to be processed
        buffers_to_process[total_buffers] = &padding_buffer;
        
        // Increment counters / indexes
        eth_frame_length        = ETH_MIN_FRAME_LEN;
        total_buffers          += 1;
    }

    // 
    // NOTE:  At this point in the code, we have:
    //     - total_buffers is the number of buffers we need to process
    //     - buffers_to_process contains all the necessary pointers to IP/UDP buffers
    //

    // Sanity check:  Do we have enough buffer descriptors to transmit the frame
    if (total_buffers > WLAN_EXP_IP_UDP_TXBD_CNT) {
        eth_print_err_msg(eth_dev_num, WLAN_EXP_IP_UDP_ETH_ERROR_CODE, ETH_ERROR_CODE_TX_BD_CNT, &status, 1);
        
        // Re-enable the interrupts
        interrupt_enable_callback(int_status);
        
        return WLAN_EXP_IP_UDP_FAILURE;
    }

    // Check that there are currently no DMA errors (restarts DMA on error)
    eth_check_dma(eth_dev_num);
    
    //
    // NOTE:  In order to allow for the the maximum number of packets to be queued up waiting on
    //   the Ethernet module (ie to create the maximum buffer to allow for the CPU to be occupied
    //   by other things), we want to allocate as many buffer descriptors as possible and only
    //   free them when necessary or convenient.  Therefore, only if we cannot allocate buffer
    //   descriptors will we try to free the already processed buffer descriptors.
    //

    // Allocate buffer descriptors for the Frame    
    status = XAxiDma_BdRingAlloc(dma_tx_ring_ptr, total_buffers, &bd_set_ptr);

    // We cannot allocate buffer descriptors so we need to free up some
    //
    //     NOTE:  This loop is where the transport needs the function will block if all the descriptors
    //         are in use.  We do not want to exit the loop prematurely unless there is an unrecoverable
    //         error.  Otherwise, we will drop packets that need to be transmitted.
    //
    //     NOTE:  By keeping this loop simple, we avoid many potential race conditions that could arise
    //         between the processing code and the DMA peripheral.
    //
    while (status != XST_SUCCESS) {

        // Process any completed BDs
        bd_count = eth_process_tx_descriptors(eth_dev_num, dma_tx_ring_ptr);

        // Check that we processed the tx descriptors successfully
        if (bd_count == WLAN_EXP_IP_UDP_FAILURE) {
            eth_print_err_msg(eth_dev_num, WLAN_EXP_IP_UDP_ETH_ERROR_CODE, 0xDEADBEEF, &status, 1);  // !!! FIX !!!
            
            // Re-enable the interrupts
            interrupt_enable_callback(int_status);
        
            return WLAN_EXP_IP_UDP_FAILURE;

        } else {
            // Allocate buffer descriptors for the Frame
            status = XAxiDma_BdRingAlloc(dma_tx_ring_ptr, total_buffers, &bd_set_ptr);
        }
    }

    // Get the first descriptor in the set
    bd_ptr = bd_set_ptr;

    // Set up all of the buffer descriptors
    for (i = 0; i < total_buffers; i++) {

        // Clear the buffer descriptor
        XAxiDma_BdClear(bd_ptr);
    
        // Get the buffer address / size
        buffer_addr = (u32) buffers_to_process[i]->data;
        buffer_size = buffers_to_process[i]->size;
    
        // Set the descriptor address to the start of the buffer
        status = XAxiDma_BdSetBufAddr(bd_ptr, buffer_addr);
        
        if (status != XST_SUCCESS) {
            eth_print_err_msg(eth_dev_num, WLAN_EXP_IP_UDP_ETH_ERROR_CODE, ETH_ERROR_CODE_DMA_BD_SET_BUF_ADDR, &status, 1);
            
            // Re-enable the interrupts
            interrupt_enable_callback(int_status);
        
            return WLAN_EXP_IP_UDP_FAILURE;
        }

        // Set the descriptor length to the length of the buffer
        status = XAxiDma_BdSetLength(bd_ptr, buffer_size, dma_tx_ring_ptr->MaxTransferLen);
        
        if (status != XST_SUCCESS) {
            eth_print_err_msg(eth_dev_num, WLAN_EXP_IP_UDP_ETH_ERROR_CODE, ETH_ERROR_CODE_DMA_BD_SET_LENGTH, &status, 1);
            
            // Re-enable the interrupts
            interrupt_enable_callback(int_status);
        
            return WLAN_EXP_IP_UDP_FAILURE;
        }

        // Set the flags for first / last descriptor
        //    NOTE:  Since this is a "set" command, if there is only one buffer, then we need to set both bits in one write
        //
        if (i == 0) {
            if (total_buffers == 1) {
                XAxiDma_BdSetCtrl(bd_ptr, (XAXIDMA_BD_CTRL_TXSOF_MASK | XAXIDMA_BD_CTRL_TXEOF_MASK));
            } else {
                XAxiDma_BdSetCtrl(bd_ptr, XAXIDMA_BD_CTRL_TXSOF_MASK);
            }
        } else if (i == (total_buffers - 1)) {

            XAxiDma_BdSetCtrl(bd_ptr, XAXIDMA_BD_CTRL_TXEOF_MASK);
        }

        // Get next descriptor
        bd_ptr = (XAxiDma_Bd*)XAxiDma_BdRingNext(dma_tx_ring_ptr, bd_ptr);
    }

    // Enqueue to the HW
    status = XAxiDma_BdRingToHw(dma_tx_ring_ptr, total_buffers, bd_set_ptr);
    
    if (status != XST_SUCCESS) {
        // Undo descriptor allocation and exit
        XAxiDma_BdRingUnAlloc(dma_tx_ring_ptr, total_buffers, bd_set_ptr);
        
        eth_print_err_msg(eth_dev_num, WLAN_EXP_IP_UDP_ETH_ERROR_CODE, ETH_ERROR_CODE_DMA_TX_BD_RING_TO_HW, &status, 1);
        
        // Re-enable the interrupts
        interrupt_enable_callback(int_status);
        
        return WLAN_EXP_IP_UDP_FAILURE;
    }

    // Start the transmission, if necessary
    //     NOTE:  If the channel is already started, then the XAxiDma_BdRingToHw() call will start the transmission
    if (dma_tx_ring_ptr->RunState == AXIDMA_CHANNEL_HALTED) {
    
        status = XAxiDma_BdRingStart(dma_tx_ring_ptr);
        
        if (status != XST_SUCCESS) {
            eth_print_err_msg(eth_dev_num, WLAN_EXP_IP_UDP_ETH_ERROR_CODE, ETH_ERROR_CODE_DMA_TX_BD_RING_START, &status, 1);
            
            // Re-enable the interrupts
            interrupt_enable_callback(int_status);
            
            return WLAN_EXP_IP_UDP_FAILURE;
        }
    }

    //
    // Check if the HW is done processing some buffer descriptors
    //
    //   NOTE:  Since it takes some time to begin the Ethernet transfer (see NOTE below), it is convenient to try
    //       to process completed buffer descriptors here.  Unfortunately, it requires a non-trivial amount of time
    //       to even check if there are buffer descriptors to process due to interactions with the hardware.  Hence,
    //       there is a tradeoff between calling eth_process_tx_descriptors() in each iteration of this loop vs
    //       waiting until there are more buffer descriptors to process since the incremental cost of processing
    //       a buffer descriptor is much less than checking if there are buffer descriptors to process.  The down side
    //       of waiting to process more descriptors is that this makes the time it takes to process the packet to
    //       send less consistent.  Currently, we are opting for timing consistency and eating the overhead required
    //       to check that descriptors are ready to process.  However, this decision should be revisited in future
    //       revisions to this library.
    //
    //   NOTE:  The Ethernet controller requires that all data to be sent in a given packet be located in the
    //       Ethernet controller local memory.  Therefore, the AXI DMA attached to the Ethernet controller must
    //       transfer all necessary data to the Ethernet controller before the Ethernet transfer can begin.
    //       Unfortunately, the time of this transfer is bounded by the AXI stream channel between the AXI DMA and
    //       Ethernet controller which is only 32 bits @ 160 MHz.  As of WARPLab 7.5.1, the Ethernet controller and
    //       AXI DMA did not allow the AXI stream interface to be configured so 640 MBps is the maximum throughput
    //       attainable through that link.
    //
    //   NOTE:  Based on empirical measurements, here is the rough timing for processing tx descriptors.  In this
    //       experiment, we were using Read IQ, which requires 2 buffer descriptors per Ethernet packet, and measuring
    //       timing using debug GPIO calls (WLAN_EXP_IP_UDP_TXBD_CNT = 10):
    //
    //       HwCnt > X     Time to process    Num Loops            Avg Time per Loop
    //         0             4   us              1                      4    us
    //         4             6.4 us              2                      3.2  us
    //         6             8.8 us              3                      2.9  us
    //         8            11.2 us              4                      2.8  us
    //
    //       For a full packet, there is approximately 14.5 us between the start of the DMA and the start of the
    //       Ethernet packet.  If we decide to change the decision to wait to process buffer descriptors, then
    //       uncomment the following line and choose the appropriate value:
    //
    // if (dma_tx_ring_ptr->HwCnt > 4) {

        // Process any completed BDs
        bd_count = eth_process_tx_descriptors(eth_dev_num, dma_tx_ring_ptr);

        // Check that we processed the tx descriptors successfully
        if (bd_count == WLAN_EXP_IP_UDP_FAILURE) {
            eth_print_err_msg(eth_dev_num, WLAN_EXP_IP_UDP_ETH_ERROR_CODE, 0xDEADBEEF, &status, 1);  // !!! FIX !!!
            
            // Re-enable the interrupts
            interrupt_enable_callback(int_status);
            
            return WLAN_EXP_IP_UDP_FAILURE;
        }
    //}

    // Re-enable the interrupts
    interrupt_enable_callback(int_status);
    
    return eth_frame_length;
}



/*****************************************************************************/
/**
 * Process TX buffer descriptors
 *
 * @param   dma_tx_ring_ptr   - Pointer to DMA Tx BD ring
 *
 * @return  int               - Number of descriptors processed
 *                                 WLAN_EXP_IP_UDP_FAILURE - There was an error in the command
 *
 *****************************************************************************/
int eth_process_tx_descriptors(u32 eth_dev_num, XAxiDma_BdRing * dma_tx_ring_ptr) {

    int                      i;
       int                      status;

    int                      processed_bd_count;

    XAxiDma_Bd             * bd_set_ptr                    = NULL;
    XAxiDma_Bd             * bd_ptr                        = NULL;

    // Check how many buffer descriptors have been processed
    processed_bd_count = XAxiDma_BdRingFromHw(dma_tx_ring_ptr, WLAN_EXP_IP_UDP_TXBD_CNT, &bd_set_ptr);

    // Initialize the working descriptor pointer
    bd_ptr = bd_set_ptr;

    for (i = 0; i < processed_bd_count; i++) {

        // Get the status of the descriptor
        status = XAxiDma_BdGetSts(bd_ptr);

        // Check the status
        if ((status & XAXIDMA_BD_STS_ALL_ERR_MASK) || (!(status & XAXIDMA_BD_STS_COMPLETE_MASK))) {
            eth_print_err_msg(eth_dev_num, WLAN_EXP_IP_UDP_ETH_ERROR_CODE, ETH_ERROR_CODE_DMA_TX_ERROR, &status, 1);
        }

        // NOTE:  No need to clear out all the control / status information before freeing the buffer
        //     descriptor, since we clear them when they are allocated.

        // Get next descriptor
        bd_ptr = (XAxiDma_Bd*)XAxiDma_BdRingNext(dma_tx_ring_ptr, bd_ptr);
    }

    // Free all processed RX BDs for future transmission
    status = XAxiDma_BdRingFree(dma_tx_ring_ptr, processed_bd_count, bd_set_ptr);

    if (status != XST_SUCCESS) {
        eth_print_err_msg(eth_dev_num, WLAN_EXP_IP_UDP_ETH_ERROR_CODE, ETH_ERROR_CODE_DMA_TX_BD_RING_FREE, &status, 1);
        return WLAN_EXP_IP_UDP_FAILURE;
    }

    return processed_bd_count;
}



/*****************************************************************************/
/**
 * Receive an Ethernet Frame
 * 
 * Will try to receive an Ethernet frame and perform initial IP / UDP processing on it.
 * This function is non-blocking and will have populated the wlan_exp_ip_udp_buffer if 
 * the return value is greater than 0 (ie > 0).
 *
 * @param   eth_dev_num - Ethernet device to receive Ethernet frame on
 * @param   eth_frame   - Mango wlan_exp IP/UDP Buffer to populate with Ethernet data
 *
 * @return  int         - Number of bytes of data in the received packet
 *                            0 if there is no packet received
 *                            WLAN_EXP_IP_UDP_FAILURE if there was a library failure
 *
 ******************************************************************************/
int eth_recv_frame(u32 eth_dev_num, wlan_exp_ip_udp_buffer * eth_frame) {

    int                      status;

    int                      processed_bd_count  = 0;
    int                      size                = 0;
    int                      length              = 0;

    XAxiDma_BdRing         * dma_rx_ring_ptr;
    XAxiDma_Bd             * bd_ptr;
    ethernet_header        * header;
    u8                     * data;

    u32                      node_addr_match;
    u32                      bcast_addr_match;
    
    u32                    * temp_ptr_0;
    u32                    * temp_ptr_1;
    u32                      temp_32_0;
    u32                      temp_32_1;
    u16                      temp_16_0;
    u16                      temp_16_1;


    // Check the Ethernet device (optional - this is checked in many other places; removing for performance reasons)
    // if (eth_check_device(eth_dev_num) != XST_SUCCESS) { return WLAN_EXP_IP_UDP_FAILURE; }

    // Get the RX Buffer Descriptor Ring pointer
    dma_rx_ring_ptr = (XAxiDma_BdRing *)(eth_device[eth_dev_num].dma_rx_ring_ptr);

    // Check to see that the HW is started
    //     - If it is not started, we must have gotten an error somewhere, so we need to reset and restart the DMA
    //
    if (!XAxiDma_BdRingHwIsStarted(dma_rx_ring_ptr)) {

        // Check that there are no DMA errors (restarts DMA on error)
        eth_check_dma(eth_dev_num);
    
        // Start DMA RX channel
        status = XAxiDma_BdRingStart(dma_rx_ring_ptr);
        
        if (status != XST_SUCCESS) {
            eth_print_err_msg(eth_dev_num, WLAN_EXP_IP_UDP_ETH_ERROR_CODE, ETH_ERROR_CODE_DMA_RX_BD_RING_START, &status, 1);
            return WLAN_EXP_IP_UDP_FAILURE;
        }
    }
    
    // See if we have any data to process
    //     NOTE:  We will only process one buffer descriptor at a time in this function call
    processed_bd_count = XAxiDma_BdRingFromHw(dma_rx_ring_ptr, 0x1, &bd_ptr);

    // If we have data, then we need process the buffer
    if (processed_bd_count > 0) {

        // Get the status of the buffer descriptor
        status = XAxiDma_BdGetSts(bd_ptr);

        if ((status & XAXIDMA_BD_STS_ALL_ERR_MASK) || (!(status & XAXIDMA_BD_STS_COMPLETE_MASK))) {
            eth_print_err_msg(eth_dev_num, WLAN_EXP_IP_UDP_ETH_ERROR_CODE, ETH_ERROR_CODE_DMA_RX_ERROR, &status, 1);
            return WLAN_EXP_IP_UDP_FAILURE;
        } else {
            size = (XAxiDma_BdRead(bd_ptr, XAXIDMA_BD_USR4_OFFSET)) & 0x0000FFFF;
        }

        // Get the address of the buffer data
        data = (u8 *)XAxiDma_BdGetId(bd_ptr);                        // BD ID was set to the base address of the buffer
        
        // Strip off the FCS (or CRC) from the received frame
        size -= 4;
        
        // Populate the IP/UDP buffer
        eth_frame->max_size   = WLAN_EXP_IP_UDP_ETH_BUF_SIZE;            // Max buffer size (Should not be changed for received packets)
        eth_frame->size       = size;                                // Current buffer size (Should not be changed for received packets)
        eth_frame->data       = data;                                // Address of buffer data
        eth_frame->offset     = data + ETH_HEADER_LEN;               // Initialize offset after Ethernet header
        eth_frame->length     = size - ETH_HEADER_LEN;               // Initialize the length after the Ethernet header
        eth_frame->descriptor = (void *) bd_ptr;                     // Populate the descriptor pointer to be freed later

        // Process the packet
        if ( size > 0 ) {

            // Get a pointer to the Ethernet header
            header           = (ethernet_header *) data;

            // Check Ethernet header to see if packet is destined for the node
            //     NOTE:  The code below implements the same function as:
            //
            //         node_addr_match  = ~(memcmp(header->dest_mac_addr, eth_device[eth_dev_num].hw_addr, ETH_ADDR_LEN));
            //         bcast_addr_match = ~(memcmp(header->dest_mac_addr, eth_bcast_addr, ETH_ADDR_LEN));
            //
            //     However, it is optimized for the fewest number of system reads since those take a
            //     significant amount of time (time difference is ~0.9 us).
            //
            temp_ptr_0 = (u32 *) header->dest_mac_addr;
            temp_32_0  = temp_ptr_0[0];
            temp_16_0  = temp_ptr_0[1] & 0x0000FFFF;

            temp_ptr_1 = (u32 *) eth_device[eth_dev_num].hw_addr;
            temp_32_1  = temp_ptr_1[0];
            temp_16_1  = temp_ptr_1[1] & 0x0000FFFF;

            node_addr_match  = ((temp_32_0 == temp_32_1) & (temp_16_0 == temp_16_1));
            bcast_addr_match = ((temp_32_0 == 0xFFFFFFFF) & (temp_16_0 == 0xFFFF));

            // Process the packet based on the EtherType
            if (node_addr_match | bcast_addr_match) {
                switch (Xil_Ntohs(header->ethertype)) {
                    // IP packet
                    case ETHERTYPE_IP_V4:
                        length = ipv4_process_packet(eth_dev_num, eth_frame);
                    break;

                    // ARP packet
                    case ETHERTYPE_ARP:
                        length = arp_process_packet(eth_dev_num, eth_frame);
                    break;

                    // NOTE:  The library does not include a default case because there are a number of
                    //     Ethernet packets that the node will receive that the library will not process.
                    //     Since the library doesn't know at this point if the packet is destined for the
                    //     given node, having an error message here would be a distraction.
                    //
                }

                // Need to adjust the buffer for the library delimiter to 32-bit align the buffer data
                //
                // ASSUMPTION:
                //     The buffer descriptor used for this packet will be freed when it is done being processed.
                //
                if (length > 0) {
                    eth_frame->offset += WLAN_EXP_IP_UDP_DELIM_LEN;
                    eth_frame->length -= WLAN_EXP_IP_UDP_DELIM_LEN;
                    length            -= WLAN_EXP_IP_UDP_DELIM_LEN;
                } else {
                    // Library is done with the Ethernet frame, need to free buffer descriptor
                    eth_free_recv_buffers(eth_dev_num, eth_frame->descriptor, 0x1);
                }

            } else {
                // Ethernet frame not intended for node, need to free buffer descriptor
                eth_free_recv_buffers(eth_dev_num, eth_frame->descriptor, 0x1);
            }
        }
    }
    // }

    return length;
}



/*****************************************************************************/
/**
 * Free receive buffer so they can be used again
 *
 * @param   eth_dev_num      - Ethernet device number
 * @param   descriptors      - Pointer to receive buffer descriptor to be freed
 * @param   num_descriptors  - Number of buffers to be freed
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS - Command completed successfully
 *                                 XST_FAILURE - There was an error in the command
 *
 *****************************************************************************/
int eth_free_recv_buffers(u32 eth_dev_num, void * descriptors, u32 num_descriptors) {

    int                      status;
    u32                      free_bd_cnt;

    XAxiDma_BdRing         * dma_rx_ring_ptr;
    XAxiDma_Bd             * bd_ptr;
 
    // Check the Ethernet device
    if (eth_check_device(eth_dev_num) == XST_FAILURE) { return XST_FAILURE; }

    // Get DMA info from Ethernet device structure
    dma_rx_ring_ptr   = (XAxiDma_BdRing *) eth_device[eth_dev_num].dma_rx_ring_ptr;
    
    // Cast the buffer descriptor pointer
    bd_ptr = (XAxiDma_Bd *)(descriptors);

    // Free processed RX descriptors for future receptions
    status = XAxiDma_BdRingFree(dma_rx_ring_ptr, num_descriptors, bd_ptr);

    if (status != XST_SUCCESS) {
        eth_print_err_msg(eth_dev_num, WLAN_EXP_IP_UDP_ETH_ERROR_CODE, ETH_ERROR_CODE_DMA_RX_BD_RING_FREE, &status, 1);
        return XST_FAILURE;
    }

    // Return processed descriptors to RX channel so they are ready to receive new packets
    free_bd_cnt = XAxiDma_BdRingGetFreeCnt(dma_rx_ring_ptr);

    status = XAxiDma_BdRingAlloc(dma_rx_ring_ptr, free_bd_cnt, &bd_ptr);

    if (status != XST_SUCCESS) {
        eth_print_err_msg(eth_dev_num, WLAN_EXP_IP_UDP_ETH_ERROR_CODE, ETH_ERROR_CODE_DMA_RX_BD_RING_ALLOC, &status, 1);
        return XST_FAILURE;
    }

    status = XAxiDma_BdRingToHw(dma_rx_ring_ptr, free_bd_cnt, bd_ptr);

    if (status != XST_SUCCESS) {
        eth_print_err_msg(eth_dev_num, WLAN_EXP_IP_UDP_ETH_ERROR_CODE, ETH_ERROR_CODE_DMA_RX_BD_RING_TO_HW, &status, 1);
        return XST_FAILURE;
    }

    return XST_SUCCESS;
}



/*****************************************************************************/
/**
 * Read the contents of the Ethernet Phy
 *
 * @param   eth_dev_num      - Ethernet device number
 * @param   phy_addr         - Address of the PHY to read
 * @param   reg_addr         - Address of register within the PHY to read
 * @param   reg_value        - Pointer to where data will be returned
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS - Command completed successfully
 *                                 XST_FAILURE - There was an error in the command
 *
 *****************************************************************************/
int eth_read_phy_reg(u32 eth_dev_num, u32 phy_addr, u32 reg_addr, u16 * reg_value) {

    // Check the Ethernet device
    if (eth_check_device(eth_dev_num) == XST_FAILURE) {
        return XST_FAILURE;
    }

    // Check if the Ethernet PHY reports a valid link
    XAxiEthernet_PhyRead(eth_device[eth_dev_num].eth_ptr, phy_addr, reg_addr, reg_value);

    return XST_SUCCESS;
}



/*****************************************************************************/
/**
 * Write a value to the Ethernet Phy
 *
 * @param   eth_dev_num      - Ethernet device number
 * @param   phy_addr         - Address of the PHY to write
 * @param   reg_addr         - Address of register within the PHY to write
 * @param   reg_value        - u16 value to write
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS - Command completed successfully
 *                                 XST_FAILURE - There was an error in the command
 *
 *****************************************************************************/
int eth_write_phy_reg(u32 eth_dev_num, u32 phy_addr, u32 reg_addr, u16 reg_value) {

    // Check the Ethernet device
    if (eth_check_device(eth_dev_num) == XST_FAILURE) {
        return XST_FAILURE;
    }

    // Check if the Ethernet PHY reports a valid link
    XAxiEthernet_PhyWrite(eth_device[eth_dev_num].eth_ptr, phy_addr, reg_addr, reg_value);

    return XST_SUCCESS;
}



/*****************************************************************************/
/**
 * Set the operating speed of the Ethernet device
 *
 * @param   eth_dev_num      - Ethernet device number
 * @param   speed            - Speed of the device in Mbps
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS - Command completed successfully
 *                                 XST_FAILURE - There was an error in the command
 *
 *****************************************************************************/
int eth_set_mac_operating_speed(u32 eth_dev_num, u32 speed) {
    
    // Check the Ethernet device
    if (eth_check_device(eth_dev_num) == XST_FAILURE) {
        return XST_FAILURE;
    }

    return XAxiEthernet_SetOperatingSpeed(eth_device[eth_dev_num].eth_ptr, speed);
}



/*****************************************************************************/
/**
 * Set / Get MAC Address
 *
 * @param   eth_dev_num      - Ethernet device number
 * @param   hw_addr          - u8 pointer of MAC address to set / get
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS - Command completed successfully
 *                                 XST_FAILURE - There was an error in the command
 *
 ******************************************************************************/
int eth_set_hw_addr(u32 eth_dev_num, u8 * hw_addr) {

    int i;

    // Check the Ethernet device
    if (eth_check_device(eth_dev_num) == XST_FAILURE) {
        return XST_FAILURE;
    }
    
    // Update the MAC Address
    for (i = 0; i < ETH_ADDR_LEN; i++) {
        eth_device[eth_dev_num].hw_addr[i] = hw_addr[i];
    }

    return XST_SUCCESS;
}



int eth_get_hw_addr(u32 eth_dev_num, u8 * hw_addr) {

    int i;

    // Check the Ethernet device
    if (eth_check_device(eth_dev_num) == XST_FAILURE) {
        return XST_FAILURE;
    }
    
    // Populate the MAC Address
    for ( i = 0; i < ETH_ADDR_LEN; i++ ) {
        hw_addr[i] = eth_device[eth_dev_num].hw_addr[i];
    }

    return XST_SUCCESS;
}



/*****************************************************************************/
/**
 * Set / Get IP Address
 *
 * @param   eth_dev_num      - Ethernet device number
 * @param   ip_addr          - u8 pointer of IP address to set / get
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS - Command completed successfully
 *                                 XST_FAILURE - There was an error in the command
 *
 ******************************************************************************/
int eth_set_ip_addr(u32 eth_dev_num, u8 * ip_addr) {

    int i;
    
    // Check the Ethernet device
    if (eth_check_device(eth_dev_num) == XST_FAILURE) {
        return XST_FAILURE;
    }
    
    // Update the IP Address
    for (i = 0; i < IP_ADDR_LEN; i++) {
        eth_device[eth_dev_num].ip_addr[i] = ip_addr[i];
    }

    return XST_SUCCESS;
}
    


int eth_get_ip_addr(u32 eth_dev_num, u8 * ip_addr) {

    int i;

    // Check the Ethernet device
    if (eth_check_device(eth_dev_num) == XST_FAILURE) {
        return XST_FAILURE;
    }
    
    // Populate the IP Address
    for ( i = 0; i < IP_ADDR_LEN; i++) {
        ip_addr[i] = eth_device[eth_dev_num].ip_addr[i];
    }

    return XST_SUCCESS;
}



/*****************************************************************************/
/**
 * Get Internal Constants
 *
 * NOTE:  Can expand this section if more constants in wlan_exp_ip_udp_config.h and
 *     wlan_exp_ip_udp_device.h are needed outside the transport.
 *
 * @param   None
 *
 * @return  int              - Value of the constant
 *
 ******************************************************************************/
int eth_get_num_tx_descriptors() {

    return WLAN_EXP_IP_UDP_TXBD_CNT;
}



/*****************************************************************************/
/**
 * Initialize the Ethernet Header
 *
 * @param   header           - Pointer to the Ethernet header
 * @param   src_hw_addr      - Source MAC address for Ethernet packet
 *
 * @return  None
 *
 ******************************************************************************/
void eth_init_header(ethernet_header * header, u8 * src_hw_addr) {

    u32 i;

    // Update the following static fields for a socket:
    //   - Source MAC address
    //
    if (src_hw_addr != NULL) { 
        for (i = 0; i < ETH_ADDR_LEN; i++) {   
            header->src_mac_addr[i] = src_hw_addr[i];
        }
    }
}



/*****************************************************************************/
/**
 * Update the Ethernet Header
 *
 * @param   header           - Pointer to the Ethernet header
 * @param   dest_hw_addr     - Destination MAC address for Ethernet packet (big-endian)
 * @param   ethertype        - EtherType of the Ethernet packet (little-endian)
 *
 * @return  None
 *
 ******************************************************************************/
void eth_update_header(ethernet_header * header, u8 * dest_hw_addr, u16 ethertype) {

    u32 i;

    // Update the following fields:
    //   - Destination MAC address
    //   - EtherType
    //
    // NOTE:  We do not need to update the following fields because they are static for the socket:
    //   - Source MAC address
    //
    if (dest_hw_addr != NULL) { 
        for (i = 0; i < ETH_ADDR_LEN; i++) {   
            header->dest_mac_addr[i] = dest_hw_addr[i];
        }
    }

    header->ethertype = Xil_Htons(ethertype);
}






/**********************************************************************************************************************/
/**
 * @brief Internal Methods
 *
 **********************************************************************************************************************/
 
 
/*****************************************************************************/
/**
 * Check the Ethernet device
 *
 * @param   eth_dev_num      - Ethernet device number
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS - Command completed successfully
 *                                 XST_FAILURE - There was an error in the command
 *
 ******************************************************************************/
int eth_check_device(u32 eth_dev_num) {

    // Check to see if we are sending on a valid interface
    if (eth_dev_num >= WLAN_EXP_IP_UDP_NUM_ETH_DEVICES) {
        eth_print_err_msg(eth_dev_num, WLAN_EXP_IP_UDP_ETH_ERROR_NUM_DEV, 0, NULL, 0);
        return XST_FAILURE;
    }
    
    // Check if Ethernet device has been initialized
    if (!eth_device[eth_dev_num].initialized) {
        eth_print_err_msg(eth_dev_num, WLAN_EXP_IP_UDP_ETH_ERROR_INTIALIZED, 0, NULL, 0);
        return XST_FAILURE;
    }
    
    return XST_SUCCESS;
}



/*****************************************************************************/
/**
 * Check the Ethernet DMA for TX / RX error
 *
 * @param   eth_dev_num      - Ethernet device number
 *
 * @return  None
 *
 * @note    If there is a DMA error, then this function will reset the DMA.
 *
 ******************************************************************************/
void eth_check_dma(u32 eth_dev_num) {

    XAxiDma_BdRing    * rx_ring_ptr;
    XAxiDma_BdRing    * tx_ring_ptr;
    u32                 rx_dma_error   = 0;
    u32                 tx_dma_error   = 0;

    // Get the RX Buffer Descriptor Ring pointer
    rx_ring_ptr = (XAxiDma_BdRing *)eth_device[eth_dev_num].dma_rx_ring_ptr;

    // Get the TX Buffer Descriptor Ring pointer
    tx_ring_ptr = (XAxiDma_BdRing *)eth_device[eth_dev_num].dma_tx_ring_ptr;
    
    // Check to see if we have a RX error
    rx_dma_error = XAxiDma_BdRingGetError(rx_ring_ptr);
    
    if (rx_dma_error) {
        eth_print_err_msg(eth_dev_num, WLAN_EXP_IP_UDP_ETH_ERROR_CODE, ETH_ERROR_CODE_DMA_RX_ERROR, &rx_dma_error, 1);
    }
    
    // Check to see if we have a TX error
    tx_dma_error = XAxiDma_BdRingGetError(tx_ring_ptr);
    
    if (tx_dma_error) {
        eth_print_err_msg(eth_dev_num, WLAN_EXP_IP_UDP_ETH_ERROR_CODE, ETH_ERROR_CODE_DMA_TX_ERROR, &tx_dma_error, 1);
    }
        
    // If there is an error, reset the DMA
    if (rx_dma_error || tx_dma_error) {
        xil_printf("\n!!! Resetting the DMA !!!\n\n");
        XAxiDma_Reset((XAxiDma *)(eth_device[eth_dev_num].dma_ptr));
    }
}



/*****************************************************************************/
/**
 * Consolidate error messages to reduce code overhead
 *
 * @param   eth_dev_num      - Ethernet device number
 * @param   msg_num          - Message number
 * @param   data             - Pointer to message data
 * @param   data_length      - Number of elements in data
 *
 * @return  None
 *
 ******************************************************************************/
void eth_print_err_msg(u32 eth_dev_num, u32 msg_num, u32 error_code, void * data, u32 data_length) {

    u32 i;

    xil_printf("ERROR in Ethernet %c:\n", wlan_exp_conv_eth_dev_num(eth_dev_num));

    switch(msg_num) {
        case WLAN_EXP_IP_UDP_ETH_ERROR_NUM_DEV:
            xil_printf("    Ethernet device number out of range:  %d\n", eth_dev_num);
            xil_printf("    Currently, there are %d supported Ethernet devices.\n", WLAN_EXP_IP_UDP_NUM_ETH_DEVICES);
        break;
        
        case WLAN_EXP_IP_UDP_ETH_ERROR_INTIALIZED:
            xil_printf("    Mango wlan_exp IP/UDP Library not configured to use Ethernet device.\n");
            xil_printf("    Please check library configuration in the BSP.\n" );
        break;

        case WLAN_EXP_IP_UDP_ETH_ERROR_CODE:
            xil_printf("    Mango wlan_exp IP/UDP transport error:  0x%08x\n", error_code);
            xil_printf("    See documentation for more information.\n");
        break;
    }

    if (data_length > 0) {
        for (i = 0; i < data_length; i++) {
            xil_printf("        0x%08x\n", ((u32 *)data)[i]);
        }
    }
    
    xil_printf("\n");
}




/*****************************************************************************/
/**
 * Debug print functions
 *
 ******************************************************************************/

#ifdef _DEBUG_

void print_pkt(u8 * buf, int size) {

    int i;

    xil_printf("Ethernet Packet: (0x%x bytes)\n", size);

    for (i = 0; i < size; i++) {
        xil_printf("%2x ", buf[i]);
        if ((((i + 1) % 16) == 0) && ((i + 1) != size)) {
            xil_printf("\n");
        }
    }
    xil_printf("\n\n");
}


void print_XAxiDma_Bd(XAxiDma_Bd * bd_ptr) {

    int i;

    xil_printf("Buffer Descriptor: 0x%x\n", bd_ptr);
    
    for ( i = 0; i < XAXIDMA_BD_NUM_WORDS; i++ ) {
        xil_printf("  Value[%2d]:        0x%x \n", i, (*bd_ptr)[i]);
    }
    xil_printf("\n");
}

#endif

