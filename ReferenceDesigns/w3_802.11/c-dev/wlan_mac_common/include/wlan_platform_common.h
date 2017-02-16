#ifndef WLAN_PLATFORM_COMMON_H_
#define WLAN_PLATFORM_COMMON_H_

#include "xil_types.h"
#include "wlan_mac_802_11_defs.h"

//-----------------------------------------------
// Hardware information struct to share data between the
//   low and high CPUs

#define WLAN_MAC_FPGA_DNA_LEN         2

typedef struct {
    u32  serial_number;
    u32  fpga_dna[WLAN_MAC_FPGA_DNA_LEN];
    u8   hw_addr_wlan[MAC_ADDR_LEN];
    u8   hw_addr_wlan_exp[MAC_ADDR_LEN];
} wlan_mac_hw_info_t;

typedef enum {
	USERIO_DISP_STATUS_IDENTIFY     		= 0,
	USERIO_DISP_STATUS_APPLICATION_ROLE     = 1,
	USERIO_DISP_STATUS_MEMBER_LIST_UPDATE   = 2,
	USERIO_DISP_STATUS_WLAN_EXP_CONFIGURE   = 3,
	USERIO_DISP_STATUS_GOOD_FCS_EVENT       = 4,
	USERIO_DISP_STATUS_BAD_FCS_EVENT        = 5,
	USERIO_DISP_STATUS_CPU_ERROR    		= 255
} userio_disp_status_t;

typedef enum {
	USERIO_INPUT_MASK_PB_0		= 0x00000001,
	USERIO_INPUT_MASK_PB_1		= 0x00000002,
	USERIO_INPUT_MASK_PB_2		= 0x00000004,
	USERIO_INPUT_MASK_PB_3		= 0x00000008,
	USERIO_INPUT_MASK_SW_0		= 0x00000010,
	USERIO_INPUT_MASK_SW_1		= 0x00000020,
	USERIO_INPUT_MASK_SW_2		= 0x00000040,
	USERIO_INPUT_MASK_SW_3		= 0x00000080,
} userio_input_mask_t;

//---------------------------------------
// Platform information struct
typedef struct{
	u32	platform_id;
	u32 cpu_id;
	u32 is_cpu_high;
	u32 is_cpu_low;
	u32 mailbox_dev_id;
	u32	pkt_buf_mutex_dev_id;
	u32	tx_pkt_buf_baseaddr;
	u32	rx_pkt_buf_baseaddr;
} platform_common_dev_info_t;

// Common platform init functions
platform_common_dev_info_t wlan_platform_common_get_dev_info();
int wlan_platform_common_init();
wlan_mac_hw_info_t wlan_platform_get_hw_info();

// User IO functions
void wlan_platform_userio_disp_status(userio_disp_status_t status, ...);
u32  wlan_platform_userio_get_state();

// Temperature functions
u32 wlan_platform_get_current_temp();
u32 wlan_platform_get_min_temp();
u32 wlan_platform_get_max_temp();

// MAC time functions
volatile u64       get_mac_time_usec();
void               set_mac_time_usec(u64 new_time);
void               apply_mac_time_delta_usec(s64 time_delta);
volatile u64       get_system_time_usec();
void               wlan_usleep(u64 delay);

#endif /* WLAN_PLATFORM_COMMON_H_ */
