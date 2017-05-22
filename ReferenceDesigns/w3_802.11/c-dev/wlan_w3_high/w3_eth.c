#include "wlan_mac_high_sw_config.h"

#if WLAN_SW_CONFIG_ENABLE_ETH_BRIDGE
#include "wlan_platform_high.h"
#include "include/w3_high.h"
#include "include/w3_eth.h"
#include "xparameters.h"

#include "stdlib.h"
#include "xaxiethernet.h"
#include "xaxidma.h"
#include "xintc.h"
#include "stddef.h"
#include "wlan_platform_common.h"
#include "wlan_platform_high.h"
#include "wlan_mac_common.h"
#include "wlan_mac_dl_list.h"
#include "wlan_mac_high.h"
#include "wlan_mac_eth_util.h"
#include "wlan_mac_schedule.h"
#include "wlan_mac_queue.h"
#include "wlan_mac_802_11_defs.h"

// Performance monitoring using SW GPIO
//     - Each of these defines if 1 will enable performance monitoring using the
//       wlan_mac_set_dbg_hdr_out() / wlan_mac_clear_dbg_hdr_out() functions.
//     - All of the times are approximate because GPIO functions take time to
//       execute and might increase the time you are trying to measure.
// FIXME: These need to be updated to use wlan_platform
#define PERF_MON_ETH_PROCESS_RX                            0
#define PERF_MON_ETH_PROCESS_ALL_RX                        0
#define PERF_MON_ETH_UPDATE_DMA                            0

// Performance monitoring BD usage
//     - Code enabled by the define will water-mark the BD usage of the Ethernet
//       driver.
//
#define PERF_MON_ETH_BD                                    0

// Instance structure for Ethernet DMA
static XAxiDma               eth_dma_instance;

// Number of TX / RX buffer descriptors
static u32                   num_rx_bd;
static u32                   num_tx_bd;

// Global representing the schedule ID and pointer
static u32					 rx_schedule_id;
static dl_entry* 			 rx_schedule_dl_entry;

// Ethernet packet processing variables
//
//   In order to have a 1-to-1 interrupt assertion to ISR execution, when the Ethernet ISR
// runs, we have to disable the interrupt, acknowledge the interrupt and then collect all
// available packets to be processed.  Given that we cannot block the system for the time
// it takes to process all packets, we will schedule an event that will process the packets
// as quickly as possible using the scheduler.

static XAxiDma_Bd          * bd_set_to_process_ptr;
static int                   bd_set_count;
#define						 MAX_PACKETS_ENQUEUED 2
#define						 MAX_PACKETS_TOTAL 10
static u32                   irq_status;

static u32					gl_eth_tx_bd_mem_base;
static u32					gl_eth_tx_bd_mem_size;
static u32					gl_eth_tx_bd_mem_high;
static u32					gl_eth_rx_bd_mem_base;
static u32					gl_eth_rx_bd_mem_size;
static u32					gl_eth_rx_bd_mem_high;

#if PERF_MON_ETH_BD
static u32                   bd_high_water_mark;
#endif

// Local Function Declarations -- these are not intended to be called by functions outside of this file
int      _wlan_eth_dma_init();
int      _init_rx_bd(XAxiDma_Bd * bd_ptr, dl_entry * tqe_ptr, u32 max_transfer_len);
void     _wlan_process_all_eth_pkts(u32 schedule_id);
void 	 _eth_rx_interrupt_handler(void *callbarck_arg);
void 	 _wlan_eth_dma_update();


/*****************************************************************************/
/**
 * @brief Transmits a packet over Ethernet using the ETH DMA
 *
 * This function transmits a single packet via Ethernet using the axi_dma. The
 * packet passed via pkt_ptr must be a valid Ethernet packet, complete with
 * 14-byte Ethernet header. This function does not check for a valid header
 * (i.e. the calling function must ensure this).
 *
 * The packet must be stored in a memory location accessible by the axi_dma core.
 * The MicroBlaze DLMB is *not* accessible to the DMA. Thus packets cannot be
 * stored in malloc'd areas (the heap is in the DLMB).  In the reference implementation
 * all Ethernet transmissions start as wireless receptions. Thus, the Ethernet payloads
 * are stored in the wireless Rx packet buffer, which is accessible by the DMA.
 *
 * Custom code which needs to send Ethernet packets can use a spare wireless Tx/Rx
 * packet buffer, a spare Tx queue entry in DRAM or the user scratch space in DRAM
 * to create custom Ethernet payloads.
 *
 * This function blocks until the Ethernet transmission completes.
 *
 * @param u8* pkt_ptr
 *  - Pointer to the first byte of the Ethernet header of the packet to send; valid
 *    header must be created before calling this function
 * @param u32 length
 *  - Length (in bytes) of the packet to send
 *
 * @return 0 for successful Ethernet transmission, -1 otherwise
 */
int wlan_platform_ethernet_send(u8* pkt_ptr, u32 length) {
    int                 status;
    XAxiDma_BdRing    * tx_ring_ptr;
    XAxiDma_Bd        * cur_bd_ptr = NULL;

    if ((length == 0) || (length > 1518)) {
        xil_printf("ERROR: wlan_eth_dma_send length = %d\n", length);
        return -1;
    }

    // IMPORTANT:  If the data cache is enabled the cache must be flushed before
    // attempting to send a packet via the ETH DMA. The DMA will read packet
    // contents directly from RAM, bypassing any cache checking normally done by
    // the MicroBlaze.  The data cache is disabled by default in the reference
    // implementation.
    //
    // Xil_DCacheFlushRange((u32)TxPacket, MAX_PKT_LEN);

    // Check if the user-supplied pointer is in the DLMB, unreachable by the DMA
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
	// The -Wtype-limits flag catches the below checks when any of the base addresses are 0 since the
	// evaluation will always be true. Nevertheless, these checks are useful if the memory map changes.
	// So, we can explicitly disable the warning around these sets of lines.
    if (((u32)pkt_ptr >= DLMB_BASEADDR && (u32)pkt_ptr <= DLMB_HIGHADDR)) {
        xil_printf("ERROR: Eth DMA send -- DLMB source address (0x%08x) not reachable by DMA\n", pkt_ptr);
        return -1;
    }
#pragma GCC diagnostic pop

    // Get pointer to the axi_dma Tx buffer descriptor ring
    tx_ring_ptr = XAxiDma_GetTxRing(&eth_dma_instance);

    // Allocate and setup one Tx BD
    status  = XAxiDma_BdRingAlloc(tx_ring_ptr, 1, &cur_bd_ptr);
    status |= XAxiDma_BdSetBufAddr(cur_bd_ptr, (u32)pkt_ptr);
    status |= XAxiDma_BdSetLength(cur_bd_ptr, length, tx_ring_ptr->MaxTransferLen);

    if(status != XST_SUCCESS) {
        xil_printf("length = %d, max_transfer_len = %d\n", length, tx_ring_ptr->MaxTransferLen );
        xil_printf("ERROR setting ETH TX BD! Err = %d\n", status);
        status = XAxiDma_BdRingFree(tx_ring_ptr, 1, cur_bd_ptr);
        return -1;
    }

    // When using 1 BD for 1 pkt set both start-of-frame (SOF) and end-of-frame (EOF)
    XAxiDma_BdSetCtrl(cur_bd_ptr, (XAXIDMA_BD_CTRL_TXSOF_MASK | XAXIDMA_BD_CTRL_TXEOF_MASK));

    // Push the BD ring to hardware; this initiates the actual DMA transfer and Ethernet Tx
    status = XAxiDma_BdRingToHw(tx_ring_ptr, 1, cur_bd_ptr);
    if(status != XST_SUCCESS){
        xil_printf("ERROR: TX XAxiDma_BdRingToHw! Err = %d\n", status);
        status = XAxiDma_BdRingFree(tx_ring_ptr, 1, cur_bd_ptr);
        return -1;
    }

    // Wait for this DMA transfer to finish
    //     !!! TODO: replace with post-Tx ISR !!!
    while (XAxiDma_BdRingFromHw(tx_ring_ptr, 1, &cur_bd_ptr) == 0) { /*Do Nothing*/ }

    // Free the BD for future use
    status = XAxiDma_BdRingFree(tx_ring_ptr, 1, cur_bd_ptr);
    if(status != XST_SUCCESS) {xil_printf("ERROR: TX XAxiDma_BdRingFree! Err = %d\n", status); return -1;}

    return 0;
}

int w3_wlan_platform_ethernet_init(){
	int status = 0;

	XAxiEthernet_Config    * eth_cfg_ptr;
	XAxiEthernet             eth_instance;

	// Set global variables
	bd_set_to_process_ptr  = NULL;
	bd_set_count           = 0;

	rx_schedule_id = SCHEDULE_ID_RESERVED_MAX;
	rx_schedule_dl_entry = NULL;

#if PERF_MON_ETH_BD
	bd_high_water_mark     = 0;
#endif

	// Check to see if we were given enough room by wlan_mac_high.h for our buffer descriptors
	// At an absolute minimum, there needs to be room for 1 Tx BD and 1 Rx BD
	if (AUX_BRAM_ETH_BD_MEM_SIZE < (2*XAXIDMA_BD_MINIMUM_ALIGNMENT) ) {
		xil_printf("Only %d bytes allocated for Eth Tx BD. Must be at least %d bytes\n", AUX_BRAM_ETH_BD_MEM_SIZE, 2*XAXIDMA_BD_MINIMUM_ALIGNMENT);
		wlan_platform_userio_disp_status(USERIO_DISP_STATUS_CPU_ERROR, WLAN_ERROR_CODE_INSUFFICIENT_BD_SIZE);
	}

	// Split up the memory set aside for us in ETH_MEM_BASE
	gl_eth_tx_bd_mem_base = ETH_BD_MEM_BASE;
	gl_eth_tx_bd_mem_size = XAXIDMA_BD_MINIMUM_ALIGNMENT;
	gl_eth_tx_bd_mem_high = CALC_HIGH_ADDR(gl_eth_tx_bd_mem_base, gl_eth_tx_bd_mem_size);
	gl_eth_rx_bd_mem_base = gl_eth_tx_bd_mem_high+1;
	gl_eth_rx_bd_mem_size = AUX_BRAM_ETH_BD_MEM_SIZE - gl_eth_tx_bd_mem_size;
	gl_eth_rx_bd_mem_high = CALC_HIGH_ADDR(gl_eth_rx_bd_mem_base, gl_eth_rx_bd_mem_size);


	// Initialize buffer descriptor counts
	num_tx_bd = 1;
	xil_printf("%3d Eth Tx BDs placed in BRAM: using %d B\n", num_tx_bd, num_tx_bd*XAXIDMA_BD_MINIMUM_ALIGNMENT);

	num_rx_bd = gl_eth_rx_bd_mem_size / XAXIDMA_BD_MINIMUM_ALIGNMENT;
	xil_printf("%3d Eth Rx BDs placed in BRAM: using %d kB\n", num_rx_bd, num_rx_bd*XAXIDMA_BD_MINIMUM_ALIGNMENT/1024);

	// Initialize Ethernet instance
	eth_cfg_ptr = XAxiEthernet_LookupConfig(WLAN_ETH_DEV_ID);
	status = XAxiEthernet_CfgInitialize(&eth_instance, eth_cfg_ptr, eth_cfg_ptr->BaseAddress);
	if (status != XST_SUCCESS) { xil_printf("Error in XAxiEthernet_CfgInitialize! Err = %d\n", status); return -1; };

	// Setup the Ethernet options
	//     NOTE:  This Ethernet driver does not support jumbo Ethernet frames.  Only 2KB is allocated for each buffer
	//         descriptor and there is a basic assumption that 1 Ethernet frame = 1 buffer descriptor.
	status  = XAxiEthernet_ClearOptions(&eth_instance, XAE_LENTYPE_ERR_OPTION | XAE_FLOW_CONTROL_OPTION | XAE_JUMBO_OPTION);
	status |= XAxiEthernet_SetOptions(&eth_instance, XAE_FCS_STRIP_OPTION | XAE_PROMISC_OPTION | XAE_MULTICAST_OPTION | XAE_BROADCAST_OPTION | XAE_FCS_INSERT_OPTION);
	status |= XAxiEthernet_SetOptions(&eth_instance, XAE_RECEIVER_ENABLE_OPTION | XAE_TRANSMITTER_ENABLE_OPTION);
	if (status != XST_SUCCESS) {xil_printf("Error in XAxiEthernet_Set/ClearOptions! Err = %d\n", status); return -1;};

	// Set the operating speed
	XAxiEthernet_SetOperatingSpeed(&eth_instance, WLAN_ETH_LINK_SPEED);

	// If the link speed is 1 Gbps, then only advertise and link to 1 Gbps connection
	//     See Ethernet PHY specification for documentation on the values used
	if (WLAN_ETH_LINK_SPEED == 1000) {
		XAxiEthernet_PhyWrite(&eth_instance, WLAN_ETH_MDIO_PHYADDR, 0, 0x0140);
		XAxiEthernet_PhyWrite(&eth_instance, WLAN_ETH_MDIO_PHYADDR, 0, 0x8140);
	}

	// Initialize the DMA for Ethernet A
	status = _wlan_eth_dma_init();

	// Start the Ethernet controller
	XAxiEthernet_Start(&eth_instance);

	return 0;
}

/*****************************************************************************/
/**
 * @brief Configures and connects the axi_dma interrupt to the system interrupt controller
 *
 * @param XIntc* intc
 *  - axi_intc driver instance - this function must be called after the axi_intc is setup
 *
 * @return 0 on success, non-zero otherwise
 */
int w3_wlan_platform_ethernet_setup_interrupt(XIntc* intc){
    int                 status;
    XAxiDma_BdRing    * rx_ring_ptr;

    // The interrupt controller will remember an arbitrary value and pass it to the callback
    // when this interrupt fires. We use this to pass the axi_dma Rx BD ring pointer into
    // the _eth_rx_interrupt_handler() callback
    rx_ring_ptr = XAxiDma_GetRxRing(&eth_dma_instance);

    // Connect the axi_dma interrupt
    status = XIntc_Connect(intc, WLAN_ETH_RX_INTR_ID, (XInterruptHandler)_eth_rx_interrupt_handler, rx_ring_ptr);

    if (status != XST_SUCCESS) {
        xil_printf("ERROR: Failed to connect axi_dma interrupt: (%d)\n", status);
        return XST_FAILURE;
    }

    XIntc_Enable(intc, WLAN_ETH_RX_INTR_ID);

    return 0;
}

void w3_wlan_platform_ethernet_free_queue_entry_notify(){
#if WLAN_SW_CONFIG_ENABLE_ETH_BRIDGE
	_wlan_eth_dma_update();
#endif
}



//---------------------------------------
// Private Functions for this file
//  The following functions are helper functions scoped locally to this file



/*****************************************************************************/
/**
 * @brief Initializes the axi_dma core that handles Tx/Rx of Ethernet packets on ETH A
 *
 * Refer to the axi_dma docs and axi_ethernet driver examples for more details on using
 * the axi_dma's scatter-gather mode to handle Ethernet Tx/Rx.
 *
 * @return 0 on success, -1 otherwise
 */
int _wlan_eth_dma_init() {
    u32                 i;
    int                 status;
    u32                 bd_count;
    u32                 max_transfer_len;

    XAxiDma_Config    * eth_dma_cfg_ptr;

    XAxiDma_Bd          eth_dma_bd_template;
    XAxiDma_BdRing    * eth_tx_ring_ptr;
    XAxiDma_BdRing    * eth_rx_ring_ptr;

    XAxiDma_Bd        * first_bd_ptr;
    XAxiDma_Bd        * cur_bd_ptr;

    dl_entry* curr_tx_queue_element;


    // Initialize the DMA peripheral structures
    eth_dma_cfg_ptr = XAxiDma_LookupConfig(WLAN_ETH_DMA_DEV_ID);
    status = XAxiDma_CfgInitialize(&eth_dma_instance, eth_dma_cfg_ptr);
    if (status != XST_SUCCESS) { xil_printf("Error in XAxiDma_CfgInitialize! Err = %d\n", status); return -1; }

    // Zero-out the template buffer descriptor
    XAxiDma_BdClear(&eth_dma_bd_template);

    // Fetch handles to the Tx and Rx BD rings
    eth_tx_ring_ptr = XAxiDma_GetTxRing(&eth_dma_instance);
    eth_rx_ring_ptr = XAxiDma_GetRxRing(&eth_dma_instance);

    // Disable all Tx/Rx DMA interrupts
    XAxiDma_BdRingIntDisable(eth_tx_ring_ptr, XAXIDMA_IRQ_ALL_MASK);
    XAxiDma_BdRingIntDisable(eth_rx_ring_ptr, XAXIDMA_IRQ_ALL_MASK);

    // Disable delays and coalescing by default
    //      NOTE:  We observed no performance increase with interrupt coalescing
    XAxiDma_BdRingSetCoalesce(eth_tx_ring_ptr, 1, 0);
    XAxiDma_BdRingSetCoalesce(eth_rx_ring_ptr, 1, 0);

    // Setup Tx/Rx buffer descriptor rings in memory
    status  = XAxiDma_BdRingCreate(eth_tx_ring_ptr, gl_eth_tx_bd_mem_base, gl_eth_tx_bd_mem_base, XAXIDMA_BD_MINIMUM_ALIGNMENT, num_tx_bd);
    status |= XAxiDma_BdRingCreate(eth_rx_ring_ptr, gl_eth_rx_bd_mem_base, gl_eth_rx_bd_mem_base, XAXIDMA_BD_MINIMUM_ALIGNMENT, num_rx_bd);

    if (status != XST_SUCCESS) { xil_printf("Error creating DMA BD Rings! Err = %d\n", status); return -1; }

    // Populate each ring with empty buffer descriptors
    status  = XAxiDma_BdRingClone(eth_tx_ring_ptr, &eth_dma_bd_template);
    status |= XAxiDma_BdRingClone(eth_rx_ring_ptr, &eth_dma_bd_template);
    if (status != XST_SUCCESS) { xil_printf("Error in XAxiDma_BdRingClone()! Err = %d\n", status); return -1; }

    // Start the DMA Tx channel
    //     NOTE:  No Eth packets are transmitted until actual Tx BD's are pushed to the DMA hardware
    status = XAxiDma_BdRingStart(eth_tx_ring_ptr);

    // Initialize the Rx buffer descriptors
    bd_count = XAxiDma_BdRingGetFreeCnt(eth_rx_ring_ptr);
    if (bd_count != num_rx_bd) { xil_printf("Error in Eth Rx DMA init - not all Rx BDs were free at boot\n"); }

    status = XAxiDma_BdRingAlloc(eth_rx_ring_ptr, bd_count, &first_bd_ptr);
    if (status != XST_SUCCESS) { xil_printf("Error in XAxiDma_BdRingAlloc()! Err = %d\n", status); return -1; }

    // Iterate over each Rx buffer descriptor
    cur_bd_ptr       = first_bd_ptr;
    max_transfer_len = eth_rx_ring_ptr->MaxTransferLen;

    for (i = 0; i < bd_count; i++) {
        // Check out queue element for Rx buffer descriptor
        curr_tx_queue_element = queue_checkout();

        if (curr_tx_queue_element == NULL) {
            xil_printf("Error during _wlan_eth_dma_init: unable to check out sufficient tx_queue_element\n");
            return -1;
        }

        // Initialize the Rx buffer descriptor
        status = _init_rx_bd(cur_bd_ptr, curr_tx_queue_element, max_transfer_len);

        if (status != XST_SUCCESS) {
            xil_printf("Error initializing Rx BD %d\n", i);
            return -1;
        }

        // Update cur_bd_ptr to the next BD in the chain for the next iteration
        cur_bd_ptr = XAxiDma_BdRingNext(eth_rx_ring_ptr, cur_bd_ptr);
    }

    // Push the Rx BD ring to hardware and start receiving
    status = XAxiDma_BdRingToHw(eth_rx_ring_ptr, bd_count, first_bd_ptr);

    // Enable Interrupts
    XAxiDma_BdRingIntEnable(eth_rx_ring_ptr, XAXIDMA_IRQ_ALL_MASK);

    status |= XAxiDma_BdRingStart(eth_rx_ring_ptr);
    if (status != XST_SUCCESS) { xil_printf("Error in XAxiDma BdRingToHw/BdRingStart! Err = %d\n", status); return -1; }

    return 0;
}



/*****************************************************************************/
/**
 * @brief Initializes an Rx buffer descriptor to use the given Tx queue entry
 *
 * @param XAxiDma_Bd * bd_ptr
 *  - Pointer to buffer descriptor to be initialized
 * @param dl_entry * tqe_ptr
 *  - Pointer to Tx queue element
 * @param u32 max_transfer_len
 *  - Max transfer length for Rx BD
 *
 * @return 0 on success, -1 otherwise
 */
int _init_rx_bd(XAxiDma_Bd * bd_ptr, dl_entry * tqe_ptr, u32 max_transfer_len) {
    int  status;
    u32  buf_addr;

    if ((bd_ptr == NULL) || (tqe_ptr == NULL)) { return -1; }

    // Set the memory address for this BD's buffer to the corresponding Tx queue entry buffer
    //     NOTE:  This pointer is offset by the size of a MAC header and LLC header, which results
    //         in the Ethernet payload being copied to its post-encapsulated location. This
    //         speeds up the encapsulation process by skipping any re-copying of Ethernet payloads
    buf_addr = (u32)((void*)((tx_queue_buffer_t*)(tqe_ptr->data))->frame + ETH_PAYLOAD_OFFSET);

    status   = XAxiDma_BdSetBufAddr(bd_ptr, buf_addr);
    if (status != XST_SUCCESS) { xil_printf("XAxiDma_BdSetBufAddr failed (addr 0x08x)! Err = %d\n", buf_addr, status); return -1; }

    // Set every Rx BD to max length (this assures 1 BD per Rx pkt)
    //     NOTE:  Jumbo Ethernet frames are not supported by the Ethernet device (ie the XAE_JUMBO_OPTION is cleared),
    //         so the WLAN_ETH_PKT_BUF_SIZE must be at least large enough to support standard MTUs (ie greater than
    //         1522 bytes) so the assumption of 1 BD = 1 Rx pkt is met.
    status = XAxiDma_BdSetLength(bd_ptr, WLAN_ETH_PKT_BUF_SIZE, max_transfer_len);
    if (status != XST_SUCCESS) { xil_printf("XAxiDma_BdSetLength failed (addr 0x08x)! Err = %d\n", buf_addr, status); return -1; }

    // Rx BD's don't need control flags before use; DMA populates these post-Rx
    XAxiDma_BdSetCtrl(bd_ptr, 0);

    return 0;
}


/*****************************************************************************/
/**
 * @brief Interrupt handler for ETH DMA receptions
 *
 * @param void* callbarck_arg
 *  - Argument passed in by interrupt controller (pointer to axi_dma Rx BD ring for Eth Rx)
 */
void _eth_rx_interrupt_handler(void *callbarck_arg) {
    XAxiDma_BdRing    * rx_ring_ptr = (XAxiDma_BdRing *) callbarck_arg;

#ifdef _ISR_PERF_MON_EN_
    wlan_mac_set_dbg_hdr_out(ISR_PERF_MON_GPIO_MASK);
#endif

    irq_status = XAxiDma_BdRingGetIrq(rx_ring_ptr);

    if (!(irq_status & XAXIDMA_IRQ_ERROR_MASK)) {
        // At least one receptions is completed

        // Disable the interrupt and then acknowledge the interrupt
        XAxiDma_BdRingIntDisable(rx_ring_ptr, irq_status);
        XAxiDma_BdRingAckIrq(rx_ring_ptr, irq_status);

        // Get all the BDs that are available
        //     NOTE:  Use global variable so it is easy to process a sub-set of
        //         packets at a time.
        bd_set_count = XAxiDma_BdRingFromHw(rx_ring_ptr, XAXIDMA_ALL_BDS, &bd_set_to_process_ptr);

#if PERF_MON_ETH_BD
        // Update the BD high water mark
        if (bd_set_count > bd_high_water_mark) {
            bd_high_water_mark = bd_set_count;

            // Schedule a future event to print high water mark
            //     NOTE:  This is to minimize the impact of the print on what we are measuring.
            wlan_mac_schedule_event_repeated(SCHEDULE_COARSE, 0, 1, (void*)print_bd_high_water_mark);
        }
#endif

        // Process all Ethernet packets
        //     NOTE:  Interrupt will be re-enabled in this function when finished processing
        //         all Ethernet packets.
        _wlan_process_all_eth_pkts(SCHEDULE_ID_RESERVED_MAX);

    } else {
        // Acknowledge the error interrupt
        XAxiDma_BdRingAckIrq(rx_ring_ptr, XAXIDMA_IRQ_ERROR_MASK);

        // !!! TODO:  Clean up from error condition !!!
    }

#ifdef _ISR_PERF_MON_EN_
    wlan_mac_clear_dbg_hdr_out(ISR_PERF_MON_GPIO_MASK);
#endif
    return;
}

/*****************************************************************************/
/**
 * @brief Process all Ethernet packets
 *
 * This function will be called by the ISR and the scheduler and will process
 * all the Ethernet packets and re-enable the interrupt if all packets are processed.
 *
 * This function tries to balance two goals:
 *   1) Don't stay in an ISR so long that the Tx PHY is starved by not processing
 *      TX_DONE messages waiting in the mailbox (i.e. only process "max_pkts" packets,
 *      then return)
 *
 *   2) Don't starve the Tx PHY by leaving it idle when taking too long to enqueue
 *      a ready-to-transmit packet received from Ethernet (i.e. process as many
 *      packets as it takes for the packet buffers to be full before returning)
 *
 * NOTE:  This function must be able to handle the case where bd_set_count = 0.
 */
void _wlan_process_all_eth_pkts(u32 schedule_id) {
	u32					wlan_process_eth_rx_return;
    u32                 num_pkt_enqueued   = 0;
    u32					num_pkt_total	   = 0;
    u32					eth_rx_len, eth_rx_buf;
    u32					status;
    XAxiDma_BdRing    * rx_ring_ptr         = XAxiDma_GetRxRing(&eth_dma_instance);

#if PERF_MON_ETH_PROCESS_ALL_RX
    wlan_mac_set_dbg_hdr_out(0x2);
#endif

    if(schedule_id != SCHEDULE_ID_RESERVED_MAX){
    	// This function was called from the context of the scheduler. We should disable the schedule
    	rx_schedule_dl_entry = wlan_mac_schedule_disable_id(SCHEDULE_FINE, schedule_id);
    }

    while (bd_set_count > 0) {
        // Process Ethernet packet

        // Lookup length and data pointers from the DMA metadata
        eth_rx_len     = XAxiDma_BdGetActualLength(bd_set_to_process_ptr, rx_ring_ptr->MaxTransferLen);
        eth_rx_buf     = XAxiDma_BdGetBufAddr(bd_set_to_process_ptr);

    	wlan_process_eth_rx_return = wlan_process_eth_rx((void*)eth_rx_buf, eth_rx_len);

        // Free the ETH DMA buffer descriptor
        status = XAxiDma_BdRingFree(rx_ring_ptr, 1, bd_set_to_process_ptr);
        if(status != XST_SUCCESS) {
        	xil_printf("Error in XAxiDma_BdRingFree of Rx BD! Err = %d\n", status);
        }

        // Update to the next BD in the chain for the next iteration
        bd_set_to_process_ptr = XAxiDma_BdRingNext(rx_ring_ptr, bd_set_to_process_ptr);

        // Increment counters
        if(wlan_process_eth_rx_return & WLAN_PROCESS_ETH_RX_RETURN_IS_ENQUEUED){
        	num_pkt_enqueued++;
        }
        num_pkt_total++;
        bd_set_count--;

        // Check stop condition
        if( (num_pkt_enqueued >= MAX_PACKETS_ENQUEUED)||(num_pkt_total >= MAX_PACKETS_TOTAL)) {
            // Processed enough packets in this call and the Tx PHY isn't waiting on an
            // Ethernet packet to transmit.  Leave this ISR to handle any other higher
            // priority interrupts, such as IPC messages, then come back later to process
            // the next set of Ethernet BDs.

        	// One subtle concession in this implementation is that we only check PKT_BUF_GROUP_GENERAL. If
        	// PKT_BUF_GROUP_DTIM_MCAST is able to be dequeued into, we will still defer processing Ethernet
        	// receptions until later. The advantage of this is that we avoid a scenario where large bursts
        	// of unicast Ethernet receptions are not deferred simply because there is available space in the
        	// PKT_BUF_GROUP_DTIM_MCAST group.

            break;
        }
    }

    // Reassign any free DMA buffer descriptors to a new queue entry
    _wlan_eth_dma_update();

    if (bd_set_count > 0) {

    	if(rx_schedule_dl_entry == NULL){
			// Set up scheduled event for processing next packets
			//     NOTE:  All global variables have been updated
			wlan_mac_schedule_event_repeated(SCHEDULE_FINE, 0, SCHEDULE_REPEAT_FOREVER, (void*)_wlan_process_all_eth_pkts);
    	} else {
    		// We have already previously configured this schedule, so we should just enable it again
    		wlan_mac_schedule_enable(SCHEDULE_FINE, rx_schedule_dl_entry);
    	}

    } else {
        // Finished all available Eth Rx - re-enable interrupt
        XAxiDma_BdRingIntEnable(rx_ring_ptr, irq_status);

        // Set global variable to NULL (for safety)
        bd_set_to_process_ptr = NULL;
    }

#if PERF_MON_ETH_PROCESS_ALL_RX
    wlan_mac_clear_dbg_hdr_out(0x02);
#endif
}

/*****************************************************************************/
/**
 * @brief Updates any free ETH Rx DMA buffer descriptors
 *
 * This function checks if any ETH DMA Rx buffer descriptors have been freed and
 * are ready to be reused.  For each free BD, this function attempts to checkout
 * a Tx queue entry and assign its payload to the BD. If successful, the BD is
 * then submitted to the DMA hardware for use by future Ethernet receptions. If
 * unsuccessful the BD is left free, to be recycled on the next iteration of this
 * function.
 *
 * The total number of ETH Rx buffer descriptors is set at boot during the DMA init.
 * The same number of Tx queue entries are effectively reserved by the MAC in its
 * queue size calculations. This function can handle the case of more ETH Rx BDs
 * than free Tx queue entries, though this should never happen.
 *
 * This function should be called under the following conditions:
 *   - A Rx buffer descriptor is finished being processed
 *   - A Tx queue entry has finished being processed
 * This will assure that enough Rx BDs are available to the DMA hardware.
 */
void _wlan_eth_dma_update() {
    int                 status;
    int                 iter;
    u32                 bd_count;
    u32                 bd_queue_pairs_to_process;
    u32                 bd_queue_pairs_processed;
    u32                 max_transfer_len;

    XAxiDma_BdRing    * rx_ring_ptr;
    XAxiDma_Bd        * first_bd_ptr;
    XAxiDma_Bd        * cur_bd_ptr;

    dl_list             checkout;
    dl_entry*           tx_queue_entry;

#if PERF_MON_ETH_UPDATE_DMA
    wlan_mac_set_dbg_hdr_out(0x2);
#endif

    // Get the Rx ring pointer and the number of free Rx buffer descriptors
    rx_ring_ptr = XAxiDma_GetRxRing(&eth_dma_instance);
    bd_count    = XAxiDma_BdRingGetFreeCnt(rx_ring_ptr);

    // If there are no BDs, then we are done
    if (bd_count == 0) {

#if PERF_MON_ETH_UPDATE_DMA
        wlan_mac_set_dbg_hdr_out(0x2);
#endif

        return;
    }

    // Initialize the list to checkout Tx queue entries
    dl_list_init(&checkout);

    // Attempt to checkout Tx queue entries for all free buffer descriptors
    queue_checkout_list(&checkout, bd_count);

    // If there were not enough Tx queue entries available, the length of the
    // checkout list will be the number of free Tx queue entries that were
    // found.  Only process buffer descriptors that have a corresponding Tx
    // queue entry.
    bd_queue_pairs_to_process = min(bd_count, checkout.length);

    if (bd_queue_pairs_to_process > 0) {

        // Initialize the number of buffer descriptors processed
        bd_queue_pairs_processed = 0;

        // Allocate the correct number of Rx buffer descriptors that have
        // been freed and have Tx queue entries available.
        status = XAxiDma_BdRingAlloc(rx_ring_ptr, bd_queue_pairs_to_process, &first_bd_ptr);
        if(status != XST_SUCCESS) {xil_printf("Error in XAxiDma_BdRingAlloc()! Err = %d\n", status); return;}

        // Initialize loop variables
        iter             = checkout.length;
        tx_queue_entry   = checkout.first;
        cur_bd_ptr       = first_bd_ptr;
        max_transfer_len = rx_ring_ptr->MaxTransferLen;

        while ((tx_queue_entry != NULL) && (iter-- > 0)) {

#if PERF_MON_ETH_UPDATE_DMA
            wlan_mac_set_dbg_hdr_out(0x4);
#endif

            // Initialize the Rx buffer descriptor
            status = _init_rx_bd(cur_bd_ptr, tx_queue_entry, max_transfer_len);

            if (status != XST_SUCCESS) {
                // Clean up buffer descriptors and Tx queues
                //     NOTE:  Regardless of where we are in the update, we will check
                //         back in everything so the function can start fresh next invocation
                status = XAxiDma_BdRingUnAlloc(rx_ring_ptr, bd_queue_pairs_to_process, first_bd_ptr);
                if(status != XST_SUCCESS) {xil_printf("Error in XAxiDma_BdRingUnAlloc()! Err = %d\n", status);}

                queue_checkin_list(&checkout);

                return;
            }

            // Update the BD and queue entry pointers to the next list elements
            //     NOTE: This loop traverses both lists simultaneously
            cur_bd_ptr     = XAxiDma_BdRingNext(rx_ring_ptr, cur_bd_ptr);
            tx_queue_entry = dl_entry_next(tx_queue_entry);
            bd_queue_pairs_processed++;

#if PERF_MON_ETH_UPDATE_DMA
            wlan_mac_clear_dbg_hdr_out(0x4);
#endif
        }

        if (bd_queue_pairs_processed == bd_queue_pairs_to_process) {
            // Push the Rx BD ring to hardware and start receiving
            status = XAxiDma_BdRingToHw(rx_ring_ptr, bd_queue_pairs_to_process, first_bd_ptr);
            if(status != XST_SUCCESS) {xil_printf("XAxiDma_BdRingToHw failed! Err = %d\n", status);}
        } else {
            // Clean up buffer descriptors and Tx queues
            xil_printf("Error processing BD-queue pairs\n");

            status = XAxiDma_BdRingUnAlloc(rx_ring_ptr, bd_queue_pairs_to_process, first_bd_ptr);
            if(status != XST_SUCCESS) {xil_printf("Error in XAxiDma_BdRingUnAlloc()! Err = %d\n", status);}

            queue_checkin_list(&checkout);
        }
    }

#if PERF_MON_ETH_UPDATE_DMA
    wlan_mac_clear_dbg_hdr_out(0x2);
#endif
}


#endif //WLAN_SW_CONFIG_ENABLE_ETH_BRIDGE



