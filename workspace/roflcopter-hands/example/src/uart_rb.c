/*
 * Source code for roflcopter hand controllers
 * EECS 373 Winter 2017 - Team Roflcopter
 */

#include "chip.h"
#include "board.h"
#include "string.h"
#include <stdio.h>

// I2C Polling Speeds
#define SPEED_100KHZ        100000
#define SPEED_400KHZ        400000

// I2C Slave Address
#define CAP_ADDR			0x28 // The easiest address to set is 0x28 which is just a wire from AD to the 3Vo pin. (adafruit p.12)

// Select UART Handlers
#define UART_SELECTION 		LPC_UART3
#define IRQ_SELECTION 		UART3_IRQn
#define HANDLER_NAME 		UART3_IRQHandler
#define UART_SELECTION_2 	LPC_UART2
#define IRQ_SELECTION_2 	UART2_IRQn
#define HANDLER_NAME_2 		UART2_IRQHandler

#define I2C_SLVTXBUFF_SIZE 20 /* Slave send */

static int mode_poll; /* Poll/Interrupt mode flag */
static volatile uint8_t i2c_txbuff[I2C_SLVTXBUFF_SIZE]; // NOTE: This is volatile because it is set in an ISR

/***********************
 * I2C PRIVATE FUNCTIONS
 ***********************/

/* Set I2C mode to polling/interrupt */
static void i2c_set_mode(I2C_ID_T id, int polling)
{
	if(!polling) {
		mode_poll &= ~(1 << id);
		Chip_I2C_SetMasterEventHandler(id, Chip_I2C_EventHandler);
		NVIC_EnableIRQ(id == I2C0 ? I2C0_IRQn : I2C1_IRQn);
	} else {
		mode_poll |= 1 << id;
		NVIC_DisableIRQ(id == I2C0 ? I2C0_IRQn : I2C1_IRQn);
		Chip_I2C_SetMasterEventHandler(id, Chip_I2C_EventHandlerPolling);
	}
}

/* Initialize the I2C bus */
static void i2c_app_init(I2C_ID_T id, int speed)
{
	Board_I2C_Init(id);

	/* Initialize I2C */
	Chip_I2C_Init(id);
	Chip_I2C_SetClockRate(id, speed);

	/* Set default mode to interrupt */
	i2c_set_mode(id, 0);
}

void I2C0_IRQHandler(void) {

	// Call the handler for the slave if the capacitive buttons were pressed.
	// This reads in the data from the slave into the txbuff.

	Chip_I2C_SlaveStateHandler(I2C0);
}


/**
 * @brief	Main program body
 * @return	Always returns 1 (returns with failure)
 */
int main(void)
{

	/* Transfer buffer for UART */
	uint8_t uart_tx_buff[10] = "TEST";

	SystemCoreClockUpdate();
	Board_Init();
	Board_UART_Init(UART_SELECTION);
	Board_UART_Init(UART_SELECTION_2);
	Board_LED_Set(0, false);

	/* Setup UART1 for 9.6k8n1 */
	Chip_UART_Init(UART_SELECTION);
	Chip_UART_SetBaud(UART_SELECTION, 9600);
	Chip_UART_ConfigData(UART_SELECTION, (UART_LCR_WLEN8 | UART_LCR_SBS_1BIT));
	Chip_UART_SetupFIFOS(UART_SELECTION, (UART_FCR_FIFO_EN | UART_FCR_TRG_LEV2));
	Chip_UART_TXEnable(UART_SELECTION);

	/* Set up I2C0 */
	i2c_app_init(I2C0, SPEED_100KHZ);

	/* Set up I2C Slave */
	I2C_XFER_T xfer;
	xfer.slaveAddr = CAP_ADDR;				// Capacitive Sensor Address
	xfer.txBuff = (uint8_t*)&i2c_txbuff;	// The buffer to store what it sends
	xfer.txSz = (int)sizeof(i2c_txbuff);	// How large the buffer is
	xfer.rxBuff = 0;						// Not receiving anything
	xfer.rxSz = 0;							// Not receiving anything

	Chip_I2C_SlaveSetup(I2C0, I2C_SLAVE_0, &xfer, (I2C_EVENTHANDLER_T)I2C_EVENT_SLAVE_TX, 0);

	/* Set up I2C Interrupts for Cap Sensors */
	NVIC_SetPriority(I2C0_IRQn, 1);
	NVIC_EnableIRQ(I2C0_IRQn);

	while(1) {

		Chip_UART_SendBlocking(UART_SELECTION, uart_txbuff, 10);

	}



	return 1;
}

/**
 * @}
 */
