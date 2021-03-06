/*
 * Source code for roflcopter hand controllers
 * EECS 373 Winter 2017 - Team Roflcopter
 */

#include "chip.h"
#include "board.h"
#include "gpio_17xx_40xx.h"
#include "gpioint_17xx_40xx.h"
#include "string.h"
#include <stdio.h>
#include <assert.h>
#include <math.h>

#ifndef max
    #define max(a,b) ((a) > (b) ? (a) : (b))
#endif

/* Select UART Handlers */
#define UART_SELECTION 		LPC_UART3
#define IRQ_SELECTION 		UART3_IRQn
#define HANDLER_NAME 		UART3_IRQHandler


#define UART_RRB_SIZE 	0x18 				// Depends on sampling rate. Don't want it to overflow
#define RIGHT_HAND_ADDR	0x416A1E08			// The LOWER address of the right hand xbee (These DO NOT change with XBEE settings!)
#define LEFT_HAND_ADDR 	0x40AE5BE4			// The LOWER address of the left hand xbee
#define ADDR_START_IDX	4					// The index of the start of the address in the XBEE packet

/* On the LPC1769, the GPIO interrupts share the EINT3 vector. */
#define GPIO_IRQ_HANDLER  			EINT3_IRQHandler/* GPIO interrupt IRQ function name */
#define GPIO_INTERRUPT_NVIC_NAME    EINT3_IRQn		/* GPIO interrupt NVIC interrupt name */
#define GPIO_INTERRUPT_PIN     		13				/* GPIO pin number mapped to interrupt */
#define GPIO_INTERRUPT_PORT    		GPIOINT_PORT2	/* GPIO port number mapped to interrupt */

/* I2C DAC Addresses and such */
#define DAC_ADDRESS_0	0x62
#define DAC_ADDRESS_1	0x63
#define SPEED_100KHZ 	1000000

// This threshold is so that it is easier to keep the drone balanced with the right hand.
// That is, if your hand is close to 1.5V, it will output 1.5V.
// This makes the drone substantially easier to control, but sacrifices some resolution
// in the speed that you can move it. This is fine, as it makes it
#define RIGHT_HAND_X_THRESHOLD 	0x07FF	// 1.5V scaled for 12 bit DAC
#define RIGHT_HAND_Y_THRESHOLD 	0x07FF	// 1.5V scaled for 12 bit DAC
#define THRESH_VARIANCE			1300	//

// Hystresis to stop stuttering at the turn-on threshold
#define HYSTRESIS_UPPER_THRESHOLD 1000		// Turn on Threshold
#define HYSTRESIS_LOWER_THRESHOLD 500		// Turn off Threshold
uint8_t on_flag = 0;

/* axes for i2c transfer */
static uint8_t x_axis[2], y_axis[2];

/* Variables for I2C */
static int mode_poll;
static uint8_t iox_data[2]; /* PORT0 input port, PORT1 Output port */

// A kill switch if things go wrong
uint8_t killSwitchFlag = 0;

// Enum for sample types
enum adcsample_t	{X_AXIS = 0, Y_AXIS = 1};

// Ring buffers for averaging the data to make smooth transitions
#define RINGBUFFSIZE 16
static RINGBUFF_T x_left_ring, y_left_ring, x_right_ring, y_right_ring;
static uint16_t x_left_buff[RINGBUFFSIZE], y_left_buff[RINGBUFFSIZE], x_right_buff[RINGBUFFSIZE], y_right_buff[RINGBUFFSIZE];
static uint64_t x_left_sum, y_left_sum, x_right_sum, y_right_sum;

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

/**
 * @brief	Handle interrupt from GPIO pin or GPIO pin mapped to PININT
 * @return	Nothing
 */
void GPIO_IRQ_HANDLER(void)
{
	Chip_GPIOINT_ClearIntStatus(LPC_GPIOINT, GPIO_INTERRUPT_PORT, 1 << GPIO_INTERRUPT_PIN);
	killSwitchFlag = ~killSwitchFlag; //Toggle disable flag for ignoring right hand
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


void getADCSample(uint8_t* data, uint8_t sample, uint64_t address) {
	switch (sample) {
		case X_AXIS:
			// X is the first sample, 3 and 4 indices
			x_axis[0] = data[21];
			x_axis[1] = data[22];

			// Concatenate the two samples so that you can multiply by 4 (12 bit DAC vs 10 bit sample)

			// Concatenate the two bytes to one uint16
			uint16_t xtemp = byteConcat(x_axis[0], x_axis[1]);

			// Multiply by 4 for scaling for a 4 bit dac
			xtemp *= 4;

			// If the ring buffer is full, pop a value and subtract it from
			// the sum. Then add the new value in and add it to the sum.
			// Otherwise, don't pop and just continue.
			// Provides O(1) averaging.
			if (address == LEFT_HAND_ADDR) {

				// Scale the range that the accelerometer outputs with linear mapping
				xtemp = (xtemp - ceil(4095 * (0.4/3.3)))*ceil(3.0/2.4); // 0.4 is min, 2.4 is max - min

				// Detect integer overflow and don't let it loop around to 0
				if ((int)xtemp > 4095)
					xtemp = 4095;

				uint8_t insertbool = RingBuffer_Insert(&x_left_ring, &xtemp);
				if (!insertbool) {
					uint16_t value;
					RingBuffer_Pop(&x_left_ring, &value);
					x_left_sum -= value;
					x_left_sum += xtemp;
					value = RingBuffer_Insert(&x_left_ring, &xtemp);
				}
				else {
					x_left_sum += xtemp;
				}

				// Average the ring buffer;
				xtemp = x_left_sum / RINGBUFFSIZE;
			}
			else if (address == RIGHT_HAND_ADDR) {

				// Scale the range that the accelerometer outputs with linear mapping
				xtemp = ceil(xtemp - (4095 * (0.9/3.3)))*(3.0/1.3);

				// Detect integer overflow and don't let it loop around to 0
				if ((int)xtemp > 4095)
					xtemp = 4095;

				uint8_t insertbool = RingBuffer_Insert(&x_right_ring, &xtemp);
				if (!insertbool) {
					uint16_t value;
					RingBuffer_Pop(&x_right_ring, &value);
					x_right_sum -= value;
					x_right_sum += xtemp;
					value = RingBuffer_Insert(&x_right_ring, &xtemp);
				}
				else {
					x_right_sum += xtemp;
				}

				// Average the ring buffer;
				xtemp = x_right_sum / RINGBUFFSIZE;


				// Keep a threshold range to make sure that you can keep the drone balanced.
				if ((RIGHT_HAND_X_THRESHOLD+THRESH_VARIANCE > xtemp) &&(RIGHT_HAND_X_THRESHOLD-THRESH_VARIANCE < xtemp)){
					if (xtemp>RIGHT_HAND_X_THRESHOLD)
					{
						uint16_t tmp = xtemp - RIGHT_HAND_X_THRESHOLD;
						tmp /= 4;
						xtemp = RIGHT_HAND_X_THRESHOLD+tmp;
					}
					else {
						uint16_t tmp = RIGHT_HAND_X_THRESHOLD - xtemp;
						tmp /= 4;
						xtemp = RIGHT_HAND_X_THRESHOLD-tmp;
					}
				}
			}

			// Detect integer overflow and don't let it loop around to 0
			if ((int)xtemp > 4095)
				xtemp = 4095;

			// Invert the x axis
			xtemp = ~xtemp & 0x0FFF;


			// Put the sample back into the indices
			x_axis[0] = (xtemp & 0xFF00) >> 8;
			x_axis[1] = (xtemp & 0x00FF);


			break;
		case Y_AXIS:
			// Y is the second sample, 5 and 6 indices
			y_axis[0] = data[19];
			y_axis[1] = data[20];

			uint16_t ytemp = byteConcat(y_axis[0], y_axis[1]);
			ytemp *= 4;

			// If the ring buffer is full, pop a value and subtract it from
			// the sum. Then add the new value in and add it to the sum.
			// Otherwise, don't pop and just continue.
			// Provides O(1) averaging.
			if (address == LEFT_HAND_ADDR) {

				ytemp = max(ytemp, ceil(4095*(0.9/3.3)) + 100);
				ytemp = ceil(ytemp - (4095 * (0.9/3.3)))*(3.0/1.5);

				// Detect integer overflow and don't let it loop around to 0
				if ((int)ytemp > 4095)
					ytemp = 4095;

				uint8_t insertbool = RingBuffer_Insert(&y_left_ring, &ytemp);
				if (!insertbool) {
					uint16_t value;
					RingBuffer_Pop(&y_left_ring, &value);
					y_left_sum -= value;
					y_left_sum += ytemp;
					value = RingBuffer_Insert(&y_left_ring, &ytemp);
				}
				else {
					y_left_sum += ytemp;
				}

				// Average the ring buffer;
				ytemp = y_left_sum / RINGBUFFSIZE;

				// Apply hystresis to the left hand Y axis to stop drone stuttering
				if (ytemp >= HYSTRESIS_UPPER_THRESHOLD)
					on_flag = 1;
				else if (ytemp <= HYSTRESIS_LOWER_THRESHOLD)
					on_flag = 0;

				if (!on_flag) {
					ytemp = 0;
				}
			}
			else if (address == RIGHT_HAND_ADDR) {

				ytemp = max(ytemp, ceil(4095*(0.9/3.3)) + 100);
				ytemp = ceil(ytemp - (4095 * (0.9/3.3)))*(3.3/1.3);

				// Detect integer overflow and don't let it loop around to 0
				if ((int)ytemp > 4095)
					ytemp = 4095;

				uint8_t insertbool = RingBuffer_Insert(&y_right_ring, &ytemp);
				if (!insertbool) {
					uint16_t value;
					RingBuffer_Pop(&y_right_ring, &value);
					y_right_sum -= value;
					y_right_sum += ytemp;
					value = RingBuffer_Insert(&y_right_ring, &ytemp);
				}
				else {
					y_right_sum += ytemp;
				}
				// Average the ring buffer;
				ytemp = y_right_sum / RINGBUFFSIZE;

				if ((RIGHT_HAND_Y_THRESHOLD+THRESH_VARIANCE > ytemp) &&(RIGHT_HAND_Y_THRESHOLD-THRESH_VARIANCE < ytemp)){
					if (ytemp>RIGHT_HAND_Y_THRESHOLD)
					{
						uint16_t tmp = ytemp - RIGHT_HAND_Y_THRESHOLD;
						tmp /= 4;
						ytemp = RIGHT_HAND_Y_THRESHOLD+tmp;
					}
					else {
						uint16_t tmp = RIGHT_HAND_Y_THRESHOLD - ytemp;
						tmp /= 4;
						ytemp = RIGHT_HAND_Y_THRESHOLD-tmp;
					}
				}
			}

			// Detect integer overflow and don't let it loop around to 0
			if ((int)ytemp > 4095)
				ytemp = 4095;

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

	// Shift all data over by 8 bytes
	int i;
	for(i = 0; i < 16; i++)
	{
		data[i] = data[i+8];
	}
	i = 16;

	// Read in the new data from the UART buffer
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
 * @brief	I2C1 Interrupt Handler
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
	Board_SystemInit();
	Board_UART_Init(UART_SELECTION);

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

	/* Initialize GPIO */
	Chip_GPIO_Init(LPC_GPIO);
	Chip_GPIO_SetPinDIRInput(LPC_GPIO, GPIO_INTERRUPT_PORT, GPIO_INTERRUPT_PIN);	 		// Set P2[13] as an input (the bottom-rightmost pin)
	Chip_GPIOINT_SetIntFalling(LPC_GPIOINT, GPIO_INTERRUPT_PORT, 1 << GPIO_INTERRUPT_PIN);	// Set to falling edge interrupt

	/* preemption = 2, sub-priority = 2 */
	NVIC_SetPriority(GPIO_INTERRUPT_NVIC_NAME, 2);
	NVIC_EnableIRQ(GPIO_INTERRUPT_NVIC_NAME);

	// Initialize ring buffers (used for smoothness)
	RingBuffer_Init(&x_left_ring, x_left_buff, 2, RINGBUFFSIZE);
	RingBuffer_Init(&y_left_ring, y_left_buff, 2, RINGBUFFSIZE);
	RingBuffer_Init(&x_right_ring, x_right_buff, 2, RINGBUFFSIZE);
	RingBuffer_Init(&y_right_ring, y_right_buff, 2, RINGBUFFSIZE);

	uint8_t latch[24]; 			// The latched value of data (since we are shifting data in)

	while(1) {

		uint64_t address;

		// First word is always 0x7E00, next is the size of packet, next is 0x82 to determine packet type
		if (data[0] == 0x7E && data[1] == 0x00 && data[2] == 0x14 && data[3] == 0x82) {

			// Latch the data wire so that we can read from it without other interrupts ruining the data
			int i;
			for (i = 0; i < 24; i++)
				latch[i] = data[i];

			// Extract the address
			address = xbeeAddress(latch);

			// Extract the ADC samples
			getADCSample(latch, X_AXIS, address);
			getADCSample(latch, Y_AXIS, address);

		}

		// Output samples on I2C1 for the right accelerometer
		if (address == RIGHT_HAND_ADDR) {

			Chip_I2C_SetMasterEventHandler(I2C1, Chip_I2C_EventHandler);
			Chip_I2C_MasterSend(I2C1, DAC_ADDRESS_0, x_axis, 2);
			Chip_I2C_MasterSend(I2C1, DAC_ADDRESS_1, y_axis, 2);
		}

		// Output samples on I2C0 for the left accelerometer
		else if (address == LEFT_HAND_ADDR) {


			// This is a kill switch so that we can turn off the drone if it freaks out
			if (killSwitchFlag) {
				x_axis[0] = 0x00;
				x_axis[1] = 0x00;
				y_axis[0] = 0x00;
				y_axis[1] = 0x00;
			}

			Chip_I2C_SetMasterEventHandler(I2C0, Chip_I2C_EventHandler);
			Chip_I2C_MasterSend(I2C0, DAC_ADDRESS_0, x_axis, 2);
			Chip_I2C_MasterSend(I2C0, DAC_ADDRESS_1, y_axis, 2);
		}

	}
	return 1;
}
