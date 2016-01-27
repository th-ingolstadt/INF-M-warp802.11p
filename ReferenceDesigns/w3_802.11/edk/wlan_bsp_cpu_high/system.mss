
 PARAMETER VERSION = 2.2.0


BEGIN OS
 PARAMETER OS_NAME = standalone
 PARAMETER OS_VER = 3.08.a
 PARAMETER PROC_INSTANCE = mb_high
 PARAMETER STDIN = mb_high_uart
 PARAMETER STDOUT = mb_high_uart
END


BEGIN PROCESSOR
 PARAMETER DRIVER_NAME = cpu
 PARAMETER DRIVER_VER = 1.14.a
 PARAMETER HW_INSTANCE = mb_high
END


BEGIN DRIVER
 PARAMETER DRIVER_NAME = axicdma
 PARAMETER DRIVER_VER = 2.01.a
 PARAMETER HW_INSTANCE = axi_cdma_0
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = sysmon
 PARAMETER DRIVER_VER = 5.02.a
 PARAMETER HW_INSTANCE = axi_sysmon_adc_0
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = v6_ddrx
 PARAMETER DRIVER_VER = 2.00.a
 PARAMETER HW_INSTANCE = ddr3_sodimm
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = axidma
 PARAMETER DRIVER_VER = 7.01.a
 PARAMETER HW_INSTANCE = eth_a_dma
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = axiethernet
 PARAMETER DRIVER_VER = 3.00.a
 PARAMETER HW_INSTANCE = eth_a_mac
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = axidma
 PARAMETER DRIVER_VER = 7.01.a
 PARAMETER HW_INSTANCE = eth_b_dma
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = axiethernet
 PARAMETER DRIVER_VER = 3.00.a
 PARAMETER HW_INSTANCE = eth_b_mac
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = bram
 PARAMETER DRIVER_VER = 3.01.a
 PARAMETER HW_INSTANCE = mb_high_aux_bram_ctrl
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = bram
 PARAMETER DRIVER_VER = 3.01.a
 PARAMETER HW_INSTANCE = mb_high_dlmb_bram_cntlr_0
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = bram
 PARAMETER DRIVER_VER = 3.01.a
 PARAMETER HW_INSTANCE = mb_high_dlmb_bram_cntlr_1
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = bram
 PARAMETER DRIVER_VER = 3.01.a
 PARAMETER HW_INSTANCE = mb_high_ilmb_bram_cntlr_0
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = bram
 PARAMETER DRIVER_VER = 3.01.a
 PARAMETER HW_INSTANCE = mb_high_ilmb_bram_cntlr_1
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = bram
 PARAMETER DRIVER_VER = 3.01.a
 PARAMETER HW_INSTANCE = mb_high_init_bram_ctrl
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = intc
 PARAMETER DRIVER_VER = 2.05.a
 PARAMETER HW_INSTANCE = mb_high_intc
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = gpio
 PARAMETER DRIVER_VER = 3.00.a
 PARAMETER HW_INSTANCE = mb_high_sw_gpio
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = tmrctr
 PARAMETER DRIVER_VER = 2.04.a
 PARAMETER HW_INSTANCE = mb_high_timer
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = uartlite
 PARAMETER DRIVER_VER = 2.00.a
 PARAMETER HW_INSTANCE = mb_high_uart
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = mbox
 PARAMETER DRIVER_VER = 3.02.a
 PARAMETER HW_INSTANCE = mb_mailbox
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = bram
 PARAMETER DRIVER_VER = 3.01.a
 PARAMETER HW_INSTANCE = pkt_buff_rx_bram_ctrl
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = bram
 PARAMETER DRIVER_VER = 3.01.a
 PARAMETER HW_INSTANCE = pkt_buff_tx_bram_ctrl
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = mutex
 PARAMETER DRIVER_VER = 3.01.a
 PARAMETER HW_INSTANCE = pkt_buffer_mutex
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = w3_iic_eeprom_axi
 PARAMETER DRIVER_VER = 1.02.a
 PARAMETER HW_INSTANCE = w3_iic_eeprom_onboard
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = w3_userio_axi
 PARAMETER DRIVER_VER = 1.01.a
 PARAMETER HW_INSTANCE = w3_userio
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = wlan_mac_time_hw_axiw
 PARAMETER DRIVER_VER = 1.00.d
 PARAMETER HW_INSTANCE = wlan_mac_time_hw
END


BEGIN LIBRARY
 PARAMETER LIBRARY_NAME = WARP_HW_VER
 PARAMETER LIBRARY_VER = 3.10.a
 PARAMETER PROC_INSTANCE = mb_high
END

BEGIN LIBRARY
 PARAMETER LIBRARY_NAME = WARP_ip_udp
 PARAMETER LIBRARY_VER = 1.00.a
 PARAMETER PROC_INSTANCE = mb_high
 PARAMETER ETH_A_USES_LIBRARY = 0
 PARAMETER NUM_RX_BUFFERS = 3
 PARAMETER NUM_TX_BUFFERS = 2
END

