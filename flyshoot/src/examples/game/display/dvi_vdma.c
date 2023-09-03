#include "dvi_vdma.h"
#include "xil_io.h"
#include "stdlib.h"
#include "xil_cache.h"
#include "GFX.h"

static UINTPTR memptr;
unsigned int videoMem[DVI_HORIZONTAL][DVI_VERTICAL];

void flushMem(void) {
	Xil_DCacheFlushRange(memptr, DVI_TOTALMEM);
}

void DVI_drawOutline(void) {

	GFX_changePenColor(0x00);	// change color to Black
	GFX_fillRect(0, 0, DVI_HORIZONTAL, DVI_VERTICAL);

	GFX_changePenColor(0xff);
	drawLine(0, 0, 0, DVI_VERTICAL - 1);
	drawLine(DVI_HORIZONTAL - 1, 0, DVI_HORIZONTAL - 1, DVI_VERTICAL - 1);

	drawLine(0, 0, DVI_HORIZONTAL - 1, 0);
	drawLine(0, DVI_VERTICAL - 1, DVI_HORIZONTAL - 1, DVI_VERTICAL - 1);

	GFX_changePenColor(0xff);	//change color to White

	Xil_DCacheFlushRange(memptr, DVI_TOTALMEM);

}

static void DVI_ConfigureVDMA(UINTPTR mempoint, int htzl, int vtl,
		int bytes_per_line) {

//    xil_printf("version:0x%08X\n", *((int*)0x4300002c));

	Xil_Out32(XPAR_AXI_VDMA_0_BASEADDR + 0x00, 0x00); 		//Stop DMA
	Xil_Out32(XPAR_AXI_VDMA_0_BASEADDR + 0x00, 0x01); 	//Start DMA in park mode

	Xil_Out32(XPAR_AXI_VDMA_0_BASEADDR + 0x5C, mempoint); 	//Pass DMA pointers
//	Xil_Out32(XPAR_AXI_VDMA_0_BASEADDR + 0x60, mempoint); 	//Pass DMA pointers
//	Xil_Out32(XPAR_AXI_VDMA_0_BASEADDR + 0x64, mempoint); 	//Pass DMA pointers

//    xil_printf("Mem Location:0x%08X\n", *((int*)0x4300005c));

	Xil_Out32(XPAR_AXI_VDMA_0_BASEADDR + 0x58, (htzl) * bytes_per_line); //Configures the htzl
	Xil_Out32(XPAR_AXI_VDMA_0_BASEADDR + 0x54, (htzl) * bytes_per_line);
	Xil_Out32(XPAR_AXI_VDMA_0_BASEADDR + 0x50, vtl); 		//Configures lines

	Xil_Out32(XPAR_AXI_VDMA_0_BASEADDR + 0x28, 0x00); // in park mode, sets the pointer where it will park

//    xil_printf("status:0x%08X\n", *((int*)0x43000004));
}

static int DVI_initVDMA(void) {
//	memptr = (UINTPTR) malloc(
//			DVI_HORIZONTAL * DVI_VERTICAL * DVI_BYTES_PER_LINE);

	memptr = (UINTPTR) &videoMem;
	DVI_ConfigureVDMA(memptr, DVI_HORIZONTAL, DVI_VERTICAL, DVI_BYTES_PER_LINE);

	return 0;
}

int DVI_initDVI(void) {
	DVI_initVDMA();

	GFX_init(memptr, DVI_HORIZONTAL, DVI_VERTICAL);
	return 0;
}

void Display_sendPA(uint32_t const *pa, uint16_t y, uint16_t h) {
	uint32_t i, j, k;
	uint32_t const *p = pa;
//	uint16_t cmd;

	/* Need to adjust start row by one because LS013B7DH03 starts
	 * counting lines from 1, while the DISPLAY interface starts from 0.
	 */
//	++y;
	for (i = y; i < DVI_VERTICAL; i++) {

		for (j = 0; j < h; j += 32, p ++) {

			for (k = 0; k < 32; k++) {
				if (*p & (1 << k)) {
					setPixelColor(k + j,i, 0xff);
				} else {
					setPixelColor(k + j,i, 0x00);
				}
			}
		}
	}

	/* Assert SCS */
//	PAL_GpioPinOutSet(LCD_PORT_SCS, LCD_PIN_SCS);
	/* SCS setup time: min 6us */
//	PAL_TimerMicroSecondsDelay(6);
	/* Send update command and first line address */
//	cmd = LS013B7DH03_CMD_UPDATE | (y << 8);
//	PAL_SpiTransmit((uint8_t *) &cmd, 2);
	/* send pixels for all lines except the last... */
//	for (i = 0; i < h - 1; ++i) {
//		PAL_SpiTransmit((uint8_t *) p, LS013B7DH03_WIDTH / 8);
//		p += (LS013B7DH03_WIDTH / 8);
//		cmd = 0xFFU | ((y + i + 1) << 8);
//		PAL_SpiTransmit((uint8_t *) &cmd, 2);
//	}
	/* send pixels for the last line */
//	PAL_SpiTransmit((uint8_t *) p, LS013B7DH03_WIDTH / 8);
//	cmd = 0xFFFFU;
//	PAL_SpiTransmit((uint8_t *) &cmd, 2);
	/* SCS hold time: min 2us */
//	PAL_TimerMicroSecondsDelay(2);
	/* De-assert SCS */
//	PAL_GpioPinOutClear(LCD_PORT_SCS, LCD_PIN_SCS);
}
