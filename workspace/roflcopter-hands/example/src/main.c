/*
 * Source code for roflcopter hand controllers
 * EECS 373 Winter 2017 - Team Roflcopter
 */

#include "chip.h"
#include "board.h"
#include "string.h"
#include <stdio.h>
#include <assert.h>


// Select UART Handlers
#define UART_SELECTION 		LPC_UART3
#define IRQ_SELECTION 		UART3_IRQn
#define HANDLER_NAME 		UART3_IRQHandler


#define UART_RRB_SIZE 	0x18 				// Depends on sampling rate. Don't want it to overflow
#define RIGHT_HAND_ADDR	0x415AEC15			// The LOWER address of the right hand xbee (These DO NOT change with XBEE settings!)
#define LEFT_HAND_ADDR 	0x40AE5BE4			// The LOWER address of the left hand xbee
#define ADDR_START_IDX	4					// The index of the start of the address in the XBEE packet

#define DAC_ADDRESS_0 0x62
#define DAC_ADDRESS_1 0x63
#define SPEED_100KHZ  1000000

/* axes for i2c transfer */
static uint8_t x_axis[2], y_axis[2];

// Used for i2c
static int mode_poll;
static uint8_t iox_data[2]; /* PORT0 input port, PORT1 Output port */


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
uint32_t xbeeAddress(uint8_t* data) {

	uint64_t address = 0;
	int i;
	for (i = 8; i < 12; i++) {
		address = (address << 8) | data[i];
	}

	return address;
}

void getADCSample(uint8_t* data, uint8_t sample) {
	switch (sample) {
		case X_AXIS:
			// X is the first sample, 3 and 4 indices
			x_axis[0] = data[19];
			x_axis[1] = data[20];

			// Concatenate the two samples so that you can multiply by 4 (12 bit DAC vs 10 bit sample)
			uint16_t xtemp = byteConcat(x_axis[0], x_axis[1]);
			xtemp *= 4;

			// Put the sample back into the indices
			x_axis[0] = (xtemp & 0xFF00) >> 8;
			x_axis[1] = (xtemp & 0x00FF);
			break;
		case Y_AXIS:
			// Y is the second sample, 5 and 6 indices
			y_axis[0] = data[21];
			y_axis[1] = data[22];

			uint16_t ytemp = byteConcat(y_axis[0], y_axis[1]);

			ytemp *= 4;

			y_axis[0] = (ytemp & 0xFF00) >> 8;
			y_axis[1] = (ytemp & 0x00FF);
			break;
	}


}

uint8_t validAddress(uint8_t* data) {
	return xbeeAddress(data) == RIGHT_HAND_ADDR || xbeeAddress(data) == LEFT_HAND_ADDR;
}

/* State machine handler for I2C0 and I2C1 */
static void i2c_state_handling(I2C_ID_T id)
{
	if (Chip_I2C_IsMasterActive(id)) {
		Chip_I2C_MasterStateHandler(id);
	} else {
		Chip_I2C_SlaveStateHandler(id);
	}
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

/* Update IN/OUT port states to real devices */
void i2c_iox_update_regs(int ops)
{
	if (ops & 1) { /* update out port */
		Board_LED_Set(0, iox_data[1] & 1);
		Board_LED_Set(1, iox_data[1] & 2);
		Board_LED_Set(2, iox_data[1] & 4);
		Board_LED_Set(3, iox_data[1] & 8);
	}

	if (ops & 2) { /* update in port */
		iox_data[0] = (uint8_t) Buttons_GetStatus();
	}
}

/*
 * INTERRUPT HANDLERS
 */

/**
 * @brief	UART 3 interrupt handler using ring buffers for only read ring buffers
 * @return	Nothing
 */
uint8_t UART_INT_COUNT = 0;
uint8_t UART_BYTES;
uint8_t data[24];
void HANDLER_NAME(void) {
	int i;
	for(i = 0; i < 16; i++)
	{
		data[i] = data[i+8];
	}
	i = 16;
	while (Chip_UART_ReadLineStatus(UART_SELECTION) & UART_LSR_RDR) {
		data[i++] = Chip_UART_ReadByte(UART_SELECTION);
		if (i == 24) break; // For safety
	}
}

/**
 * @brief	SysTick Interrupt Handler
 * @return	Nothing
 * @note	Systick interrupt handler updates the button status
 */
void SysTick_Handler(void)
{
	i2c_iox_update_regs(2);
}

/**
 * @brief	I2C Interrupt Handler
 * @return	None
 */
void I2C1_IRQHandler(void)
{
	i2c_state_handling(I2C1);
}

/**
 * @brief	I2C0 Interrupt handler
 * @return	None
 */
void I2C0_IRQHandler(void)
{
	i2c_state_handling(I2C0);
}





/**
 * @brief	Main program body
 * @return	Always returns 1 (returns with failure, since it should never return)
 */
int main(void)
{

	SystemCoreClockUpdate();
	Board_Init();
	Board_UART_Init(UART_SELECTION);
	Board_LED_Set(0, false);

	/* Setup UART3 for 9.6k8n1 */
	Chip_UART_Init(UART_SELECTION);
	Chip_UART_SetBaud(UART_SELECTION, 9600);
	Chip_UART_ConfigData(UART_SELECTION, (UART_LCR_WLEN8 | UART_LCR_SBS_1BIT));
	Chip_UART_SetupFIFOS(UART_SELECTION, (UART_FCR_FIFO_EN | UART_FCR_TRG_LEV2));

	/* Enable UART 3 interrupt */
	Chip_UART_IntEnable(UART_SELECTION, (UART_IER_RBRINT | UART_IER_RLSINT));

	/* I2C Init */
	i2c_app_init(I2C1, SPEED_100KHZ);
	i2c_app_init(I2C0, SPEED_100KHZ);

	/* preemption = 1, sub-priority = 1 */
	NVIC_SetPriority(IRQ_SELECTION, 1);
	NVIC_EnableIRQ(IRQ_SELECTION);


	while(1) {
		// First word is always 0x7E00, next is the size of packet, next is 0x82 (for some reason)

		if (data[0] == 0x7E && data[1] == 0x00 && data[2] == 0x14 && data[3] == 0x82) {


			// Get the ADC samples
			getADCSample(data, X_AXIS);
			getADCSample(data, Y_AXIS);

			// Output samples on I2C for the right accelerometer
			if (xbeeAddress(data) == RIGHT_HAND_ADDR) {
				Chip_I2C_SetMasterEventHandler(I2C1, Chip_I2C_EventHandler);
				int tmp = Chip_I2C_MasterSend(I2C1, DAC_ADDRESS_0, x_axis, 2);
				//assert(tmp == 2); // To ensure that both bytes are transferred

				tmp = Chip_I2C_MasterSend(I2C1, DAC_ADDRESS_1, y_axis, 2);
				//assert(tmp == 2); // To ensure that both bytes are transferred
			}

			// Output samples on I2C for the left accelerometer
			// TODO: I2C1 is not working. Only I2C0
			else if (xbeeAddress(data) == LEFT_HAND_ADDR) {
				Chip_I2C_SetMasterEventHandler(I2C0, Chip_I2C_EventHandler);
				int tmp = Chip_I2C_MasterSend(I2C0, DAC_ADDRESS_0, x_axis, 2);
				tmp = Chip_I2C_MasterSend(I2C0, DAC_ADDRESS_1, y_axis, 2);
			}

		}
	}
	return 1;
}

// Questions:
// 1. Are interrupts ever occurring when you're in the ISR?
// 2. How long is the ISR actually taking?
// 3. Try to read out a shorter sequence each time (4 bytes) ... but probably not the problem
