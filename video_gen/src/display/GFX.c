/*
 * GFX.c
 *
 *  Created on: Aug 3, 2018
 *      Author: aterrazas
 */

#include "GFX.h"

#include "xil_io.h"
#include <stdlib.h>
#include "OLEDDisplayFonts.h"

//#include "global.h"

uint16_t DisplayWidth;
uint16_t DisplayHeight;

uint8_t fontColor = 0xff;// ILI9341_WHITE;
uint16_t fontBackground = 0x0000;//ILI9341_BLACK;
static UINTPTR buffer; // this is the buffer used for map the display data
static char textBuffer[BUFFERSIZE]; // this is the buffer to track the text
static char maskBuffer[BUFFERSIZE]; // Keeps tracks of only the chars that changes
//static uint16_t DisplayWidth;
//static uint16_t DisplayHeight;

const char *fontData = DejaVu_LGC_Sans_Mono_10;
OLEDDISPLAY_TEXT_ALIGNMENT   textAlignment = TEXT_ALIGN_LEFT;

/* Changes the font color for next GFX action */
int GFX_changePenColor(uint8_t iliColor)
{
	fontColor = iliColor;
	return 0;
}

/* Initializes the GFX library */
int GFX_init(UINTPTR  membuf,uint16_t displayWidth, uint16_t displayHeight)
{
	buffer = membuf;
	DisplayWidth = displayWidth;
	DisplayHeight = displayHeight;
	return 0;
}




int setPixel(int16_t x, int16_t y) {
	if (x >= 0 && x <   DisplayWidth && y >= 0 && y <  DisplayHeight) {
//		buffer[ (x + y *  DisplayWidth)] |=  fontColor;

		Xil_Out8(buffer + (x*3 + y *  DisplayWidth * 3 + 0) ,fontColor);
		Xil_Out8(buffer + (x*3 + y *  DisplayWidth * 3 + 1) ,fontColor);
		Xil_Out8(buffer + (x*3 + y *  DisplayWidth * 3 + 2) ,fontColor);

	}
	return 0;
}

int setPixelColor(int16_t x, int16_t y,uint8_t color) {
	if (x >= 0 && x <   DisplayWidth && y >= 0 && y <  DisplayHeight) {
//		buffer[ (x + y *  DisplayWidth)] |=  fontColor;

//		Xil_Out8(buffer + (x*3 + y *  DisplayWidth * 3 + 0) ,color);
//		Xil_Out8(buffer + (x*3 + y *  DisplayWidth * 3 + 1) ,color);
//		Xil_Out8(buffer + (x*3 + y *  DisplayWidth * 3 + 2) ,color);

		u32 tmpColor = color==0?0:0xffffffff;

		Xil_Out32(buffer + (x*3 + y *  DisplayWidth * 3 + 0) ,tmpColor);

	}
	return 0;
}

int GFX_pattern(){
	int x, y = 0;

	for (x = 0; x < DisplayWidth; x += 6){

		drawLine(x, 0, x, 480);

		for (y = 0; y < DisplayWidth; y += 13){
//			setPixel(x,y);
			drawLine(0, y, 640, y);
		}

	}

	return 0;

}

int drawCircleQuads(int16_t x0, int16_t y0, int16_t radius, uint8_t quads) {
	int16_t x = 0, y = radius;
	int16_t dp = 1 - radius;
	while (x < y) {
		if (dp < 0)
			dp = dp + 2 * (++x) + 3;
		else
			dp = dp + 2 * (++x) - 2 * (--y) + 5;
		if (quads & 0x1) {
			setPixel(x0 + x, y0 - y);
			setPixel(x0 + y, y0 - x);
		}
		if (quads & 0x2) {
			setPixel(x0 - y, y0 - x);
			setPixel(x0 - x, y0 - y);
		}
		if (quads & 0x4) {
			setPixel(x0 - y, y0 + x);
			setPixel(x0 - x, y0 + y);
		}
		if (quads & 0x8) {
			setPixel(x0 + x, y0 + y);
			setPixel(x0 + y, y0 + x);
		}
	}
	if ((quads & 0x1) && (quads & 0x8)) {
		setPixel(x0 + radius, y0);
	}
	if ((quads & 0x4) && (quads & 0x8)) {
		setPixel(x0, y0 + radius);
	}
	if ((quads & 0x2) && (quads & 0x4)) {
		setPixel(x0 - radius, y0);
	}
	if ((quads & 0x1) && (quads & 0x2)) {
		setPixel(x0, y0 - radius);
	}

	return 0;
}

int drawHorizontalLine(int16_t x, int16_t y, int16_t length) {
	if (y < 0 || y >= DisplayHeight) { return 0; }

//	int16_t x,y;
//
//	x=xMove;
//
//	if (yMove < 0)
//		y =0;
//
//	if (yMove >= DisplayHeight)
//		y = DisplayHeight - 1;



	if (x < 0) {
		length += x;
		x = 0;
	}

	if ( (x + length) > DisplayWidth) {
		length = (DisplayWidth - x);
	}

	if (length <= 0) { return 0; }

	UINTPTR bufferPtr = buffer;
	bufferPtr += y * DisplayWidth * 3;
	bufferPtr += x * 3;

	while (length--) {
//		*bufferPtr++ = fontColor;
//		Xil_Out8(bufferPtr++ , fontColor);
//		Xil_Out8(bufferPtr++ , fontColor);
//		Xil_Out8(bufferPtr++ , fontColor);

		setPixel(x + length , y);
	};

	return 0;
}

int drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1) {
	int16_t steep = abs(y1 - y0) > abs(x1 - x0);
	if (steep) {
		_swap_int16_t(x0, y0);
		_swap_int16_t(x1, y1);
	}

	if (x0 > x1) {
		_swap_int16_t(x0, x1);
		_swap_int16_t(y0, y1);
	}

	int16_t dx, dy;
	dx = x1 - x0;
	dy = abs(y1 - y0);

	int16_t err = dx / 2;
	int16_t ystep;

	if (y0 < y1) {
		ystep = 1;
	}
	else {
		ystep = -1;
	}


	for (; x0<=x1; x0++) {
		if (steep) {
			setPixel(y0, x0);
		}
		else {
			setPixel(x0, y0);
		}
		err -= dy;
		if (err < 0) {
			y0 += ystep;
			err += dx;
		}
	}

	return 0;
}

int drawCircle(int16_t x0, int16_t y0, int16_t radius) {
	int16_t x = 0, y = radius;
	int16_t dp = 1 - radius;
	do {
		if (dp < 0)
			dp = dp + 2 * (++x) + 3;
		else
			dp = dp + 2 * (++x) - 2 * (--y) + 5;

		setPixel(x0 + x, y0 + y);     //For the 8 octants
		setPixel(x0 - x, y0 + y);
		setPixel(x0 + x, y0 - y);
		setPixel(x0 - x, y0 - y);
		setPixel(x0 + y, y0 + x);
		setPixel(x0 - y, y0 + x);
		setPixel(x0 + y, y0 - x);
		setPixel(x0 - y, y0 - x);

	} while (x < y);

	setPixel(x0 + radius, y0);
	setPixel(x0, y0 + radius);
	setPixel(x0 - radius, y0);
	setPixel(x0, y0 - radius);

	return 0;
}

int drawVerticalLine(int16_t x, int16_t y, int16_t length) {
	if (x < 0 || x >= DisplayWidth) return 0;

	if (y < 0) {
		length += y;
		y = 0;
	}

	if ( (y + length) > DisplayHeight) {
		length = (DisplayHeight - y);
	}

	if (length <= 0) return 0;

	UINTPTR bufferPtr = buffer;

	bufferPtr += y * DisplayWidth * 3 ;
	bufferPtr += x * 3;
	do {
//		*bufferPtr = fontColor;
		Xil_Out8(bufferPtr++ , 0xff);
		Xil_Out8(bufferPtr++ , 0xff);
		Xil_Out8(bufferPtr++ , 0xff);

		bufferPtr += DisplayWidth * 3;
		length -= 1;
	} while (length > 0);

	if (length > 0) {
//		*bufferPtr =fontColor;
		Xil_Out8(bufferPtr , 0xff);
		Xil_Out8(bufferPtr , 0xff);
		Xil_Out8(bufferPtr , 0xff);
	}

	return 0;
}

int GFX_drawRect(int16_t x, int16_t y, int16_t width, int16_t height) {
	drawHorizontalLine(x, y, width);
	drawVerticalLine(x, y, height);
	drawVerticalLine(x + width - 1, y, height);
	drawHorizontalLine(x, y + height - 1, width);
	return 0;
}

int GFX_fillRoundRect(int16_t xMove, int16_t yMove, int16_t width, int16_t height) {
	int16_t x;
	for (x = xMove; x < xMove + width; x++) {
		drawVerticalLine(x, yMove, height);
	}
	return 0;
}

int GFX_fillRectVertical(int16_t xMove, int16_t yMove, int16_t width, int16_t height) {
	int16_t x;
	for (x = xMove; x < xMove + width; x++) {
		drawVerticalLine(x, yMove, height);
	}
	return 0;
}

/* Draws a Filled rectangle with the last font pen color */
int GFX_fillRect(int16_t xMove, int16_t yMove, int16_t width, int16_t height) {
	int16_t y;
	for (y = yMove; y < yMove + height; y++) {
		drawHorizontalLine(xMove, y, width);
	}
	return 0;
}

/* Draws a Filled circle with the last font pen color*/
int GFX_fillCircle(int16_t x0, int16_t y0, int16_t radius) {
	int16_t x = 0, y = radius;
	int16_t dp = 1 - radius;
	do {
		if (dp < 0)
			dp = dp + 2 * (++x) + 3;
		else
			dp = dp + 2 * (++x) - 2 * (--y) + 5;

		drawHorizontalLine(x0 - x, y0 - y, 2*x);
		drawHorizontalLine(x0 - x, y0 + y, 2*x);
		drawHorizontalLine(x0 - y, y0 - x, 2*y);
		drawHorizontalLine(x0 - y, y0 + x, 2*y);
	} while (x < y);

	drawHorizontalLine(x0 - radius, y0, 2 * radius);
	return 0;
}

int GFX_drawProgressBar(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t progress) {
	uint16_t radius = height / 2;
	uint16_t xRadius = x + radius;
	uint16_t yRadius = y + radius;
	uint16_t doubleRadius = 2 * radius;
	uint16_t innerRadius = radius - 2;

	drawCircleQuads(xRadius, yRadius, radius, 0b00000110);
	drawHorizontalLine(xRadius, y, width - doubleRadius + 1);
	drawHorizontalLine(xRadius, y + height, width - doubleRadius + 1);
	drawCircleQuads(x + width - radius, yRadius, radius, 0b00001001);

	uint16_t maxProgressWidth = (width - doubleRadius - 1) * progress / 100;

	GFX_fillCircle(xRadius, yRadius, innerRadius);
	GFX_fillRect(xRadius + 1, y + 2, maxProgressWidth, height - 3);
	GFX_fillCircle(xRadius + maxProgressWidth, yRadius, innerRadius);
	return 0;
}

uint16_t getStringWidth(const char* text, uint16_t length) {
	uint16_t firstChar        = *(fontData + FIRST_CHAR_POS);
	// printf("\r\n firstChar = 0x%02x\r\n", firstChar);

	uint16_t stringWidth = 0;
	uint16_t maxWidth = 0;

	while (length--) {
		stringWidth += *(fontData + JUMPTABLE_START + (text[length] - firstChar) * JUMPTABLE_BYTES + JUMPTABLE_WIDTH);
		//    printf("stringwidth = 0x%02x, data = 0x%02x\r\n", stringWidth,*(fontData + JUMPTABLE_START + (text[length] - firstChar) * JUMPTABLE_BYTES + JUMPTABLE_WIDTH) );
		if (text[length] == 10) {
			maxWidth = max(maxWidth, stringWidth);
			stringWidth = 0;
		}
	}

	//printf("maxWidth = 0x%02x stringWidth = 0x%02x\r\n", maxWidth,stringWidth);
	return max(maxWidth, stringWidth);
}




int drawInternal(int16_t xMove, int16_t yMove, int16_t width, int16_t height, const char *data, uint16_t offset, uint16_t bytesInData, int8_t color) {
	if (width < 0 || height < 0) return 0;
	if (yMove + height < 0 || yMove > DisplayHeight)  return -1;
	if (xMove + width  < 0 || xMove > DisplayWidth)   return -1;

	uint32_t  rasterHeight = 1 + ((height - 1)>>3); // fast ceil(height / 8.0)
	int   yOffset;

	bytesInData = bytesInData == 0 ? width * rasterHeight : bytesInData;

	uint32_t k,z;
	int i;
	k=0;
	for (i = 0 ; i < width ; i++) {
		z= rasterHeight;
		for(yOffset = rasterHeight -1 ; yOffset >=0 ;yOffset--){
			if(bytesInData == k)
				break;
			k++;

			byte currentByte = *(data + offset + (yOffset) +  (i*rasterHeight)  );

			int32_t xPos = xMove + i;
			int32_t yPos = (yMove * (DisplayWidth) )+ ((z--) * (DisplayWidth) * 8);
			int32_t dataPos    = xPos  + yPos - (8 * DisplayWidth);
			dataPos = dataPos * 3;
			int bitIterator;

			if(xPos > DisplayWidth-1)
				return -1;

			for(bitIterator = 7; bitIterator >= 0; bitIterator --)
				if((0x01 << bitIterator) & (currentByte)){
					if((dataPos + (bitIterator * DisplayWidth * 3)) < ( DisplayWidth * DisplayHeight * 3)){
						Xil_Out8(buffer + dataPos + (bitIterator * DisplayWidth * 3) + 0,color);
						Xil_Out8(buffer + dataPos + (bitIterator * DisplayWidth * 3) + 1 ,color);
						Xil_Out8(buffer + dataPos + (bitIterator * DisplayWidth * 3) + 2,color);
					}
				}

		}
	}
	return 0;
}

int drawStringBuffer(char * text, char * mask, int rowSize, int colSize){
	uint8_t textHeight       = *(fontData + HEIGHT_POS);
	uint8_t firstChar        = *(fontData + FIRST_CHAR_POS);
	uint16_t sizeOfJumpTable = *(fontData + CHAR_NUM_POS)  * JUMPTABLE_BYTES;
	int error;

	uint16_t j,i;
	for (i = 0; i < rowSize; i++){


		for (j = 0; j < colSize; j++) {
			int16_t xPos;
			int16_t yPos;
			byte code;

			if(mask[j + i * colSize] == 1){

				// removes background
				byte charByteSize     = 12;  // Size
				byte currentCharWidth = 6; // Width
				xPos = j%colSize * currentCharWidth;
				yPos = i%rowSize * textHeight;
				const char space_char[15] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};	// 32
				error = drawInternal(xPos, yPos, currentCharWidth, textHeight, space_char, 0, charByteSize,0x00);


				code = text[j + i * colSize];
				if (code >= firstChar) {
					byte charCode = code - firstChar;

					// 4 Bytes per char code
					byte msbJumpToChar    = *( fontData + JUMPTABLE_START + charCode * JUMPTABLE_BYTES );                  // MSB  \ JumpAddress
					byte lsbJumpToChar    = *( fontData + JUMPTABLE_START + charCode * JUMPTABLE_BYTES + JUMPTABLE_LSB);   // LSB /
					byte charByteSize     = *( fontData + JUMPTABLE_START + charCode * JUMPTABLE_BYTES + JUMPTABLE_SIZE);  // Size
					byte currentCharWidth = *( fontData + JUMPTABLE_START + charCode * JUMPTABLE_BYTES + JUMPTABLE_WIDTH); // Width

					xPos = j%colSize * currentCharWidth;
					yPos = i%rowSize * textHeight;


					// Test if the char is drawable
					if (!(msbJumpToChar == 255 && lsbJumpToChar == 255)) {
						// Get the position of the char data
						uint16_t charDataPosition = JUMPTABLE_START + sizeOfJumpTable + ((msbJumpToChar << 8) + lsbJumpToChar);
						error = drawInternal(xPos, yPos, currentCharWidth, textHeight, fontData, charDataPosition, charByteSize, fontColor);
					} // end if
				} // end if
			} // end if
		} // end for
	} // end for
	return error;
}



int drawStringInternal(int16_t xMove, int16_t yMove, char* text, uint16_t textLength, uint16_t textWidth) {
	uint8_t textHeight       = *(fontData + HEIGHT_POS);
	uint8_t firstChar        = *(fontData + FIRST_CHAR_POS);
	uint16_t sizeOfJumpTable = *(fontData + CHAR_NUM_POS)  * JUMPTABLE_BYTES;

	uint16_t cursorX         = 0;
	uint16_t cursorY         = 0;
	int error = 0;


	// Don't draw anything if it is not on the screen.
	//if (yMove < 0 || yMove > DisplayHeight) {return 0;}
	if (xMove + textWidth  < 0 || xMove > DisplayWidth ) {return 0;}
	if (yMove + textHeight < 0 || yMove > DisplayHeight) {return 0;}

	uint16_t j;
	for (j = 0; j < textLength; j++) {
		int16_t xPos = xMove + cursorX;
		int16_t yPos = yMove + cursorY;

		byte code = text[j];
		if (code >= firstChar) {
			byte charCode = code - firstChar;

			// 4 Bytes per char code
			byte msbJumpToChar    = *( fontData + JUMPTABLE_START + charCode * JUMPTABLE_BYTES );                  // MSB  \ JumpAddress
			byte lsbJumpToChar    = *( fontData + JUMPTABLE_START + charCode * JUMPTABLE_BYTES + JUMPTABLE_LSB);   // LSB /
			byte charByteSize     = *( fontData + JUMPTABLE_START + charCode * JUMPTABLE_BYTES + JUMPTABLE_SIZE);  // Size
			byte currentCharWidth = *( fontData + JUMPTABLE_START + charCode * JUMPTABLE_BYTES + JUMPTABLE_WIDTH); // Width

			// Test if the char is drawable
			if (!(msbJumpToChar == 255 && lsbJumpToChar == 255)) {
				// Get the position of the char data
				uint16_t charDataPosition = JUMPTABLE_START + sizeOfJumpTable + ((msbJumpToChar << 8) + lsbJumpToChar);
				error = drawInternal(xPos, yPos, currentCharWidth, textHeight, fontData, charDataPosition, charByteSize, fontColor);
			}

			cursorX += currentCharWidth;
		}
		if((error < 0)|(cursorX > DisplayWidth) |(cursorY > DisplayHeight)) {
			break;
		}
	}
	return 0;
}

int GFX_drawString(int16_t xMove, int16_t yMove, char* text) {
	uint16_t lineHeight = *(fontData + HEIGHT_POS);

	int16_t line = 0;
	char* textPart = strtok(text,"\n");
	while (textPart != NULL) {
		uint16_t length = strlen(textPart);
		drawStringInternal(xMove, yMove + (line++) * (lineHeight), textPart, length, getStringWidth(textPart, length));
		textPart = strtok(NULL, "\n");
	}
	return 0;
}

int GFX_printLine(char* text,uint16_t row, uint16_t col){
	uint16_t lineHeight = *(fontData + HEIGHT_POS);
	uint16_t lineWidth = *(fontData + WIDTH_POS);

	GFX_drawString(col * lineWidth, row * lineHeight, text);
	return 0;

}


void GFX_update_textBuffer(){

	drawStringBuffer(textBuffer, maskBuffer, MAXROWS, MAXCOLS);

	int j = 0;

	for(j = 0; j < BUFFERSIZE ; j++){ // clears mask buffer
		maskBuffer[j] = 0;
	}
}

void GFX_printText(char * text){
	static int cursor = 0;	// Keeps track what's the current char cursor
	static int line   = 0;	// keeps track of the lines
//    static char textBuffer[BUFFERSIZE];
//    char maskBuffer[BUFFERSIZE];

	int i = 0;
//	int j = 0;
//
//	for(j = 0; j < BUFFERSIZE ; j++){ // clears mask buffer
//		maskBuffer[j] = 0;
//	}

	while (text[i]){
		line = cursor / MAXCOLS;

		if(text[i] == '\n' || text[i] == '\r'){	// Add new line
			//cursor += (MAXCOLS - cursor) + (line * MAXCOLS);
			i++;

			for(int x = cursor; x < (line + 1) * MAXCOLS; x++){

				maskBuffer[cursor] = 1;  // sets mask only to the modified characters

				textBuffer[cursor] = ' ';
				cursor ++;

			if(cursor >= BUFFERSIZE){	// puts the cursor back to the top
				cursor = 0;
			}
			}

			continue;
		}
		if(text[i] == 0x7f){	// handles the backspace
			cursor--;
			i++;
			continue;
		}

		if(cursor >= BUFFERSIZE){	// puts the cursor back to the top
			cursor = 0;
		}

		maskBuffer[cursor] = (textBuffer[cursor] != text[i])? 1:0;  // sets mask only to the modified characters

		textBuffer[cursor] = text[i];

		cursor++;
		i++;
	}


//	drawStringBuffer(textBuffer, maskBuffer, MAXROWS, MAXCOLS);
}


void GFX_printText_rc(char * text, int row, int col){
	int cursor = row * MAXCOLS + col  ;	// Keeps track what's the current char cursor
	int line   = 0;	// keeps track of the lines
//    char textBuffer[BUFFERSIZE];
//    char maskBuffer[BUFFERSIZE];

	int i = 0;
//	int j = 0;
//
//	for(j = 0; j < BUFFERSIZE ; j++){ // clears mask buffer
//		maskBuffer[j] = 0;
//	}

	while (text[i]){
		line = cursor / MAXCOLS;

		if(text[i] == '\n' || text[i] == '\r'){	// Add new line
			cursor += (MAXCOLS - cursor) + (line * MAXCOLS);
			i++;
			continue;
		}
		if(text[i] == 0x7f){	// handles the backspace
			cursor--;
			i++;
			continue;
		}

		if(cursor >= BUFFERSIZE){	// puts the cursor back to the top
			cursor = 0;
		}

		maskBuffer[cursor] = (textBuffer[cursor] != text[i])? 1:0;  // sets mask only to the modified characters

		textBuffer[cursor] = text[i];

		cursor++;
		i++;
	}


//	drawStringBuffer(textBuffer, maskBuffer, MAXROWS, MAXCOLS);
}


