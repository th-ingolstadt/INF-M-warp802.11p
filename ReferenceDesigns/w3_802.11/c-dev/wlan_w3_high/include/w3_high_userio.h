#ifndef W3_HIGH_USERIO_H_
#define W3_HIGH_USERIO_H_

#include "xgpio.h"
#include "xintc.h"

#define GPIO_USERIO_INPUT_CHANNEL            1                                  ///< Channel used as input user IO inputs (buttons, DIP switch)
#define GPIO_USERIO_INPUT_IR_CH_MASK         XGPIO_IR_CH1_MASK                  ///< Mask for enabling interrupts on GPIO input

#define GPIO_MASK_DRAM_INIT_DONE                           0x00000100                         ///< Mask for GPIO -- DRAM initialization bit
#define GPIO_MASK_PB_U                                     0x00000040                         ///< Mask for GPIO -- "Up" Pushbutton
#define GPIO_MASK_PB_M                                     0x00000020                         ///< Mask for GPIO -- "Middle" Pushbutton
#define GPIO_MASK_PB_D                                     0x00000010                         ///< Mask for GPIO -- "Down" Pushbutton
#define GPIO_MASK_DS_3                                     0x00000008                         ///< Mask for GPIO -- MSB of Dip Switch

int  w3_high_userio_init();
int  w3_high_userio_setup_interrupt(XIntc* intc);

#endif /* W3_HIGH_USERIO_H_ */
