#ifndef WLAN_HIGH_TYPES_H_
#define WLAN_HIGH_TYPES_H_

//FIXME: wlan_exp should be updated to use this enum to report node type
typedef enum application_role_t{
	APPLICATION_ROLE_AP			= 1,
	APPLICATION_ROLE_STA		= 2,
	APPLICATION_ROLE_IBSS		= 3,
	APPLICATION_ROLE_UNKNOWN	= 0xFF
} application_role_t;

/********************************************************************
 * @brief Channel Type Enum
 *
 * This enum described the type of channel that is specified in the
 * chan_spec_t struct. The size of an enum is compiler dependent. Because
 * this enum will be used in structs whose contents must be aligned with
 * wlan_exp in Python, we use a compile time assertion to at least create
 * a compilation error if the size winds up being different than wlan_exp
 * expects.
 *
 ********************************************************************/
typedef enum __attribute__((__packed__)) {
	CHAN_TYPE_BW20 = 0,
	CHAN_TYPE_BW40_SEC_BELOW = 1,
	CHAN_TYPE_BW40_SEC_ABOVE = 2
} chan_type_t;

CASSERT(sizeof(chan_type_t) == 1, chan_type_t_alignment_check);

/********************************************************************
 * @brief Channel Specifications Struct
 *
 * This struct contains a primary channel number and a chan_type_t.
 * Together, this tuple of information can be used to calculate the
 * center frequency and bandwidth of the radio.
 ********************************************************************/
typedef struct __attribute__((__packed__)){
	u8             chan_pri;
	chan_type_t    chan_type;
} chan_spec_t;
CASSERT(sizeof(chan_spec_t) == 2, chan_spec_t_alignment_check);


#endif /* WLAN_HIGH_TYPES_H_ */
