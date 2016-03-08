/** @file wlan_mac_bss_info.h
 *  @brief BSS Info Subsystem
 *
 *  This contains code tracking BSS information. It also contains code for managing
 *  the active scan state machine.
 *
 *  @copyright Copyright 2014-2016, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 */

#ifndef WLAN_MAC_BSS_INFO_H_
#define WLAN_MAC_BSS_INFO_H_

/***************************** Include Files *********************************/

#include "wlan_mac_high.h"


/*************************** Constant Definitions ****************************/

//-----------------------------------------------
// Timeout used to remove inactive BSS infos
//     - Part of bss_info_timestamp_check()
//
#define BSS_INFO_TIMEOUT_USEC                              600000000


//-----------------------------------------------
// Field size defines
//
#define BSSID_LEN                                          6
#define NUM_BASIC_RATES_MAX                                10


//-----------------------------------------------
// BSS Beacon Interval defines
//
#define BSS_MICROSECONDS_IN_A_TU                           1024
#define BEACON_INTERVAL_NO_BEACON_TX                       0x0


//-----------------------------------------------
// BSS State defines
//
#define BSS_STATE_UNAUTHENTICATED                          1
#define BSS_STATE_AUTHENTICATED                            2
#define BSS_STATE_ASSOCIATED                               4


//-----------------------------------------------
// BSS Flags defines
//
#define BSS_FLAGS_KEEP                                     0x0001


//-----------------------------------------------
// BSS Configuration Bit Masks
//
#define BSS_FIELD_MASK_BSSID                               0x00000001
#define BSS_FIELD_MASK_CHAN                                0x00000002
#define BSS_FIELD_MASK_SSID                                0x00000004
#define BSS_FIELD_MASK_BEACON_INTERVAL                     0x00000008
#define BSS_FIELD_MASK_HT_CAPABLE                          0x00000010


//-----------------------------------------------
// configure_bss() return error flags
//
#define BSS_CONFIG_FAILURE_BSSID_INVALID                   0x00000001
#define BSS_CONFIG_FAILURE_BSSID_INSUFFICIENT_ARGUMENTS    0x00000002
#define BSS_CONFIG_FAILURE_CHANNEL_INVALID                 0x00000004
#define BSS_CONFIG_FAILURE_BEACON_INTERVAL_INVALID         0x00000008
#define BSS_CONFIG_FAILURE_HT_CAPABLE_INVALID              0x00000010


//-----------------------------------------------
// Define common BSS info fields
//   Note: These fields have been 32 bit aligned using padding bytes

#define MY_BSS_INFO_COMMON_FIELDS                                                                       \
        u8         bssid[BSSID_LEN];                  /* BSS ID - 48 bit HW address */                  \
        u8         chan;                              /* Channel */                                     \
        u8         flags;                             /* BSS Flags - Each flag is 1 bit */              \
        u64        latest_activity_timestamp;         /* Timestamp - Last interaction with BSS */       \
        char       ssid[SSID_LEN_MAX + 1];            /* SSID of the BSS - 33 bytes */                  \
        u8         state;                             /* State of the BSS */                            \
        u16        capabilities;                      /* Supported capabilities */                      \
        u16        beacon_interval;                   /* Beacon interval - In time units of 1024 us */  \
        u8         padding0;                          /* Padding byte - for 32-bit alignment */         \
        u8         num_basic_rates;                   /* Number of basic rates supported */             \
        u8         basic_rates[NUM_BASIC_RATES_MAX];  /* Supported basic rates - 10 bytes */            \
        u8         phy_mode;                          /* PHY Mode (Legacy, HT) */                       \
        s8         rx_power_dBm;                      /* Last observed Rx Power (dBm) */



/*********************** Global Structure Definitions ************************/

/**
 * @brief Result of Join attempt
 *
 * This defines the values of a join attempt
 */
typedef enum {
    NEVER_ATTEMPTED = 0,
    NO_RESPONSE     = 1,
    DENIED          = 2,
    SUCCESSFUL      = 3
} join_attempt_result_t;


/**
 * @brief Basic Service Set (BSS) Information Structure
 *
 * This struct contains information about the basic service set for the node.
 */
typedef struct{
    MY_BSS_INFO_COMMON_FIELDS
    join_attempt_result_t           last_join_attempt_result;
    u64                             last_join_attempt_timestamp;
    dl_list associated_stations;
} bss_info;



/**
 * @brief Base BSS Information Structure
 *
 * This struct is a modification of the bss_info struct that eliminates
 * pointers to other data.
 */
typedef struct{
    MY_BSS_INFO_COMMON_FIELDS
} bss_info_base;



/**
 * @brief BSS Configuration Structure
 *
 * This struct contains all the BSS info fields that can be modified.
 */
typedef struct{
    u32        update_mask;                       /* Mask of fields that were updated */
    u8         bssid[BSSID_LEN];                  /* BSS ID */
    u16        beacon_interval;                   /* Beacon interval - In time units of 1024 us */
    char       ssid[SSID_LEN_MAX + 1];            /* SSID of the BSS - 33 bytes */
    u8         chan;                              /* Channel */
    u8         ht_capable;                        /* Support HTMF Tx/Rx */
    u8         padding;                           /* Padding byte(s) */
} bss_config_t;



/*************************** Function Prototypes *****************************/

void             bss_info_init(u8 dram_present);
void             bss_info_init_finish();

dl_entry       * bss_info_checkout();
void             bss_info_checkin(dl_entry* bsi);

inline void      bss_info_rx_process(void* pkt_buf_addr);

void             print_bss_info();
void             bss_info_timestamp_check();

dl_list        * wlan_mac_high_find_bss_info_SSID(char* ssid);
dl_entry       * wlan_mac_high_find_bss_info_BSSID(u8* bssid);

bss_info       * wlan_mac_high_create_bss_info(u8* bssid, char* ssid, u8 chan);
void 			 wlan_mac_high_reset_network_list();
void             wlan_mac_high_clear_bss_info(bss_info * info);

inline dl_list * wlan_mac_high_get_bss_info_list();

#endif
