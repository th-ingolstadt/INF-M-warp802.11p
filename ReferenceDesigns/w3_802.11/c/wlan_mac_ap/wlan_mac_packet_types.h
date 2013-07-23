////////////////////////////////////////////////////////////////////////////////
// File   : wlan_mac_packet_types.h
// Authors: Patrick Murphy (murphpo [at] mangocomm.com)
//			Chris Hunter (chunter [at] mangocomm.com)
// License: Copyright 2013, Mango Communications. All rights reserved.
//          Distributed under the Mango Communications Reference Design License
//				See LICENSE.txt included in the design archive or
//				at http://mangocomm.com/802.11/license
////////////////////////////////////////////////////////////////////////////////


#ifndef WLAN_MAC_PACKET_TYPES_H_
#define WLAN_MAC_PACKET_TYPES_H_


typedef struct{
	u16 auth_algorithm;
	u16 auth_sequence;
	u16 status_code;
} authentication_frame;

typedef struct{
	u16 capabilities;
	u16 listen_interval;
} association_request_frame;

typedef struct{
	u16 capabilities;
	u16 status_code;
	u16 association_id;
} association_response_frame;

#define AUTH_ALGO_OPEN_SYSTEM 0x00

#define AUTH_SEQ_REQ 0x01
#define AUTH_SEQ_RESP 0x02

// Status Codes: Table 7-23 in 802.11-2007
#define STATUS_SUCCESS 0
#define STATUS_AUTH_REJECT_CHALLENGE_FAILURE 15

int wlan_create_beacon_probe_frame(void* pkt_buf, u8 subtype, u8* address1, u8* address2, u8* address3, u16 seq_num, u16 beacon_interval, u8 ssid_len, u8* ssid, u8 chan, u8* OUI);
int wlan_create_auth_frame(void* pkt_buf, u16 auth_algorithm,  u16 auth_seq, u16 status_code, u8* address1, u8* address2, u8* address3, u16 seq_num, u8* OUI);
int wlan_create_association_response_frame(void* pkt_buf, u8 subtype, u8* address1, u8* address2, u8* address3, u16 seq_num, u16 status, u16 AID, u8* OUI);
int wlan_create_data_frame(void* pkt_buf, u8 flags, u8* address1, u8* address2, u8* address3, u16 seq_num);

#endif /* WLAN_MAC_PACKET_TYPES_H_ */
