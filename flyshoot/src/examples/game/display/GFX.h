/*
 * GFX.h
 *
 *  Created on: Aug 3, 2018
 *      Author: aterrazas
 */


#ifndef GFX_H_
#define GFX_H_


#include <stdint.h>
#include <stdio.h>
#include "xil_types.h"

#define byte unsigned char

// Header Values
#define JUMPTABLE_BYTES 4

#define JUMPTABLE_LSB   1
#define JUMPTABLE_SIZE  2
#define JUMPTABLE_WIDTH 3
#define JUMPTABLE_START 4


#define WIDTH_POS 0
#define HEIGHT_POS 1
#define FIRST_CHAR_POS 2
#define CHAR_NUM_POS 3

#define MAXCOLS 106
#define MAXROWS 37
#define BUFFERSIZE (MAXCOLS * MAXROWS)

#define TOPLINE  DisplayHeight - 30

#define max(a,b) ((a)>(b)?(a):(b))

#ifndef _swap_int16_t
#define _swap_int16_t(a, b) { int16_t t = a; a = b; b = t; }
#endif

enum OLEDDISPLAY_COLOR {
  BLACK = 0,
  WHITE = 1,
  INVERSE = 2
};

typedef enum enum_alignment {
  TEXT_ALIGN_LEFT = 0,
  TEXT_ALIGN_RIGHT = 1,
  TEXT_ALIGN_CENTER = 2,
  TEXT_ALIGN_CENTER_BOTH = 3
} OLEDDISPLAY_TEXT_ALIGNMENT;


extern OLEDDISPLAY_TEXT_ALIGNMENT   textAlignment;

extern int GFX_init(UINTPTR membuf,uint16_t displayWidth, uint16_t displayHeight);
extern int GFX_drawProgressBar(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t progress);
extern int GFX_printLine(char* text,uint16_t row, uint16_t col);
extern int drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
extern int drawCircle(int16_t x0, int16_t y0, int16_t radius);
extern int GFX_fillCircle(int16_t x0, int16_t y0, int16_t radius);
extern int GFX_fillRect(int16_t xMove, int16_t yMove, int16_t width, int16_t height);
extern int GFX_changePenColor(uint8_t iliColor);
extern int GFX_drawString(int16_t xMove, int16_t yMove, char* text);
extern int GFX_pattern();
extern int drawStringBuffer(char * text, char * draw, int rowSize, int colSize);
extern void GFX_printText(char * text);
extern void GFX_printText_rc(char * text, int row, int col);
extern void GFX_update_textBuffer();
extern int setPixelColor(int16_t x, int16_t y,uint8_t color) ;
#endif /* GFX_H_ */


