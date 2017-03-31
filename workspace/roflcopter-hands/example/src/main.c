/*
 * Source code for roflcopter hand controllers
 * EECS 373 Winter 2017 - Team Roflcopter
 */

#include "chip.h"
#include "board.h"
#include "string.h"
#include <stdio.h>


// Select UART Handlers
#define UART_SELECTION 		LPC_UART3
#define IRQ_SELECTION 		UART3_IRQn
#define HANDLER_NAME 		UART3_IRQHandler

/* Receive buffer for the UART signals */
/* Contains all the signals for ADC */
STATIC RINGBUFF_T rxring;

#define UART_RRB_SIZE 	0x18 				// Depends on sampling rate. Don't want it to overflow
#define RIGHT_HAND_ADDR	0x0013A200415AEC15	// The address of the right hand xbee (These DO NOT change with XBEE settings!)
#define LEFT_HAND_ADDR 	0x0013A20040AE5BE4	// The address of the left hand xbee
#define ADDR_START_IDX	4					// The index of the start of the address in the XBEE packet

#define DAC_ADDRESS_0 0x62
#define DAC_ADDRESS_1 0x63
#define SPEED_100KHZ  1000000

/* Receive buffer */
static uint8_t rxbuff[UART_RRB_SIZE];

/* axes for i2c transfer */
static uint8_t x_axis[2], y_axis[2];

// Used for i2c
static int mode_poll;

enum adcsample_t	{X_AXIS = 0, Y_AXIS = 1};

/*
 * PRIVATE FUNCTIONS
 */

/*
 *  @brief	Concatenate two bytes into a word. First parameter becomes MSBs
 *  @return word made from concatenation
 */
uint16_t byteConcat(uint8_t byte1, uint8_t byte2) {
	return (byte1 << 8) | byte2;
}

/*
 * 	@brief	Given the data packet, find the xbee address
 * 	@return	the XBEE address
 */
uint64_t xbeeAddress(uint8_t* data) {

	uint64_t address = 0;
	int i;
	for (i = ADDR_START_IDX; i < ADDR_START_IDX + 8; i++) {
		address = (address << 8) | data[i];
	}

	return address;
}

void getADCSample(uint8_t* data, uint8_t sample) {
	switch (sample) {
		case X_AXIS:
			// 5 and 4 because the X axis is the first of two ADC samples
			x_axis[0] = data[UART_RRB_SIZE-5];
			x_axis[1] = data[UART_RRB_SIZE-4];
			break;
		case Y_AXIS:
			// 3 and 2 because the Y axis is the second of the two ADC samples
			y_axis[0] = data[UART_RRB_SIZE-3];
			y_axis[1] = data[UART_RRB_SIZE-2];
			break;
	}


}

uint8_t validAddress(uint8_t* data) {
	return xbeeAddress(data) == RIGHT_HAND_ADDR || xbeeAddress(data) == LEFT_HAND_ADDR;
}


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

/*
 * INTERRUPT HANDLERS
 */

/**
 * @brief	UART 3 interrupt handler using ring buffers for only read ring buffers
 * @return	Nothing
 */
void HANDLER_NAME(void) {
	Chip_UART_RXIntHandlerRB(UART_SELECTION, &rxring);
}





/**
 * @brief	Main program body
 * @return	Always returns 1 (returns with failure, since it should never return)
 */
int main(void)
{

	/* things to read the UART Ring buffer */
	uint8_t data[UART_RRB_SIZE];

	SystemCoreClockUpdate();
	Board_Init();
	Board_UART_Init(UART_SELECTION);
	Board_LED_Set(0, false);

	/* Setup UART3 for 9.6k8n1 */
	Chip_UART_Init(UART_SELECTION);
	Chip_UART_SetBaud(UART_SELECTION, 9600);
	Chip_UART_ConfigData(UART_SELECTION, (UART_LCR_WLEN8 | UART_LCR_SBS_1BIT));
	Chip_UART_SetupFIFOS(UART_SELECTION, (UART_FCR_FIFO_EN | UART_FCR_TRG_LEV3)); // TODO: The trigger level is the problem. We need UART to trigger after it reads the WHOLE packet. Otherwise it won't get the ADC samples or the whole address

	/* Init the read ring buffer */
	RingBuffer_Init(&rxring, rxbuff, 1, UART_RRB_SIZE);

	/* Enable ring buffer UART 3 interrupt */
	Chip_UART_IntEnable(UART_SELECTION, (UART_IER_RBRINT | UART_IER_RLSINT));

	/* preemption = 1, sub-priority = 1 */
	NVIC_SetPriority(IRQ_SELECTION, 1);
	NVIC_EnableIRQ(IRQ_SELECTION);

	/* I2C Init */
	i2c_app_init(I2C2, SPEED_100KHZ);
	i2c_app_init(I2C1, SPEED_100KHZ);

	while(1) {

		int bytes; // Used for return values of serial comm functions

		// TODO: bytes is always 8 and I'm only reading 8 bytes correctly
		bytes = Chip_UART_ReadRB(UART_SELECTION, &rxring, &data, sizeof(data)-1);

		// First word is always 0x7E00, next is the size of packet, next is 0x82 (for some reason)
		if (bytes > 0 && byteConcat(data[0], data[1]) == 0x7E00 && byteConcat(data[2], data[3]) == 0x1482 /*&& validAddress(data)*/) {

			// Get the ADC samples
			getADCSample(data, X_AXIS);
			getADCSample(data, Y_AXIS);

			// Output samples on I2C for the right accelerometer
			if (xbeeAddress(data) == RIGHT_HAND_ADDR) {
				bytes = Chip_I2C_MasterSend(I2C1, DAC_ADDRESS_0, x_axis, 2);
				bytes = Chip_I2C_MasterSend(I2C1, DAC_ADDRESS_1, y_axis, 2);
			}

			// Output samples on I2C for the left accelerometer
			else if (xbeeAddress(data) == LEFT_HAND_ADDR) {
				bytes = Chip_I2C_MasterSend(I2C2, DAC_ADDRESS_0, x_axis, 2);
				bytes = Chip_I2C_MasterSend(I2C2, DAC_ADDRESS_1, y_axis, 2);
			}

		}
	}



	return 1;
}
