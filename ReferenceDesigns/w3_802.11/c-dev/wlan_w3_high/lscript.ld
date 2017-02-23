/****************************************************/
/*                                                  */
/* Mango 802.11 Reference Design                    */
/* Linker script for CPU High on WARP v3            */
/* Copyright (c) 2017 Mango Communications, Inc.    */
/*  All rights reserved.                            */
/****************************************************/

_STACK_SIZE = DEFINED(_STACK_SIZE) ? _STACK_SIZE : 0x1000;
_HEAP_SIZE  = DEFINED(_HEAP_SIZE)  ? _HEAP_SIZE  : 0x2000;

/* The I/DLMB for CPU High is implemented as two 128kB      */
/*  BRAM blocks. These blocks are mapped into continguous   */
/*  addresses for CPU High, allowing the linker to map      */
/*  sections into these memories as if it were a single     */
/*  memory block. The mb_high_ilmb MEMORY below represents  */
/*  the full 256kB logical I/DLMB memory area.              */

MEMORY
{
   mb_high_ilmb        : ORIGIN = 0x00000050, LENGTH = 0x0003FFB0
   mb_high_init_bram   : ORIGIN = 0x10000000, LENGTH = 0x00001000
   pkt_buff_rx_bram    : ORIGIN = 0x90000000, LENGTH = 0x00010000
   pkt_buff_tx_bram    : ORIGIN = 0x91000000, LENGTH = 0x00010000
   mb_high_aux_bram    : ORIGIN = 0xA0000000, LENGTH = 0x00010000
   ddr3_sodimm         : ORIGIN = 0xC0000000, LENGTH = 0x40000000
}

/* Specify the default entry point to the program */

ENTRY(_start)

/* Define the sections, and where they are mapped in memory */

SECTIONS
{
.vectors.reset 0x00000000 : {
   KEEP (*(.vectors.reset))
} 

.vectors.sw_exception 0x00000008 : {
   KEEP (*(.vectors.sw_exception))
} 

.vectors.interrupt 0x00000010 : {
   KEEP (*(.vectors.interrupt))
} 

.vectors.hw_exception 0x00000020 : {
   KEEP (*(.vectors.hw_exception))
} 

.text : {
   *(.text)
   *(.text.*)
   *(.gnu.linkonce.t.*)
} > mb_high_ilmb

/******************************************************************
 * CPU High Linker Data in DDR
 *
 * Since on device BRAM is being used for many different things, it 
 * becomes necessary to link data sections into DDR.  The section
 * cpu_high_ddr_linker_data is used for this data.
 *
 * However, since this section is located in DDR, which cannot be 
 * loaded by the SDK, we inlucde the (NOLOAD) directive for this 
 * section.  This means that the initial value of the section will be 
 * random (ie whatever was last in DDR).  
 *
 * If this section needs to be initialized, then the function
 * wlan_mac_high_cpu_linker_data_init() must be updated in 
 * wlan_mac_high.c.
 *
 ******************************************************************/
.wlan_exp_eth_buffers_section (NOLOAD) : {
   __wlan_exp_eth_buffers_section_start = .;
   *(.ip_udp_eth_buffers)
   *(.wlan_exp_eth_buffers)
   __wlan_exp_eth_buffers_section_end = .;
} > ddr3_sodimm

.init : {
   KEEP (*(.init))
} > mb_high_ilmb

.fini : {
   KEEP (*(.fini))
} > mb_high_ilmb

.ctors : {
   __CTOR_LIST__ = .;
   ___CTORS_LIST___ = .;
   KEEP (*crtbegin.o(.ctors))
   KEEP (*(EXCLUDE_FILE(*crtend.o) .ctors))
   KEEP (*(SORT(.ctors.*)))
   KEEP (*(.ctors))
   __CTOR_END__ = .;
   ___CTORS_END___ = .;
} > mb_high_ilmb

.dtors : {
   __DTOR_LIST__ = .;
   ___DTORS_LIST___ = .;
   KEEP (*crtbegin.o(.dtors))
   KEEP (*(EXCLUDE_FILE(*crtend.o) .dtors))
   KEEP (*(SORT(.dtors.*)))
   KEEP (*(.dtors))
   PROVIDE(__DTOR_END__ = .);
   PROVIDE(___DTORS_END___ = .);
} > mb_high_ilmb

.rodata : {
   __rodata_start = .;
   *(.rodata)
   *(.rodata.*)
   *(.gnu.linkonce.r.*)
   __rodata_end = .;
} > mb_high_ilmb

.sdata2 : {
   . = ALIGN(8);
   __sdata2_start = .;
   *(.sdata2)
   *(.sdata2.*)
   *(.gnu.linkonce.s2.*)
   . = ALIGN(8);
   __sdata2_end = .;
} > mb_high_ilmb

.sbss2 : {
   __sbss2_start = .;
   *(.sbss2)
   *(.sbss2.*)
   *(.gnu.linkonce.sb2.*)
   __sbss2_end = .;
} > mb_high_ilmb

.data : {
   . = ALIGN(4);
   __data_start = .;
   *(.data)
   *(.data.*)
   *(.gnu.linkonce.d.*)
   __data_end = .;
} > mb_high_ilmb

.got : {
   *(.got)
} > mb_high_ilmb

.got1 : {
   *(.got1)
} > mb_high_ilmb

.got2 : {
   *(.got2)
} > mb_high_ilmb

.eh_frame : {
   *(.eh_frame)
} > mb_high_ilmb

.jcr : {
   *(.jcr)
} > mb_high_ilmb

.gcc_except_table : {
   *(.gcc_except_table)
} > mb_high_ilmb

.sdata : {
   . = ALIGN(8);
   __sdata_start = .;
   *(.sdata)
   *(.sdata.*)
   *(.gnu.linkonce.s.*)
   . += 16;                  /* fix for linker false error message */
   __sdata_end = .;
} > mb_high_ilmb

.sbss (NOLOAD) : {
   . = ALIGN(4);
   __sbss_start = .;
   *(.sbss)
   *(.sbss.*)
   *(.gnu.linkonce.sb.*)
   . = ALIGN(8);
   __sbss_end = .;
} > mb_high_ilmb

.tdata : {
   __tdata_start = .;
   *(.tdata)
   *(.tdata.*)
   *(.gnu.linkonce.td.*)
   __tdata_end = .;
} > mb_high_ilmb

.tbss : {
   __tbss_start = .;
   *(.tbss)
   *(.tbss.*)
   *(.gnu.linkonce.tb.*)
   __tbss_end = .;
} > mb_high_ilmb

.bss (NOLOAD) : {
   . = ALIGN(4);
   __bss_start = .;
   *(.bss)
   *(.bss.*)
   *(.gnu.linkonce.b.*)
   *(COMMON)
   . = ALIGN(4);
   __bss_end = .;
} > mb_high_ilmb

_SDA_BASE_  = __sdata_start  + ((__sbss_end  - __sdata_start)  / 2 );

_SDA2_BASE_ = __sdata2_start + ((__sbss2_end - __sdata2_start) / 2 );

/* Generate Stack and Heap definitions */

.heap (NOLOAD) : {
   . = ALIGN(8);
   _heap = .;
   _heap_start = .;
   . += _HEAP_SIZE;
   _heap_end = .;
} > mb_high_ilmb

.stack (NOLOAD) : {
   _stack_end = .;
   . += _STACK_SIZE;
   . = ALIGN(8);
   _stack = .;
   __stack = _stack;
} > mb_high_ilmb

_end = .;
}

