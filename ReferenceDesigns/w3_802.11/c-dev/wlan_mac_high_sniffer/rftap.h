#include "xparameters.h"
#include "stdio.h"
#include "stdlib.h"
#include "xtmrctr.h"
#include "xio.h"
#include "string.h"
#include "xintc.h"

#include "wlan_mac_high.h"
#include "wlan_mac_eth_util.h"

struct rftap_header {
    u32 	magic;  // signature: "RFta"
    u16 	len32;  // length
    u16 	flags;  // bitfield indicating presence of fields
    u32		dlt;
} __attribute__((packed));
typedef struct rftap_header rftap_header_t;
ASSERT_TYPE_SIZE(rftap_header_t, 12);

struct ieee80211_radiotap_header {
        u8        it_version;     /* set to 0 */
        u8        it_pad;
        u16       it_len;         /* entire length */
        u32       it_present;     /* fields present */
} __attribute__((__packed__));
typedef struct ieee80211_radiotap_header radiotap_header_t;
ASSERT_TYPE_SIZE(radiotap_header_t, 8);

void rftap_init(unsigned int device_num);

radiotap_header_t* radiotap_frame_init(void *dst, size_t *offset);
rftap_header_t* rftap_frame_init(void *dst, size_t *offset);
udp_header_t* udp_frame_init(void *dst, size_t *offset);
ipv4_header_t* ip_frame_init(void *dst, size_t *offset);
ethernet_header_t* ethernet_frame_init(void *dst, size_t *offset);

// private

