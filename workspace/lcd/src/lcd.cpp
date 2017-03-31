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
#include "Adafruit_ILI9341.h"

// TODO: insert other definitions and declarations here
#define TFT_CS   9
#define TFT_DC   8
#define TFT_MOSI 7
#define TFT_CLK  6
#define TFT_RST  0
#define TFT_MISO 1
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);


unsigned long testLines(uint16_t color) {
  unsigned long start, t;
  int           x1, y1, x2, y2,
                w = tft.width(),
                h = tft.height();

  tft.fillScreen(ILI9341_BLACK);


  x1 = y1 = 0;
  y2    = h - 1;
  for(x2=0; x2<w; x2+=6) tft.drawLine(x1, y1, x2, y2, color);
  x2    = w - 1;
  for(y2=0; y2<h; y2+=6) tft.drawLine(x1, y1, x2, y2, color);


  tft.fillScreen(ILI9341_BLACK);


  x1    = w - 1;
  y1    = 0;
  y2    = h - 1;
  for(x2=0; x2<w; x2+=6) tft.drawLine(x1, y1, x2, y2, color);
  x2    = 0;
  for(y2=0; y2<h; y2+=6) tft.drawLine(x1, y1, x2, y2, color);


  tft.fillScreen(ILI9341_BLACK);


  x1    = 0;
  y1    = h - 1;
  y2    = 0;
  for(x2=0; x2<w; x2+=6) tft.drawLine(x1, y1, x2, y2, color);
  x2    = w - 1;
  for(y2=0; y2<h; y2+=6) tft.drawLine(x1, y1, x2, y2, color);


  tft.fillScreen(ILI9341_BLACK);


  x1    = w - 1;
  y1    = h - 1;
  y2    = 0;
  for(x2=0; x2<w; x2+=6) tft.drawLine(x1, y1, x2, y2, color);
  x2    = 0;
  for(y2=0; y2<h; y2+=6) tft.drawLine(x1, y1, x2, y2, color);

  return 1;
}



int main(void) {

#if defined (__USE_LPCOPEN)
    // Read clock settings and update SystemCoreClock variable
    SystemCoreClockUpdate();
#if !defined(NO_BOARD_LIB)
    // Set up and initialize all required blocks and
    // functions related to the board hardware
    Board_Init();
    // Set the LED to the state of "On"
    Board_LED_Set(0, true);
#endif
#endif
    //Chip_GPIO_Init(LPC_GPIO);
    // TODO: insert code here
    tft.begin();
    tft.fillScreen(ILI9341_BLUE);

    // Force the counter to be placed into memory
    volatile static int i = 0 ;
    // Enter an infinite loop, just incrementing a counter
    while(1) {
    	for (i=8750; i >0;i--){};
    	testLines(ILI9341_CYAN);
    }
    return 0 ;
}
