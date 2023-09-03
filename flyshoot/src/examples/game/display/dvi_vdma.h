#ifndef DVI_VDMA_H_
#define DVI_VDMA_H_

#include <xil_types.h>

#define DVI_HORIZONTAL 640
#define DVI_VERTICAL 480
#define DVI_BYTES_PER_LINE 3
#define DVI_TOTALMEM DVI_HORIZONTAL * DVI_VERTICAL * DVI_BYTES_PER_LINE

int DVI_initDVI(void);
void Display_sendPA(uint32_t const *pa, uint16_t y, uint16_t h);

void flushMem(void);

#endif
