/* A simple example for how to use UART to send 
 * "Go Blue" as a string. Can be viewed on Saleae.
 * NOTE that UART 3 is used in this example.
 * See the pinout diagram for info as to where UART3 is.
*/


#include "chip.h"
#include "board.h"
#include "string.h"
#include <stdio.h>

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

#if defined(BOARD_EA_DEVKIT_1788) || defined(BOARD_EA_DEVKIT_4088)
#define UART_SELECTION 	LPC_UART0
#define IRQ_SELECTION 	UART0_IRQn
#define HANDLER_NAME 	UART0_IRQHandler
#elif defined(BOARD_NXP_LPCXPRESSO_1769)
#define UART_SELECTION 	LPC_UART3
#define IRQ_SELECTION 	UART3_IRQn
#define HANDLER_NAME 	UART3_IRQHandler
#else
#error No UART selected for undefined board
#endif


/* Transmit and receive ring buffer sizes */
#define UART_SRB_SIZE 128	/* Send */
#define UART_RRB_SIZE 32	/* Receive */



/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/**
 * @brief	UART 0 interrupt handler using ring buffers
 * @return	Nothing
 */
void HANDLER_NAME(void)
{
	/* Want to handle any errors? Do it here. */

	/* Use default ring buffer handler. Override this with your own
	   code if you need more capability. */
	Chip_UART_IRQRBHandler(UART_SELECTION, &rxring, &txring);
}

int main(void) {


	uint8_t tx_buff[10] = "Go Blue";

	/* BEGIN MY CODE */
	SystemCoreClockUpdate();
	Board_Init();
	Board_UART_Init(UART_SELECTION);
	Board_LED_Set(0, false);

	/* Setup UART for 57.6k8n1 */
	Chip_UART_Init(UART_SELECTION);
	Chip_UART_SetBaud(UART_SELECTION, 57600);
	Chip_UART_ConfigData(UART_SELECTION, (UART_LCR_WLEN8 | UART_LCR_SBS_1BIT));
	Chip_UART_SetupFIFOS(UART_SELECTION, (UART_FCR_FIFO_EN | UART_FCR_TRG_LEV2));
	Chip_UART_TXEnable(UART_SELECTION);

	while(1) {
		Chip_UART_SendBlocking(UART_SELECTION, tx_buff, 10);
		for (int i = 0; i < 1000000000; i++);
	}



	return 0;
}