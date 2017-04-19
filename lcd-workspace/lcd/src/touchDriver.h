#ifndef __TOUCHDRIVER_H_
#define __TOUCHDRIVER_H_

#include "chip.h"
#include "board.h"
#include "string.h"
#include <stdio.h>
#include "math.h"
#include "Adafruit_ILI9341.h"
#include "gpio_17xx_40xx.h"
#include "adc_17xx_40xx.h"

#ifdef CHIP_LPC175X_6X
/* GPIO pin for interrupt */

#define GPIO_INTERRUPT_PORT    GPIOINT_PORT0	/* GPIO port number mapped to interrupt */

/* On the LPC1769, the GPIO interrupts share the EINT3 vector. */
#define GPIO_IRQ_HANDLER  			EINT3_IRQHandler/* GPIO interrupt IRQ function name */
#define GPIO_INTERRUPT_NVIC_NAME    EINT3_IRQn	/* GPIO interrupt NVIC interrupt name */
#endif

// lcd touch screen
static ADC_CLOCK_SETUP_T ADCSetup;

class touchDriver {
public:
	touchDriver(int8_t _xm = 15,int8_t _ym = 16,int8_t _xp = 23,int8_t _yp = 24,int8_t _adcx = 25,int8_t _adcy = 26,int8_t _inter = 17){
		xm = _xm;
		ym = _ym;
		xp = _xp;
		yp = _yp;
		adcx = _adcx;
		adcy = _adcy;
		inter = _inter;

	}
	void 	touchDriverInit(void){
		/* Configure GPIO interrupt pin as input */
		Chip_GPIO_SetPinDIRInput(LPC_GPIO, GPIOINT_PORT0, inter);

		/* Configure the GPIO interrupt */
		Chip_GPIOINT_SetIntRising(LPC_GPIOINT, GPIOINT_PORT0, 1 << inter);

		/* Enable interrupt in the NVIC */
		NVIC_ClearPendingIRQ(GPIO_INTERRUPT_NVIC_NAME);
		NVIC_EnableIRQ(GPIO_INTERRUPT_NVIC_NAME);

		Chip_ADC_Init(LPC_ADC, &ADCSetup);


		//Chip_ADC_Int_SetGlobalCmd(LPC_ADC, DISABLE);


		Chip_GPIO_SetPinDIRInput(LPC_GPIO, 0, xp);
		Chip_GPIO_SetPinDIRInput(LPC_GPIO, 0, xm);
		Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, yp);
		Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, ym);
		Chip_GPIO_SetPinState(LPC_GPIO, 0, yp, 1);
		Chip_GPIO_SetPinState(LPC_GPIO, 0, ym, 0);
	}
	int16_t readDriverX(void){
		uint16_t data;
		Chip_ADC_EnableChannel(LPC_ADC, ADC_CH2, ENABLE);
		Chip_ADC_SetStartMode(LPC_ADC, ADC_START_NOW, ADC_TRIGGERMODE_RISING);
		/* Waiting for A/D conversion complete */
		while (Chip_ADC_ReadStatus(LPC_ADC, ADC_CH2, ADC_DR_DONE_STAT) != SET) {}
		/* Read ADC value */
		Chip_ADC_ReadValue(LPC_ADC, ADC_CH2, &data);
		Chip_ADC_EnableChannel(LPC_ADC, ADC_CH2, DISABLE);

		return data;
	}
	int16_t readDriverY(void){

		Chip_GPIO_SetPinDIRInput(LPC_GPIO, 0, yp);
		Chip_GPIO_SetPinDIRInput(LPC_GPIO, 0, ym);
		Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, xp);
		Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, xm);

		Chip_GPIO_SetPinState(LPC_GPIO, 0, xp, 1);
		Chip_GPIO_SetPinState(LPC_GPIO, 0, xm, 0);
		uint16_t data = 0;
		volatile uint32_t j = 100;
				while(j--){}
		Chip_ADC_EnableChannel(LPC_ADC, ADC_CH3, ENABLE);
		Chip_ADC_SetStartMode(LPC_ADC, ADC_START_NOW, ADC_TRIGGERMODE_RISING);
		// Waiting for A/D conversion complete
		while (Chip_ADC_ReadStatus(LPC_ADC, ADC_CH3, ADC_DR_DONE_STAT) != SET) {}
		// Read ADC value
		Chip_ADC_ReadValue(LPC_ADC, ADC_CH3, &data);
		Chip_ADC_EnableChannel(LPC_ADC, ADC_CH3, DISABLE);

		Chip_GPIO_SetPinDIRInput(LPC_GPIO, 0, xp);
		Chip_GPIO_SetPinDIRInput(LPC_GPIO, 0, xm);
		Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, yp);
		Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, ym);

		Chip_GPIO_SetPinState(LPC_GPIO, 0, yp, 1);
		Chip_GPIO_SetPinState(LPC_GPIO, 0, ym, 0);
		j = 100;
						while(j--){}
		return data;

	}
	int32_t getInternumber(void){return inter;}
private:
	int32_t xm, ym, xp, yp, adcx, adcy, inter;

};
#endif
