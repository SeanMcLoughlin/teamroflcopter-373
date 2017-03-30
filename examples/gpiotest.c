#include "board.h"
#include "gpio_17xx_40xx.h"

int main(void)
{
	//Init
	SystemCoreClockUpdate();
	Board_Init();

	uint8_t port = 2;
	uint8_t pin = 13;

	Chip_GPIO_SetPinDIROutput(LPC_GPIO, port, pin);
	Chip_GPIO_SetPinState(LPC_GPIO, port, pin, 0);
	Chip_GPIO_SetPinState(LPC_GPIO, port, pin, 1);
	Chip_GPIO_SetPinState(LPC_GPIO, port, pin, 0);

	return 0;
}
