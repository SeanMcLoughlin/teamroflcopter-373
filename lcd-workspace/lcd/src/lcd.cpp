/*
===============================================================================
 Name        : main.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
*/

#if defined (__USE_LPCOPEN)
#if defined(NO_BOARD_LIB)
#include "chip.h"
#else
#include "board.h"
#endif
#endif

//#include "lpc17xx.h"
#include <cr_section_macros.h>

// TODO: insert other include files here
#include "string.h"
#include <stdio.h>
#include "math.h"
#include <assert.h>
#include "gpio_17xx_40xx.h"
#include "Adafruit_ILI9341.h"
#include "touchDriver.h"

#ifndef max
    #define max(a,b) ((a) > (b) ? (a) : (b))
#endif

// Select UART Handlers
#define UART_SELECTION 		LPC_UART3
#define IRQ_SELECTION 		UART3_IRQn
#define HANDLER_NAME 		UART3_IRQHandler

#define UART_RRB_SIZE 	0x18 				// Depends on sampling rate. Don't want it to overflow
#define RIGHT_HAND_ADDR	0x415AEC5F			// The LOWER address of the right hand xbee (These DO NOT change with XBEE settings!)
#define LEFT_HAND_ADDR 	0x40AE5BE4			// The LOWER address of the left hand xbee
#define ADDR_START_IDX	4					// The index of the start of the address in the XBEE packet

static uint8_t x_axis[2], y_axis[2];
enum adcsample_t	{X_AXIS = 0, Y_AXIS = 1};


// TODO: insert other definitions and declarations here
#define TFT_CS   9
#define TFT_DC   8
#define TFT_MOSI 7
#define TFT_CLK  6
#define TFT_RST  2
#define TFT_MISO 3
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);
int core_w=30;
int core_h=50;
int fillet_radius=10;
int bar_width=25;
int bar_height=15;
uint16_t centerColor = ILI9341_WHITE;
uint16_t propellerColor	= ILI9341_WHITE;
#define backgroundColor 	ILI9341_BLACK
#define barColor 			ILI9341_DARKGREY
#define boundColor			ILI9341_WHITE
#define seperationLineColor	ILI9341_WHITE
#define buttonColor			ILI9341_BLUE
#define buttonwordColor		ILI9341_WHITE
#define buttonInnerColor	ILI9341_RED
uint16_t propeller_x=59;
uint16_t propeller_y=59;
uint16_t propeller_r=20;
uint16_t seperationLine_offset=117;
uint16_t button_height=300;
uint16_t button_r=18;
uint16_t button_r_small=13;
uint16_t num_buttons=3;
uint16_t directionCircle_r=110;
uint16_t directionCircle_t=3;
#define r_mask		0x0800
#define g_mask		0x0020
#define b_mask		0x0001
#define max_mask	0x1F
#define bound_size	128
int16_t bound_x[bound_size];
int16_t bound_y[bound_size];

#define center_radius_2 60
#define centerColor_2	ILI9341_DARKGREY
#define dirColor_2 		ILI9341_WHITE
#define updownColor_2	ILI9341_WHITE
#define rotColor_2		ILI9341_WHITE

int16_t updown_x1=100;
int16_t updown_x2=140;
int16_t updown_y1=140;
int16_t updown_y2=140;
int16_t updown_x3=120;

int16_t rot_x1=120;
int16_t rot_x2=120;
int16_t rot_y1=180;
int16_t rot_y2=220;
int16_t rot_y3=200;

int16_t prev_updownValue=0;
int16_t prev_rotValue=0;
int16_t previous_mode=2;
int16_t current_mode=2;


//**********************//merge
uint16_t fbNum = 0;
uint16_t lrNum = 0;
uint16_t udNum = 0;
uint16_t rotNum = 0;


uint16_t byteConcat(uint8_t byte1, uint8_t byte2) {
	return (byte1 << 8) | byte2;
}


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
		{
			// X is the first sample, 3 and 4 indices
			x_axis[0] = data[19];
			x_axis[1] = data[20];

			// Concatenate the two samples so that you can multiply by 4 (12 bit DAC vs 10 bit sample)

			// Concatenate the two bytes to one uint16
			uint16_t xtemp = byteConcat(x_axis[0], x_axis[1]);

			// Multiply by 4 for scaling for a 4 bit dac
			xtemp *= 4;

			// This is to prevent integer "underflow" so that the drone doesn't
			// stutter at low voltages
			if (xtemp < ceil(4095 * (0.4/3.3))) {
				xtemp = 0;
				x_axis[0] = (xtemp & 0xFF00) >> 8;
				x_axis[1] = (xtemp & 0x00FF);
				break;
			}


			// Scale the range that the accelerometer outputs with linear mapping
			xtemp = (xtemp - ceil(4095 * (0.4/3.3)))*ceil(3.0/2.4); // 0.4 is min, 2.4 is max - min

			// Detect integer overflow and don't let it loop around to 0
			if ((int)xtemp > 4095)
				xtemp = 4095;

			// Detect if it's within the trigger level (around 1.5V)
			if (2000 < xtemp && xtemp < 3000)
				xtemp = 2047;


			// Put the sample back into the indices
			x_axis[0] = (xtemp & 0xFF00) >> 8;
			x_axis[1] = (xtemp & 0x00FF);
		}

			break;
		case Y_AXIS:
		{
			// Y is the second sample, 5 and 6 indices
			y_axis[0] = data[21];
			y_axis[1] = data[22];

			uint16_t ytemp = byteConcat(y_axis[0], y_axis[1]);
			ytemp *= 4;

//			ytemp = max(ytemp, ceil(4095*(0.7/3.0)) + 100);
//			ytemp = (ytemp - ceil(4095 * (0.7/3.0)))*ceil(3.0/2.0);
//
//			// Detect integer overflow and don't let it loop around to 0
//			if ((int)ytemp > 4095)
//				ytemp = 4095;
//
//			// Trigger level (don't let the drone spurt)
//			ytemp -= 300;
//			if ((int)ytemp > 4095)
//				ytemp = 0;


			y_axis[0] = (ytemp & 0xFF00) >> 8;
			y_axis[1] = (ytemp & 0x00FF);
		}
			break;
		default:;
	}
}
uint8_t validAddress(uint8_t* data) {
	return xbeeAddress(data) == RIGHT_HAND_ADDR || xbeeAddress(data) == LEFT_HAND_ADDR;
}

uint8_t UART_INT_COUNT = 0;
uint8_t UART_BYTES;
uint8_t data[24];
#ifdef __cplusplus
extern "C" {
#endif
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
#ifdef __cplusplus
}
#endif


uint8_t latch[24];

void updateData(void){
	uint64_t address;
	if (data[0] == 0x7E && data[1] == 0x00 && data[2] == 0x14 && data[3] == 0x82) {

		int i;
		for (i = 0; i < 24; i++)
			latch[i] = data[i];

		address = xbeeAddress(latch);

		// Get the ADC samples
		getADCSample(latch, X_AXIS, address);
		getADCSample(latch, Y_AXIS, address);

	}
	if (address == RIGHT_HAND_ADDR) {
		lrNum = byteConcat(x_axis[0],x_axis[1]);
		fbNum = byteConcat(y_axis[0],y_axis[1]);
	}
	else if(address == LEFT_HAND_ADDR){
		//error
	}

	Chip_ADC_EnableChannel(LPC_ADC, ADC_CH4, ENABLE);
	Chip_ADC_SetStartMode(LPC_ADC, ADC_START_NOW, ADC_TRIGGERMODE_RISING);
	/* Waiting for A/D conversion complete */
	while (Chip_ADC_ReadStatus(LPC_ADC, ADC_CH4, ADC_DR_DONE_STAT) != SET) {}
	/* Read ADC value */
	Chip_ADC_ReadValue(LPC_ADC, ADC_CH4, &udNum);
	Chip_ADC_EnableChannel(LPC_ADC, ADC_CH4, DISABLE);


	Chip_ADC_EnableChannel(LPC_ADC, ADC_CH5, ENABLE);
	Chip_ADC_SetStartMode(LPC_ADC, ADC_START_NOW, ADC_TRIGGERMODE_RISING);
	/* Waiting for A/D conversion complete */
	while (Chip_ADC_ReadStatus(LPC_ADC, ADC_CH5, ADC_DR_DONE_STAT) != SET) {}
	/* Read ADC value */
	Chip_ADC_ReadValue(LPC_ADC, ADC_CH5, &rotNum);
	Chip_ADC_EnableChannel(LPC_ADC, ADC_CH5, DISABLE);
}



void init_bound() {
	for (int16_t i=0; i<32; i++) {
		bound_x[i]=tft.width()/2+i*3;
		bound_y[i]=tft.height()/2+96;
	}

	for (int16_t i=32;i<96;i++) {
		bound_x[i]=tft.width()/2+96;
		bound_y[i]=tft.height()/2+192-i*3;
	}
	for (int16_t i=96; i<128; i++) {
		bound_x[i]=tft.width()/2+(128-i)*3;
		bound_y[i]=tft.height()/2-96;
	}
}

void fill_bound(int16_t i, int16_t color) {
	tft.startWrite();
	for (int16_t x_off=-1; x_off<2; x_off++) {
		for (int16_t y_off=-1; y_off<2; y_off++) {
	        tft.writePixel(bound_x[i]+x_off, bound_y[i]+y_off, color);
	        tft.writePixel(tft.width()-bound_x[i]-x_off, tft.height()-bound_y[i]-y_off, color);
		}
	}
	tft.endWrite();
}

int16_t color_mapping (int16_t n) {//from 0~127 mapping to red--> yellow --> green --> cyan --> blue
	if (n<32) { //red to yellow
		return max_mask*b_mask+n*g_mask;
	}
	else if (n<64) {//green to cyan
		return max_mask*g_mask+(63-n)*b_mask;
	}
	else if (n<96) {//yellow to green
		return max_mask*g_mask+(n-64)*r_mask;
	}
	else if(n<128) {//red to yellow
		return max_mask*r_mask+(127-n)*g_mask;
	}
	else if (n<160) {// purple to red
		return max_mask*r_mask+(n-128)*b_mask;
	}
	else if(n<192) {//blue to purple
		return max_mask*b_mask+(191-n)*r_mask;
	}
	else {
		return 0;
	}
}

void print(char s[], int16_t start_x, int16_t start_y, int16_t size, uint16_t color) {
	  tft.setCursor(start_x, start_y);
	  tft.setTextColor(color);
	  tft.setTextSize(size);
	  tft.startWrite();
	  for (uint16_t i=0; i<strlen(s);i++) {
		  tft.write(s[i]);
	  }
	  tft.endWrite();
}

void drawSeperationLine() {
	tft.drawFastHLine(0,tft.height()/2-seperationLine_offset,tft.width(),seperationLineColor);
	tft.drawFastHLine(0,tft.height()/2+seperationLine_offset,tft.width(),seperationLineColor);
}

void drawButtons() {
	for (uint16_t i=0; i<num_buttons; i++) {
		uint16_t button_x=tft.width()*(2*i+1)/(2*num_buttons);
		tft.fillCircle(button_x,button_height,button_r,buttonColor);
		tft.fillCircle(button_x,button_height,button_r_small,buttonInnerColor);
	}
	char s1[]="I";
	char s2[]="C";
	char s3[]="P";
	int16_t offs_x=3;
	int16_t offs_y=7;
	print(s1, tft.width()*1/6-offs_x, button_height-offs_y, 2, buttonwordColor);
	print(s2, tft.width()*3/6-offs_x, button_height-offs_y, 2, buttonwordColor);
	print(s3, tft.width()*5/6-offs_x, button_height-offs_y, 2, buttonwordColor);
}

void drawBar(int16_t x0, int16_t y0, int16_t bar_w, int16_t bar_h, int16_t w, uint16_t color) {
	for (int i=0; i<w; i++) {
		tft.drawLine(x0+i,y0,x0+bar_w+i,y0+bar_h,color);
		tft.drawLine(x0-i,y0,x0+bar_w-i,y0+bar_h,color);
	}
}

void drawPropeller (int16_t num, int16_t color) {
	//0 for upper left, 1 for upper right, 2 for lower left, 3 for lower right
	int16_t center_x, center_y;
	switch (num) {
	case 0:
		center_x=tft.width()/2-propeller_x;
		center_y=tft.height()/2-propeller_y;
		break;
	case 1:
		center_x=tft.width()/2+propeller_x;
		center_y=tft.height()/2-propeller_y;
		break;
	case 2:
		center_x=tft.width()/2-propeller_x;
		center_y=tft.height()/2+propeller_y;
		break;
	case 3:
		center_x=tft.width()/2+propeller_x;
		center_y=tft.height()/2+propeller_y;
		break;
	default:
		break;
	}
    tft.fillCircle(center_x,center_y,propeller_r,color);
}

void drawBound() {
	/*for (int16_t r=directionCircle_r; r<directionCircle_r+directionCircle_t; r++) {
		tft.drawCircle(tft.width()/2, tft.height()/2, r, directionCircleColor);
	}*/
	for (int16_t i=0; i<bound_size; i++) {
		fill_bound(i,boundColor);
	}
}

void drawcenter() {
	tft.fillRoundRect(tft.width()/2-core_w/2,tft.height()/2-core_h/2,core_w,core_h,fillet_radius,centerColor);
}

void drawDroneFrame() {
	//drawcenter();
	drawBar(tft.width()/2+core_w/2,tft.height()/2+core_h/2,bar_width,bar_height,5,barColor);
	drawBar(tft.width()/2+core_w/2,tft.height()/2-core_h/2,bar_width,-bar_height,5,barColor);
	drawBar(tft.width()/2-core_w/2,tft.height()/2+core_h/2,-bar_width,bar_height,5,barColor);
	drawBar(tft.width()/2-core_w/2,tft.height()/2-core_h/2,-bar_width,-bar_height,5,barColor);
	/*for (int i=0; i<4; i++) {
		drawPropeller (i,propellerColor);
	}*/
	//drawBound();
}

void drawWords() {
	  char s[]="Go RofCopter!";
	  print(s, 1, 1, 3, ILI9341_MAGENTA);
}

void screen_init_mode1() {
	tft.fillScreen(ILI9341_BLACK);
	drawDroneFrame();
	drawSeperationLine();
	drawButtons();
	drawWords();
}

void roll(int16_t step) {
	static int16_t current_i=0;
	static int16_t color_index=0;
	int16_t end_i=current_i+step;

		for (int16_t i=current_i; i<end_i; i++) {
			fill_bound(i%bound_size,color_mapping(color_index));
			color_index=(color_index+1)%192;
		}


	if (end_i>=bound_size) {
		current_i=end_i-bound_size;
	}
	else {
		current_i=end_i;
	}
}

void screen_init_mode2() {
	tft.fillScreen(ILI9341_BLACK);
	drawButtons();
	drawWords();
	tft.fillCircle(tft.width()/2,tft.height()/2,center_radius_2,centerColor_2);
}

void drawdir_triangle(int16_t fbValue, int16_t lrValue, uint16_t color) {
	int16_t offset=10;
	int16_t dist=3;
	double len=double(sqrt(fbValue*fbValue+lrValue*lrValue));
	double norm_y=double(fbValue)/len;
	double norm_x=double(lrValue)/len;
	double x1=tft.width()/2+norm_x*(len+dist+center_radius_2);
	double y1=tft.height()/2+norm_y*(len+dist+center_radius_2);
	double x2=tft.width()/2+norm_x*(center_radius_2+dist)-offset*norm_y;
	double y2=tft.height()/2+norm_y*(center_radius_2+dist)+offset*norm_x;
	double x3=tft.width()/2+norm_x*(center_radius_2+dist)+offset*norm_y;
	double y3=tft.height()/2+norm_y*(center_radius_2+dist)-offset*norm_x;
	tft.fillTriangle(round(x1),round(y1),round(x2),round(y2),round(x3),round(y3),color);
}

void draw_dir(int16_t fbValue, int16_t lrValue) {
	static int16_t prev_fbValue=0;
	static int16_t prev_lrValue=0;
	drawdir_triangle(prev_fbValue, prev_lrValue, ILI9341_BLACK);
	drawdir_triangle(fbValue, lrValue, dirColor_2);
	prev_fbValue=fbValue;
	prev_lrValue=lrValue;
}

void draw_updown_tri (int16_t updownValue, uint16_t color) {
	int16_t updown_y3=updown_y1+updownValue;
	tft.fillTriangle(updown_x1,updown_y1,updown_x2,updown_y2,updown_x3,updown_y3,color);
}

void modify_updown_tri (int16_t updownValue1, int16_t updownValue2, uint16_t color) {
	int16_t updown_y3=updown_y1+updownValue1;
	int16_t updown_y4=updown_y1+updownValue2;
	tft.fillTriangle(updown_x1,updown_y1,updown_x3,updown_y3,updown_x3,updown_y4,color);
	tft.fillTriangle(updown_x2,updown_y2,updown_x3,updown_y3,updown_x3,updown_y4,color);
}

void draw_updown (int16_t updownValue) {

	if ((prev_updownValue>0) && (updownValue>0)) {
		if (updownValue>prev_updownValue) {
			modify_updown_tri(updownValue,prev_updownValue,updownColor_2);
		}
		else {
			modify_updown_tri(updownValue,prev_updownValue,centerColor_2);
		}
	}
	else if ((prev_updownValue<0) && (updownValue<0)) {
		if (updownValue>prev_updownValue) {
			modify_updown_tri(updownValue,prev_updownValue,centerColor_2);
		}
		else {
			modify_updown_tri(updownValue,prev_updownValue,updownColor_2);
		}
	}
	else {
		draw_updown_tri(prev_updownValue,centerColor_2);
		draw_updown_tri(updownValue,updownColor_2);
	}
	prev_updownValue=updownValue;
}

void draw_rot_tri (int16_t rotValue, uint16_t color) {
	int16_t rot_x3=rot_x1+rotValue;
	tft.fillTriangle(rot_x1,rot_y1,rot_x2,rot_y2,rot_x3,rot_y3,color);
}

void modify_rot_tri (int16_t rotValue1, int16_t rotValue2, uint16_t color) {
	int16_t rot_x3=rot_x1+rotValue1;
	int16_t rot_x4=rot_x1+rotValue2;
	tft.fillTriangle(rot_x1,rot_y1,rot_x3,rot_y3,rot_x4,rot_y3,color);
	tft.fillTriangle(rot_x2,rot_y2,rot_x3,rot_y3,rot_x4,rot_y3,color);
}

void draw_rot (int16_t rotValue) {

	if ((prev_rotValue>0) && (rotValue>0)) {
		if (rotValue>prev_rotValue) {
			modify_rot_tri(rotValue,prev_rotValue,rotColor_2);
		}
		else {
			modify_rot_tri(rotValue,prev_rotValue,centerColor_2);
		}
	}
	else if ((prev_rotValue<0) && (rotValue<0)) {
		if (rotValue>prev_rotValue) {
			modify_rot_tri(rotValue,prev_rotValue,centerColor_2);
		}
		else {
			modify_rot_tri(rotValue,prev_rotValue,rotColor_2);
		}
	}
	else {
		draw_rot_tri(prev_rotValue,centerColor_2);
		draw_rot_tri(rotValue,rotColor_2);
	}
	prev_rotValue=rotValue;
}

void reinit (int16_t thismode) {
	if (previous_mode!=0) {
		tft.fillRect(0,tft.height()/2-120,tft.width(),240,backgroundColor);
	}
	if (thismode==1) { //cool mode
		drawDroneFrame();
		drawSeperationLine();
	}
	else if (thismode==2) {// practical mode
		tft.fillCircle(tft.width()/2,tft.height()/2,center_radius_2,centerColor_2);
		prev_updownValue=0;
		prev_rotValue=0;
	}
	else {//idle mode
		//do nothing
	}
}


// touch screen gpio interrupt.
touchDriver touch = touchDriver();
#ifdef __cplusplus
extern "C" {
#endif
void GPIO_IRQ_HANDLER(void)
{
	//Chip_GPIOINT_ClearIntStatus(LPC_GPIOINT, GPIOINT_PORT0, 1 << 17);

	uint16_t dataX  = 0;
	uint16_t dataY  = 0;
	dataX = touch.readDriverX();
	dataY = touch.readDriverY();
	Board_LED_Toggle(0);

	if (dataX>3000) {
		if (dataY>2400) {
			current_mode=2;
		}
		else if (dataY>1200) {
			current_mode=1;
		}
		else if (dataY>200) {
			current_mode=0;
		}
	}
	Chip_GPIOINT_ClearIntStatus(LPC_GPIOINT, GPIOINT_PORT0, 1 << 17);
}
#ifdef __cplusplus
}
#endif




int main(void) {

#if defined (__USE_LPCOPEN)
    // Read clock settings and update SystemCoreClock variable
    SystemCoreClockUpdate();
#if !defined(NO_BOARD_LIB)
    // Set up and initialize all required blocks and
    // functions related to the board hardware
    Board_Init();
    Board_SystemInit();
    // Set the LED to the state of "On"
    Board_LED_Set(0, true);
#endif
#endif
    //Chip_GPIO_Init(LPC_GPIO);
    // TODO: insert code here
    SysTick_Config(SystemCoreClock / 1000);
    tft.begin();
    tft.setRotation(2);
    touch.touchDriverInit();


    Board_UART_Init(UART_SELECTION);
	Chip_UART_Init(UART_SELECTION);
	Chip_UART_SetBaud(UART_SELECTION, 9600);
	Chip_UART_ConfigData(UART_SELECTION, (UART_LCR_WLEN8 | UART_LCR_SBS_1BIT));
	Chip_UART_SetupFIFOS(UART_SELECTION, (UART_FCR_FIFO_EN | UART_FCR_TRG_LEV2));
	/* Enable UART 3 interrupt */
	Chip_UART_IntEnable(UART_SELECTION, (UART_IER_RBRINT | UART_IER_RLSINT));

	/* preemption = 1, sub-priority = 1 */
	NVIC_SetPriority(IRQ_SELECTION, 1);
	NVIC_EnableIRQ(IRQ_SELECTION);

	init_bound();



	  tft.setTextColor(0);
	  tft.setTextSize(1);
	/*
	    if (current_mode==1) {
	    	screen_init_mode1();
	    }
	    else {
	    	screen_init_mode2();
	    }

	    bool selection1=false; //for round robin
	    int selection2=0;		//for round robin
	    int selection3=0;		//for round robin
		int16_t updownValue=20;
		int16_t rotValue=20;
		int16_t fbValue=20;
		int16_t lrValue=20;

	    while (1) {
	    	//reinit the screen and paramters
	    	if (current_mode!=previous_mode) {
	    		reinit(current_mode);
	    		previous_mode=current_mode;
	    		selection2=0;
	    		selection3=0;
	    	}
	    	updateData();

	    	if (current_mode==2) { //practical mode
	    		switch (selection3) {
	    		case 0:
	        		//read forward/backward value
	    			fbValue=fbNum;
	        		//read left/right value
	    			lrValue=lrNum;
	    			draw_dir(fbValue,lrValue);
	    			break;
	    		case 1:
	        		//read rot value
	    			rotValue=rotNum;
	    			draw_rot(rotValue);
	    			break;
	    		case 2:
	    	 		//read up/down value
	    			updownValue=udNum;
	    			draw_updown(updownValue);
	    			break;
	    		default:
	    			break;
	    		}
	    		selection3=(selection3+1)%3;
	    	}

	    	else if (current_mode==1) { //cool mode
				if (selection1==false) {	//update up/down and propeller thrust
					selection1=true;
					switch (selection2 % 5) {
					case 4: //update up/down
						//read up/down value
		    			updownValue=udNum;
						centerColor=color_mapping(updownValue);
						drawcenter();
						break;
					case 0: //update P0 upper left
						//read fb, lr
						fbValue=fbNum;
						lrValue=lrNum;
						//lrValue+=1;
						drawPropeller(0, color_mapping(64+(fbValue+lrValue)/2));
						break;
					case 1: //update P1 upper right
						drawPropeller(1, color_mapping(64+(fbValue-lrValue)/2));
						break;
					case 2: //update P2 lower left
						drawPropeller(2, color_mapping(64+(-fbValue+lrValue)/2));
						break;
					case 3: //update P3 lower right
						drawPropeller(3, color_mapping(64+(-fbValue-lrValue)/2));
						break;
					}
					selection2=(selection2+1)%5;
				}
				else {
					//update rotation direction
					selection1=false;
					//read rotvalue
					rotValue=rotNum;
					roll(rotValue);
				}
	    	}
	    	else {//idle mode
	    		__WFI();
	    	}
	    }
*/

    return 0 ;
}
