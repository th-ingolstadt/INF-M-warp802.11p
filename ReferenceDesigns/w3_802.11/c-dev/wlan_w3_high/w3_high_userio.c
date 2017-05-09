#include "xgpio.h"

#include "wlan_mac_high.h"
#include "w3_high_userio.h"
#include "w3_high.h"
#include "xparameters.h"

#include "wlan_mac_common.h"

static XGpio                 Gpio_userio;                  ///< GPIO driver instance for user IO inputs
static u32					 gl_userio_state;

// Private functions
void _w3_high_userio_gpio_handler(void *InstancePtr);

int w3_high_userio_init(){
	int return_value;

	gl_userio_state = 0;

	// Initialize the GPIO driver instances
	return_value = XGpio_Initialize(&Gpio_userio, PLATFORM_DEV_ID_USRIO_GPIO);

	if (return_value != XST_SUCCESS) {
		wlan_printf(PL_ERROR, "ERROR: Could not initialize GPIO\n");
		return return_value;
	}

	// Set direction of GPIO channels
	//  User IO GPIO instance has 1 channel, all inputs
	XGpio_SetDataDirection(&Gpio_userio, GPIO_USERIO_INPUT_CHANNEL, 0xFFFFFFFF);

	return return_value;
}
int w3_high_userio_setup_interrupt(XIntc* intc){
	int return_value;
	// GPIO for User IO inputs
	return_value = XIntc_Connect(intc, PLATFORM_INT_ID_USRIO_GPIO, (XInterruptHandler)_w3_high_userio_gpio_handler, &Gpio_userio);
	if (return_value != XST_SUCCESS) {
		wlan_printf(PL_ERROR, "Failed to set up GPIO interrupt\n");
		return return_value;
	}

	//Update the initial state of the global tracking variable
	gl_userio_state = wlan_platform_userio_get_state();

	XIntc_Enable(intc, PLATFORM_INT_ID_USRIO_GPIO);
	XGpio_InterruptEnable(&Gpio_userio, GPIO_USERIO_INPUT_IR_CH_MASK);
	XGpio_InterruptGlobalEnable(&Gpio_userio);
	return return_value;
}

void _w3_high_userio_gpio_handler(void *InstancePtr){
	XGpio *GpioPtr;
	u32	curr_userio_state, userio_state_xor;

	GpioPtr = (XGpio *)InstancePtr;

	XGpio_InterruptDisable(GpioPtr, GPIO_USERIO_INPUT_IR_CH_MASK);

	curr_userio_state = wlan_platform_userio_get_state();

	userio_state_xor = curr_userio_state ^ gl_userio_state;

	if(userio_state_xor & USERIO_INPUT_MASK_PB_0) wlan_mac_high_userio_inputs_callback(curr_userio_state & USERIO_INPUT_MASK_PB_0, USERIO_INPUT_MASK_PB_0);
	if(userio_state_xor & USERIO_INPUT_MASK_PB_1) wlan_mac_high_userio_inputs_callback(curr_userio_state & USERIO_INPUT_MASK_PB_1, USERIO_INPUT_MASK_PB_1);
	if(userio_state_xor & USERIO_INPUT_MASK_PB_2) wlan_mac_high_userio_inputs_callback(curr_userio_state & USERIO_INPUT_MASK_PB_2, USERIO_INPUT_MASK_PB_2);
	if(userio_state_xor & USERIO_INPUT_MASK_PB_3) wlan_mac_high_userio_inputs_callback(curr_userio_state & USERIO_INPUT_MASK_PB_3, USERIO_INPUT_MASK_PB_3);
	if(userio_state_xor & USERIO_INPUT_MASK_SW_0) wlan_mac_high_userio_inputs_callback(curr_userio_state & USERIO_INPUT_MASK_SW_0, USERIO_INPUT_MASK_SW_0);
	if(userio_state_xor & USERIO_INPUT_MASK_SW_1) wlan_mac_high_userio_inputs_callback(curr_userio_state & USERIO_INPUT_MASK_SW_1, USERIO_INPUT_MASK_SW_1);
	if(userio_state_xor & USERIO_INPUT_MASK_SW_2) wlan_mac_high_userio_inputs_callback(curr_userio_state & USERIO_INPUT_MASK_SW_2, USERIO_INPUT_MASK_SW_2);
	if(userio_state_xor & USERIO_INPUT_MASK_SW_3) wlan_mac_high_userio_inputs_callback(curr_userio_state & USERIO_INPUT_MASK_SW_3, USERIO_INPUT_MASK_SW_3);

	gl_userio_state = curr_userio_state;


	(void)XGpio_InterruptClear(GpioPtr, GPIO_USERIO_INPUT_IR_CH_MASK);
	XGpio_InterruptEnable(GpioPtr, GPIO_USERIO_INPUT_IR_CH_MASK);
	return;
}



