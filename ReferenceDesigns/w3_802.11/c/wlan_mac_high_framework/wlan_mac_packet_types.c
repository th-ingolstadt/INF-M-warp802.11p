////////////////////////////////////////////////////////////////////////////////
// File   : wlan_mac_packet_types.c
// Authors: Patrick Murphy (murphpo [at] mangocomm.com)
//			Chris Hunter (chunter [at] mangocomm.com)
// License: Copyright 2013, Mango Communications. All rights reserved.
//          Distributed under the Mango Communications Reference Design License
//				See LICENSE.txt included in the design archive or
//				at http://mangocomm.com/802.11/license
////////////////////////////////////////////////////////////////////////////////
//Xilinx SDK includes
#include "stdio.h"
#include "xio.h"
#include "string.h"
#include "xil_types.h"
#include "xintc.h"

//WARP includes
#include "wlan_mac_ipc_util.h"
#include "wlan_mac_packet_types.h"

int wlan_create_beacon_probe_frame(void* pkt_buf, u8 frame_control_1, u8* address1, u8* address2, u8* address3, u16 seq_num, u16 beacon_interval, u8 ssid_len, u8* ssid, u8 chan, u8* OUI) {
	u32 packetLen_bytes;
	u8* txBufferPtr_u8;

	txBufferPtr_u8 = (u8*)pkt_buf;

	mac_header_80211* beacon_80211_header;
	beacon_80211_header = (mac_header_80211*)(txBufferPtr_u8);

	beacon_80211_header->frame_control_1 = frame_control_1;
	beacon_80211_header->frame_control_2 = 0;

	//This field may be overwritten by CPU_LOW
	beacon_80211_header->duration_id = 0;

	memcpy(beacon_80211_header->address_1,address1,6);
	memcpy(beacon_80211_header->address_2,address2,6);
	memcpy(beacon_80211_header->address_3,address3,6);

	beacon_80211_header->sequence_control = ((seq_num&0xFFF)<<4);

	beacon_probe_frame* beacon_probe_mgmt_header;
	beacon_probe_mgmt_header = (beacon_probe_frame*)(pkt_buf + sizeof(mac_header_80211));

	//This field may be overwritten by CPU_LOW
	beacon_probe_mgmt_header->timestamp = 0;

	beacon_probe_mgmt_header->beacon_interval = beacon_interval;
	beacon_probe_mgmt_header->capabilities = (CAPABILITIES_ESS | CAPABILITIES_SHORT_PREAMBLE | CAPABILITIES_SHORT_TIMESLOT);

	txBufferPtr_u8 = (u8 *)((void *)(txBufferPtr_u8) + sizeof(mac_header_80211) + sizeof(beacon_probe_frame));
	txBufferPtr_u8[0] = 0; //Tag 0: SSID parameter set
	txBufferPtr_u8[1] = ssid_len;
	memcpy((void *)(&(txBufferPtr_u8[2])),(void *)(&ssid[0]),ssid_len);

	txBufferPtr_u8+=(ssid_len+2); //Move up to next tag

	//http://my.safaribooksonline.com/book/networking/wireless/0596100523/4dot-802dot11-framing-in-detail/wireless802dot112-chp-4-sect-3
	//Top bit is whether or not the rate is mandatory (basic). Bottom 7 bits is in units of "number of 500kbps"
	txBufferPtr_u8[0] = 1; //Tag 1: Supported Rates
	txBufferPtr_u8[1] = 8; //tag length... doesn't include the tag itself and the tag length
	txBufferPtr_u8[2] = RATE_BASIC | (0x0C); 	//6Mbps  (BPSK,   1/2)
	txBufferPtr_u8[3] = (0x12);				 	//9Mbps  (BPSK,   3/4)
	txBufferPtr_u8[4] = RATE_BASIC | (0x18); 	//12Mbps (QPSK,   1/2)
	txBufferPtr_u8[5] = (0x24); 				//18Mbps (QPSK,   3/4)
	txBufferPtr_u8[6] = RATE_BASIC | (0x30); 	//24Mbps (16-QAM, 1/2)
	txBufferPtr_u8[7] = (0x48); 				//36Mbps (16-QAM, 3/4)
	txBufferPtr_u8[8] = (0x60); 				//48Mbps  (64-QAM, 2/3)
	txBufferPtr_u8[9] = (0x6C); 				//54Mbps  (64-QAM, 3/4)
	txBufferPtr_u8+=(8+2); //Move up to next tag

	//txBufferPtr_u8[9] = (0x60); 				//48Mbps (64-QAM, 2/3)
	//txBufferPtr_u8+=(8+2); //Move up to next tag

	/*txBufferPtr_u8[0] = 1; //Tag 1: Supported Rates
	txBufferPtr_u8[1] = 2; //tag length... doesn't include the tag itself and the tag length
	txBufferPtr_u8[2] = (0x02); 				//1Mbps  (DSSS)
	txBufferPtr_u8[3] = RATE_BASIC | (0x18); 	//12Mbps (QPSK,   1/2) REQUIRED (since we can't TX at rate 1Mbps, we require STA to be able to receive 12Mbps)
	txBufferPtr_u8+=(2+2); //Move up to next tag*/

	txBufferPtr_u8[0] = 3; //Tag 3: DS Parameter set
	txBufferPtr_u8[1] = 1; //tag length... doesn't include the tag itself and the tag length
	txBufferPtr_u8[2] = chan;
	txBufferPtr_u8+=(1+2);

	txBufferPtr_u8[0] = 5; //Tag 5: Traffic Indication Map (TIM)
	txBufferPtr_u8[1] = 4; //tag length... doesn't include the tag itself and the tag length
	txBufferPtr_u8[2] = 0; //DTIM count
	txBufferPtr_u8[3] = 1; //DTIM period
	txBufferPtr_u8[4] = 1; //Bitmap control 1 //0 to disable direct multicast
	txBufferPtr_u8[5] = 0; //Bitmap control 1
	txBufferPtr_u8+=(4+2);

	txBufferPtr_u8[0] = 42; //Tag 42: ERP Info
	txBufferPtr_u8[1] = 1; //tag length... doesn't include the tag itself and the tag length
	txBufferPtr_u8[2] = 0; //Non ERP Present - not set, don't use protection, no barker preamble mode
	txBufferPtr_u8+=(1+2);

	txBufferPtr_u8[0] = 47; //Tag 47: ERP Info
	txBufferPtr_u8[1] = 1; //tag length... doesn't include the tag itself and the tag length
	txBufferPtr_u8[2] = 0; //Non ERP Present - not set, don't use protection, no barker preamble mode
	txBufferPtr_u8+=(1+2);

//	txBufferPtr_u8[0] = 50; //Tag 50: Extended Supported Rates
//	txBufferPtr_u8[1] = 1;
//	txBufferPtr_u8[2] = (0x6C); 				//54Mbps  (64-QAM,   3/4)
//	txBufferPtr_u8+=(1+2);

//	txBufferPtr_u8[0] = 221; //Tag 221: Vendor specific
//	txBufferPtr_u8[1] = 3; //tag length... doesn't include the tag itself and the tag length
//	memcpy(&(txBufferPtr_u8[2]),OUI,3);
//	txBufferPtr_u8+=(3+2);

	packetLen_bytes = txBufferPtr_u8 - (u8*)(pkt_buf);



	return packetLen_bytes;
}

int wlan_create_auth_frame(void* pkt_buf, u16 auth_algorithm,  u16 auth_seq, u16 status_code, u8* address1, u8* address2, u8* address3, u16 seq_num, u8* OUI){
	u32 packetLen_bytes;
	u8* txBufferPtr_u8;

	txBufferPtr_u8 = (u8*)pkt_buf;

	mac_header_80211* auth_80211_header;
	auth_80211_header = (mac_header_80211*)(txBufferPtr_u8);

	auth_80211_header->frame_control_1 = MAC_FRAME_CTRL1_SUBTYPE_AUTH;
	auth_80211_header->frame_control_2 = 0;

	//duration can be filled in by CPU_LOW
	auth_80211_header->duration_id = 0;
	memcpy(auth_80211_header->address_1,address1,6);
	memcpy(auth_80211_header->address_2,address2,6);
	memcpy(auth_80211_header->address_3,address3,6);

	auth_80211_header->sequence_control = ((seq_num&0xFFF)<<4);

	authentication_frame* auth_mgmt_header;
	auth_mgmt_header = (authentication_frame*)(pkt_buf + sizeof(mac_header_80211));
	auth_mgmt_header->auth_algorithm = auth_algorithm;
	auth_mgmt_header->auth_sequence = auth_seq;
	auth_mgmt_header->status_code = status_code;

	txBufferPtr_u8 = (u8 *)((void *)(txBufferPtr_u8) + sizeof(mac_header_80211) + sizeof(authentication_frame));

	txBufferPtr_u8[0] = 221; //Tag 221: Vendor specific
	txBufferPtr_u8[1] = 3; //tag length... doesn't include the tag itself and the tag length
	memcpy(&(txBufferPtr_u8[2]),OUI,3);
	txBufferPtr_u8+=(3+2);

	packetLen_bytes = txBufferPtr_u8 - (u8*)(pkt_buf);

	return packetLen_bytes;

}

int wlan_create_deauth_frame(void* pkt_buf, u16 reason_code, u8* address1, u8* address2, u8* address3, u16 seq_num, u8* OUI){
	u32 packetLen_bytes;
	u8* txBufferPtr_u8;

	txBufferPtr_u8 = (u8*)pkt_buf;

	mac_header_80211* deauth_80211_header;
	deauth_80211_header = (mac_header_80211*)(txBufferPtr_u8);

	deauth_80211_header->frame_control_1 = MAC_FRAME_CTRL1_SUBTYPE_DEAUTH;
	deauth_80211_header->frame_control_2 = 0;

	//duration can be filled in by CPU_LOW
	deauth_80211_header->duration_id = 0;
	memcpy(deauth_80211_header->address_1,address1,6);
	memcpy(deauth_80211_header->address_2,address2,6);
	memcpy(deauth_80211_header->address_3,address3,6);

	deauth_80211_header->sequence_control = ((seq_num&0xFFF)<<4);

	deauthentication_frame* deauth_mgmt_header;
	deauth_mgmt_header = (deauthentication_frame*)(pkt_buf + sizeof(mac_header_80211));
	deauth_mgmt_header->reason_code = reason_code;

	txBufferPtr_u8 = (u8 *)((void *)(txBufferPtr_u8) + sizeof(mac_header_80211) + sizeof(authentication_frame));

	txBufferPtr_u8[0] = 221; //Tag 221: Vendor specific
	txBufferPtr_u8[1] = 3; //tag length... doesn't include the tag itself and the tag length
	memcpy(&(txBufferPtr_u8[2]),OUI,3);
	txBufferPtr_u8+=(3+2);

	packetLen_bytes = txBufferPtr_u8 - (u8*)(pkt_buf);

	return packetLen_bytes;

}

//wlan_create_association_response_frame((void*)(TX_PKT_BUF_TO_ADDR(tx_pkt_buf)+PHY_TX_PKT_BUF_MPDU_OFFSET), ASSOC_RESP, rx_80211_header->address_2, eeprom_mac_addr, eeprom_mac_addr, seq_num++, STATUS_SUCCESS, 0xC000 | associations[i][0],eeprom_mac_addr);
int wlan_create_association_response_frame(void* pkt_buf, u8 frame_control_1, u8* address1, u8* address2, u8* address3, u16 seq_num, u16 status, u16 AID, u8* OUI) {
	u32 packetLen_bytes;
	u8* txBufferPtr_u8;

	txBufferPtr_u8 = (u8*)pkt_buf;

	mac_header_80211* assoc_80211_header;
	assoc_80211_header = (mac_header_80211*)(txBufferPtr_u8);

	assoc_80211_header->frame_control_1 = frame_control_1;
	assoc_80211_header->frame_control_2 = 0;
	//duration can be filled in by CPU_LOW
	assoc_80211_header->duration_id = 0;

	memcpy(assoc_80211_header->address_1,address1,6);
	memcpy(assoc_80211_header->address_2,address2,6);
	memcpy(assoc_80211_header->address_3,address3,6);

	assoc_80211_header->sequence_control = ((seq_num&0xFFF)<<4);

	association_response_frame* association_resp_mgmt_header;
	association_resp_mgmt_header = (association_response_frame*)(pkt_buf + sizeof(mac_header_80211));
	association_resp_mgmt_header->capabilities = (CAPABILITIES_ESS | CAPABILITIES_SHORT_PREAMBLE | CAPABILITIES_SHORT_TIMESLOT);

	association_resp_mgmt_header->status_code = status;
	association_resp_mgmt_header->association_id = AID;

	txBufferPtr_u8 = (u8 *)((void *)(txBufferPtr_u8) + sizeof(mac_header_80211) + sizeof(association_response_frame));

	//http://my.safaribooksonline.com/book/networking/wireless/0596100523/4dot-802dot11-framing-in-detail/wireless802dot112-chp-4-sect-3
	//Top bit is whether or not the rate is mandatory (basic). Bottom 7 bits is in units of "number of 500kbps"
	txBufferPtr_u8[0] = 1; //Tag 1: Supported Rates
	txBufferPtr_u8[1] = 8; //tag length... doesn't include the tag itself and the tag length
	txBufferPtr_u8[2] = RATE_BASIC | (0x0C); 	//6Mbps  (BPSK,   1/2)
	txBufferPtr_u8[3] = (0x12);				 	//9Mbps  (BPSK,   3/4)
	txBufferPtr_u8[4] = RATE_BASIC | (0x18); 	//12Mbps (QPSK,   1/2)
	txBufferPtr_u8[5] = (0x24); 				//18Mbps (QPSK,   3/4)
	txBufferPtr_u8[6] = RATE_BASIC | (0x30); 	//24Mbps (16-QAM, 1/2)
	txBufferPtr_u8[7] = (0x48); 				//36Mbps (16-QAM, 3/4)
	txBufferPtr_u8[8] = (0x60); 				//48Mbps  (64-QAM, 2/3)
	txBufferPtr_u8[9] = (0x6C); 				//54Mbps  (64-QAM, 3/4)
	txBufferPtr_u8+=(8+2); //Move up to next tag



	packetLen_bytes = txBufferPtr_u8 - (u8*)(pkt_buf);

	return packetLen_bytes;
}

int wlan_create_data_frame(void* pkt_buf, u8 flags, u8* address1, u8* address2, u8* address3, u16 seq_num) {

	u8* txBufferPtr_u8;
	txBufferPtr_u8 = (u8*)pkt_buf;

	mac_header_80211* data_80211_header;
	data_80211_header = (mac_header_80211*)(txBufferPtr_u8);

	data_80211_header->frame_control_1 = MAC_FRAME_CTRL1_SUBTYPE_DATA;
	data_80211_header->frame_control_2 = flags;

	data_80211_header->duration_id = 0;

	memcpy(data_80211_header->address_1,address1,6);
	memcpy(data_80211_header->address_2,address2,6);
	memcpy(data_80211_header->address_3,address3,6);

	data_80211_header->sequence_control = ((seq_num&0xFFF)<<4);

	return sizeof(mac_header_80211);
}


