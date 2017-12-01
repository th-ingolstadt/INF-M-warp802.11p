#include "rftap.h"

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "xil_cache.h"
#include "xstatus.h"

#include "wlan_platform_common.h"
#include "wlan_mac_common.h"

#define IPV4_MAX_HOPS 64

void rftap_init(unsigned int device_num) {
	int ret;

	// Get hardware info
    wlan_mac_hw_info_t *hw_info = get_mac_hw_info();

    if ((ret = transport_check_device(device_num)) != XST_SUCCESS) {
    	xil_printf("Error: transport_check_device() failed: %d\n", ret);
    	return;
    }

	// Success!
	xil_printf("Successfully initialized RFTap interface\n");
}

radiotap_header_t* radiotap_frame_init(void *dst, size_t *offset) {
	radiotap_header_t *frame = dst + *offset;
	memset (frame, 0, sizeof(radiotap_header_t));
	*offset += sizeof(radiotap_header_t);

	frame->it_version = 0;

	return frame;
}

rftap_header_t* rftap_frame_init(void *dst, size_t *offset) {
	rftap_header_t *frame = dst + *offset;
	memset (frame, 0, sizeof(rftap_header_t));
	*offset += sizeof(rftap_header_t);

	frame->magic = Xil_Htonl(0x52467461);

	return frame;
}

udp_header_t* udp_frame_init(void *dst, size_t *offset) {
	udp_header_t* frame = dst + *offset;
	memset (frame, 0, sizeof(udp_header_t));
	*offset += sizeof(udp_header_t);

	return frame;
}

ipv4_header_t* ip_frame_init(void *dst, size_t *offset) {
	ipv4_header_t* frame = dst + *offset;
	memset (frame, 0, sizeof(ipv4_header_t));
	*offset += sizeof(ipv4_header_t);

	frame->version_ihl = 0x45;
	frame->ttl = IPV4_MAX_HOPS;
	frame->protocol = IPV4_PROT_UDP;

	return frame;
}

void ip_frame_calc_checksum(ipv4_header_t *dst) {
	int hdr_len = 20;
	unsigned long sum = 0;
	const u16 *ip1;
	ip1 = dst;
	while (hdr_len > 1)
	{
		sum += *ip1++;
		if (sum & 0x80000000)
			sum = (sum & 0xFFFF) + (sum >> 16);
		hdr_len -= 2;
	}

	while (sum >> 16)
		sum = (sum & 0xFFFF) + (sum >> 16);

	dst->header_checksum = sum;
}

ethernet_header_t* ethernet_frame_init(void *dst, size_t *offset) {
	ethernet_header_t* frame = dst + *offset;
	memset (frame, 0, sizeof(ethernet_header_t));
	*offset += sizeof(ethernet_header_t);

	frame->ethertype = ETH_TYPE_IP;

	return frame;
}
