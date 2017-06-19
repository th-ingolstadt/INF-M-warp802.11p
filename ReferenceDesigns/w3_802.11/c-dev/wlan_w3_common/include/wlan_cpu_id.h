#ifndef WLAN_CPU_ID_H_
#define WLAN_CPU_ID_H_

// CPU IDs
#ifdef XPAR_MB_HIGH_FREQ
#define WLAN_COMPILE_FOR_CPU_HIGH                          1
#define WLAN_COMPILE_FOR_CPU_LOW                           0
#endif

#ifdef XPAR_MB_LOW_FREQ
#define WLAN_COMPILE_FOR_CPU_HIGH                          0
#define WLAN_COMPILE_FOR_CPU_LOW                           1
#endif

#endif /* WLAN_CPU_ID_H_ */
