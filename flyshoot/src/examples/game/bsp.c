#include "qpc.h"
//#include "dpp.h"
#include "game.h"
#include "bsp.h"

#include "xparameters.h"
#include "xgpio.h"
#include "xil_exception.h"

#include "xscugic.h"
#include "xil_printf.h"

#include "display/dvi_vdma.h"
#include "display/GFX.h"
#include "stdlib.h"

#include "hcd/hcd.h"
#include "hcd/hcd_hw.h"
#include "hcd/hid.h"


extern XScuGic XScuGicInstance;
//extern XScuGic xInterruptController; 	/* Interrupt controller instance */
hcd_t* hcdPtr = NULL;
TaskHandle_t tskConfigHandle;


#define LEDBASE_DATA 0x41200000 + 8
#define SWBASE_DATA  0x41200000 + 0
#define LED_RED     (0)
#define LED_BLUE    (1)
#define LED_GREEN   (2)
#define LED_DBG    (3)
#define BTN_SW0   (1U << 0)
#define BTN_SW1   (1U << 1)
#define BTN_SW2   (1U << 2)
#define BTN_SW3   (1U << 3)

Q_DEFINE_THIS_FILE /* define the name of this file for assertions */

/* Local-scope objects ------------------------------------------------------*/
static uint32_t l_rnd; // random seed

/* LCD geometry and frame buffer */
static uint32_t l_fb[BSP_SCREEN_HEIGHT + 1][BSP_SCREEN_WIDTH / 32U];

/* the walls buffer */
static uint32_t l_walls[GAME_TUNNEL_HEIGHT + 1][BSP_SCREEN_WIDTH / 32U];

#ifdef Q_SPY
QSTimeCtr QS_tickTime_;
QSTimeCtr QS_tickPeriod_;
static uint8_t const l_TickHook = 0U;
/* QSpy source IDs */
static QSpyId const l_SysTick_Handler = { 0U };
static QSpyId const l_GPIO_EVEN_IRQHandler = { 0U };
#define UART_TXFIFO_DEPTH   16U
//    static USART_TypeDef * const l_USART0 = ((USART_TypeDef *)(0x40010000UL));

enum AppRecords { /* application-specific trace records */
	SCORE_STAT = QS_USER, COMMAND_STAT
};

#endif

static void paintBits(uint16_t x, uint16_t y, uint8_t const *bits, uint16_t h) {
	uint32_t *fb = &l_fb[y][x >> 5];
	uint32_t shft = (x & 0x1FU);
	for (y = 0; y < h; ++y, fb += (BSP_SCREEN_WIDTH >> 5)) {
		*fb |= ((uint32_t) bits[y] << shft);
		if (shft > 24U) {
			*(fb + 1) |= ((uint32_t) bits[y] >> (32U - shft));
		}
	}
}

static void paintBitsClear(uint16_t x, uint16_t y, uint8_t const *bits,
		uint8_t h) {
	uint32_t *fb = &l_fb[y][x >> 5]; //(uint32_t *)l_fb[y][x >> 5];
	uint32_t shft = (x & 0x1FU);
	uint32_t mask1 = ~((uint32_t) 0xFFU << shft);
	uint32_t mask2;
	if (shft > 24U) {
		mask2 = ~(0xFFU >> (32U - shft));
	}
	for (y = 0; y < h; ++y, fb += (BSP_SCREEN_WIDTH >> 5)) {
		*fb = ((*fb & mask1) | ((uint32_t) bits[y] << shft));
		if (shft > 24U) {
			*(fb + 1) = ((*(fb + 1) & mask2)
					| ((uint32_t) bits[y] >> (32U - shft)));
		}
	}

}

unsigned int BSP_sw0_state(void) {
	return Xil_In32(SWBASE_DATA) & 0x1;
}
unsigned int BSP_sw1_state(void) {
	return Xil_In32(SWBASE_DATA) & 0x2;
}

unsigned int BSP_sw_state(void) {
	return Xil_In32(SWBASE_DATA);
}

void setLED(u8 led) {
	Xil_Out32(LEDBASE_DATA, Xil_In32(LEDBASE_DATA) | (1 << led));
}
void clearLED(u8 led) {
	Xil_Out32(LEDBASE_DATA, Xil_In32(LEDBASE_DATA) & ~(1 << led));
}

static void configHWInterrupts(void *p) {
	int status;

	status = hcd_start(hcdPtr, &XScuGicInstance);// Start interrupts and handles the enumeration of devices registers interrupt handler
	if (status == HCD_ERROR)
		return;

	hid_requestReport(hcdPtr);
	vTaskDelete(tskConfigHandle);
}

void BSP_init(void) {

	/* initialize the QS software tracing */
	if (QS_INIT((void *)0) == 0U) {
		Q_ERROR();
	}

	QS_OBJ_DICTIONARY(&l_SysTick_Handler);
	QS_OBJ_DICTIONARY(&l_GPIO_EVEN_IRQHandler);
	QS_USR_DICTIONARY(SCORE_STAT);
	QS_USR_DICTIONARY(COMMAND_STAT);

	/* setup the QS filters... */
	QS_GLB_FILTER(QS_SM_RECORDS); /* state machine records */
	QS_GLB_FILTER(QS_AO_RECORDS); /* active object records */
	QS_GLB_FILTER(QS_UA_RECORDS); /* all user records */

	DVI_initDVI();


    hcdPtr = hcd_init();   // Initialize pointers, Create memory refs and initialize HW
//    if(hcdPtr == NULL) return HCD_ERROR;

    hcd_connectClassHandler(hcdPtr, hid_callbackHandler, hcdPtr);
//	if(status == HCD_ERROR) return status;

	xTaskCreate((TaskFunction_t) configHWInterrupts, "initSW",
			(short) configMINIMAL_STACK_SIZE, 0,
			(BaseType_t) 2 | portPRIVILEGE_BIT, &tskConfigHandle);


}

void BSP_terminate(int16_t result) {
	(void) result;
}

void BSP_randomSeed(uint32_t seed) {
	l_rnd = seed;
}

uint32_t BSP_random(void) { /* a very cheap pseudo-random-number generator */
	/* Some flating point code is to exercise the VFP... */
	float volatile x = 3.1415926F;
	x = x + 2.7182818F;

	vTaskSuspendAll(); /* lock FreeRTOS scheduler */
	/* "Super-Duper" Linear Congruential Generator (LCG)
	 * LCG(2^32, 3*7*11*13*23, 0, seed)
	 */
	l_rnd = l_rnd * (3U * 7U * 11U * 13U * 23U);
	xTaskResumeAll(); /* unlock the FreeRTOS scheduler */

	return l_rnd >> 8;
}

void BSP_updateScreen(void) {
	setLED(LED_DBG);
	Display_sendPA(&l_fb[0][0], 0, BSP_SCREEN_WIDTH);
	clearLED(LED_DBG);
	flushMem();
}
void BSP_clearFB(void) {
	uint_fast8_t y, z;
	for (y = 0U; y < BSP_SCREEN_HEIGHT; ++y) {
		for (z = 0U; z < BSP_SCREEN_WIDTH / 32; ++z) {
			l_fb[y][z] = 0U;

		}
	}
}
void BSP_clearWalls(void) {
	uint_fast8_t y, z;
	for (y = 0U; y < GAME_TUNNEL_HEIGHT; ++y) {
		for (z = 0U; z < GAME_TUNNEL_WIDTH / 32; ++z) {
			l_walls[y][z] = 0U;
		}
	}
}
void BSP_paintString(uint16_t x, uint16_t y, char const *str) {
	static uint8_t const font5x7[95][7] = { { 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
			0x00U, 0x00U }, /*   */
	{ 0x04U, 0x04U, 0x04U, 0x04U, 0x00U, 0x00U, 0x04U }, /* ! */
	{ 0x0AU, 0x0AU, 0x0AU, 0x00U, 0x00U, 0x00U, 0x00U }, /* " */
	{ 0x0AU, 0x0AU, 0x1FU, 0x0AU, 0x1FU, 0x0AU, 0x0AU }, /* # */
	{ 0x04U, 0x1EU, 0x05U, 0x0EU, 0x14U, 0x0FU, 0x04U }, /* $ */
	{ 0x03U, 0x13U, 0x08U, 0x04U, 0x02U, 0x19U, 0x18U }, /* % */
	{ 0x06U, 0x09U, 0x05U, 0x02U, 0x15U, 0x09U, 0x16U }, /* & */
	{ 0x06U, 0x04U, 0x02U, 0x00U, 0x00U, 0x00U, 0x00U }, /* ' */
	{ 0x08U, 0x04U, 0x02U, 0x02U, 0x02U, 0x04U, 0x08U }, /* ( */
	{ 0x02U, 0x04U, 0x08U, 0x08U, 0x08U, 0x04U, 0x02U }, /* ) */
	{ 0x00U, 0x04U, 0x15U, 0x0EU, 0x15U, 0x04U, 0x00U }, /* * */
	{ 0x00U, 0x04U, 0x04U, 0x1FU, 0x04U, 0x04U, 0x00U }, /* + */
	{ 0x00U, 0x00U, 0x00U, 0x00U, 0x06U, 0x04U, 0x02U }, /* , */
	{ 0x00U, 0x00U, 0x00U, 0x1FU, 0x00U, 0x00U, 0x00U }, /* - */
	{ 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x06U, 0x06U }, /* . */
	{ 0x00U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U, 0x00U }, /* / */
	{ 0x0EU, 0x11U, 0x19U, 0x15U, 0x13U, 0x11U, 0x0EU }, /* 0 */
	{ 0x04U, 0x06U, 0x04U, 0x04U, 0x04U, 0x04U, 0x0EU }, /* 1 */
	{ 0x0EU, 0x11U, 0x10U, 0x08U, 0x04U, 0x02U, 0x1FU }, /* 2 */
	{ 0x1FU, 0x08U, 0x04U, 0x08U, 0x10U, 0x11U, 0x0EU }, /* 3 */
	{ 0x08U, 0x0CU, 0x0AU, 0x09U, 0x1FU, 0x08U, 0x08U }, /* 4 */
	{ 0x1FU, 0x01U, 0x0FU, 0x10U, 0x10U, 0x11U, 0x0EU }, /* 5 */
	{ 0x0CU, 0x02U, 0x01U, 0x0FU, 0x11U, 0x11U, 0x0EU }, /* 6 */
	{ 0x1FU, 0x10U, 0x08U, 0x04U, 0x02U, 0x02U, 0x02U }, /* 7 */
	{ 0x0EU, 0x11U, 0x11U, 0x0EU, 0x11U, 0x11U, 0x0EU }, /* 8 */
	{ 0x0EU, 0x11U, 0x11U, 0x1EU, 0x10U, 0x08U, 0x06U }, /* 9 */
	{ 0x00U, 0x06U, 0x06U, 0x00U, 0x06U, 0x06U, 0x00U }, /* : */
	{ 0x00U, 0x06U, 0x06U, 0x00U, 0x06U, 0x04U, 0x02U }, /* ; */
	{ 0x08U, 0x04U, 0x02U, 0x01U, 0x02U, 0x04U, 0x08U }, /* < */
	{ 0x00U, 0x00U, 0x1FU, 0x00U, 0x1FU, 0x00U, 0x00U }, /* = */
	{ 0x02U, 0x04U, 0x08U, 0x10U, 0x08U, 0x04U, 0x02U }, /* > */
	{ 0x0EU, 0x11U, 0x10U, 0x08U, 0x04U, 0x00U, 0x04U }, /* ? */
	{ 0x0EU, 0x11U, 0x10U, 0x16U, 0x15U, 0x15U, 0x0EU }, /* @ */
	{ 0x0EU, 0x11U, 0x11U, 0x11U, 0x1FU, 0x11U, 0x11U }, /* A */
	{ 0x0FU, 0x11U, 0x11U, 0x0FU, 0x11U, 0x11U, 0x0FU }, /* B */
	{ 0x0EU, 0x11U, 0x01U, 0x01U, 0x01U, 0x11U, 0x0EU }, /* C */
	{ 0x07U, 0x09U, 0x11U, 0x11U, 0x11U, 0x09U, 0x07U }, /* D */
	{ 0x1FU, 0x01U, 0x01U, 0x0FU, 0x01U, 0x01U, 0x1FU }, /* E */
	{ 0x1FU, 0x01U, 0x01U, 0x0FU, 0x01U, 0x01U, 0x01U }, /* F */
	{ 0x0EU, 0x11U, 0x01U, 0x1DU, 0x11U, 0x11U, 0x1EU }, /* G */
	{ 0x11U, 0x11U, 0x11U, 0x1FU, 0x11U, 0x11U, 0x11U }, /* H */
	{ 0x0EU, 0x04U, 0x04U, 0x04U, 0x04U, 0x04U, 0x0EU }, /* I */
	{ 0x1CU, 0x08U, 0x08U, 0x08U, 0x08U, 0x09U, 0x06U }, /* J */
	{ 0x11U, 0x09U, 0x05U, 0x03U, 0x05U, 0x09U, 0x11U }, /* K */
	{ 0x01U, 0x01U, 0x01U, 0x01U, 0x01U, 0x01U, 0x1FU }, /* L */
	{ 0x11U, 0x1BU, 0x15U, 0x15U, 0x11U, 0x11U, 0x11U }, /* M */
	{ 0x11U, 0x11U, 0x13U, 0x15U, 0x19U, 0x11U, 0x11U }, /* N */
	{ 0x0EU, 0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x0EU }, /* O */
	{ 0x0FU, 0x11U, 0x11U, 0x0FU, 0x01U, 0x01U, 0x01U }, /* P */
	{ 0x0EU, 0x11U, 0x11U, 0x11U, 0x15U, 0x09U, 0x16U }, /* Q */
	{ 0x0FU, 0x11U, 0x11U, 0x0FU, 0x05U, 0x09U, 0x11U }, /* R */
	{ 0x1EU, 0x01U, 0x01U, 0x0EU, 0x10U, 0x10U, 0x0FU }, /* S */
	{ 0x1FU, 0x04U, 0x04U, 0x04U, 0x04U, 0x04U, 0x04U }, /* T */
	{ 0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x0EU }, /* U */
	{ 0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x0AU, 0x04U }, /* V */
	{ 0x11U, 0x11U, 0x11U, 0x15U, 0x15U, 0x15U, 0x0AU }, /* W */
	{ 0x11U, 0x11U, 0x0AU, 0x04U, 0x0AU, 0x11U, 0x11U }, /* X */
	{ 0x11U, 0x11U, 0x11U, 0x0AU, 0x04U, 0x04U, 0x04U }, /* Y */
	{ 0x1FU, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U, 0x1FU }, /* Z */
	{ 0x0EU, 0x02U, 0x02U, 0x02U, 0x02U, 0x02U, 0x0EU }, /* [ */
	{ 0x00U, 0x01U, 0x02U, 0x04U, 0x08U, 0x10U, 0x00U }, /* \ */
	{ 0x0EU, 0x08U, 0x08U, 0x08U, 0x08U, 0x08U, 0x0EU }, /* ] */
	{ 0x04U, 0x0AU, 0x11U, 0x00U, 0x00U, 0x00U, 0x00U }, /* ^ */
	{ 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x1FU }, /* _ */
	{ 0x02U, 0x04U, 0x08U, 0x00U, 0x00U, 0x00U, 0x00U }, /* ` */
	{ 0x00U, 0x00U, 0x0EU, 0x10U, 0x1EU, 0x11U, 0x1EU }, /* a */
	{ 0x01U, 0x01U, 0x0DU, 0x13U, 0x11U, 0x11U, 0x0FU }, /* b */
	{ 0x00U, 0x00U, 0x0EU, 0x01U, 0x01U, 0x11U, 0x0EU }, /* c */
	{ 0x10U, 0x10U, 0x16U, 0x19U, 0x11U, 0x11U, 0x1EU }, /* d */
	{ 0x00U, 0x00U, 0x0EU, 0x11U, 0x1FU, 0x01U, 0x0EU }, /* e */
	{ 0x0CU, 0x12U, 0x02U, 0x07U, 0x02U, 0x02U, 0x02U }, /* f */
	{ 0x00U, 0x1EU, 0x11U, 0x11U, 0x1EU, 0x10U, 0x0EU }, /* g */
	{ 0x01U, 0x01U, 0x0DU, 0x13U, 0x11U, 0x11U, 0x11U }, /* h */
	{ 0x04U, 0x00U, 0x06U, 0x04U, 0x04U, 0x04U, 0x0EU }, /* i */
	{ 0x08U, 0x00U, 0x0CU, 0x08U, 0x08U, 0x09U, 0x06U }, /* j */
	{ 0x01U, 0x01U, 0x09U, 0x05U, 0x03U, 0x05U, 0x09U }, /* k */
	{ 0x06U, 0x04U, 0x04U, 0x04U, 0x04U, 0x04U, 0x0EU }, /* l */
	{ 0x00U, 0x00U, 0x0BU, 0x15U, 0x15U, 0x11U, 0x11U }, /* m */
	{ 0x00U, 0x00U, 0x0DU, 0x13U, 0x11U, 0x11U, 0x11U }, /* n */
	{ 0x00U, 0x00U, 0x0EU, 0x11U, 0x11U, 0x11U, 0x0EU }, /* o */
	{ 0x00U, 0x00U, 0x0FU, 0x11U, 0x0FU, 0x01U, 0x01U }, /* p */
	{ 0x00U, 0x00U, 0x16U, 0x19U, 0x1EU, 0x10U, 0x10U }, /* q */
	{ 0x00U, 0x00U, 0x0DU, 0x13U, 0x01U, 0x01U, 0x01U }, /* r */
	{ 0x00U, 0x00U, 0x0EU, 0x01U, 0x0EU, 0x10U, 0x0FU }, /* s */
	{ 0x02U, 0x02U, 0x07U, 0x02U, 0x02U, 0x12U, 0x0CU }, /* t */
	{ 0x00U, 0x00U, 0x11U, 0x11U, 0x11U, 0x19U, 0x16U }, /* u */
	{ 0x00U, 0x00U, 0x11U, 0x11U, 0x11U, 0x0AU, 0x04U }, /* v */
	{ 0x00U, 0x00U, 0x11U, 0x11U, 0x15U, 0x15U, 0x0AU }, /* w */
	{ 0x00U, 0x00U, 0x11U, 0x0AU, 0x04U, 0x0AU, 0x11U }, /* x */
	{ 0x00U, 0x00U, 0x11U, 0x11U, 0x1EU, 0x10U, 0x0EU }, /* y */
	{ 0x00U, 0x00U, 0x1FU, 0x08U, 0x04U, 0x02U, 0x1FU }, /* z */
	{ 0x08U, 0x04U, 0x04U, 0x02U, 0x04U, 0x04U, 0x08U }, /* { */
	{ 0x04U, 0x04U, 0x04U, 0x04U, 0x04U, 0x04U, 0x04U }, /* | */
	{ 0x02U, 0x04U, 0x04U, 0x08U, 0x04U, 0x04U, 0x02U }, /* } */
	{ 0x02U, 0x15U, 0x08U, 0x00U, 0x00U, 0x00U, 0x00U }, /* ~ */
	};

	for (; *str != '\0'; ++str, x += 6) {
		uint8_t const *ch = &font5x7[*str - ' '][0];
		paintBitsClear(x, y, ch, 7);
	}
}

/*==========================================================================*/
typedef struct { /* the auxiliary structure to hold const bitmaps */
	uint8_t const *bits; /* the bits in the bitmap */
	uint8_t height; /* the height of the bitmap */
} Bitmap;

/* bitmap of the Ship:
 *
 *     x....
 *     xxx..
 *     xxxxx
 */
static uint8_t const ship_bits[] = { 0x01U, 0x07U, 0x1FU };

/* bitmap of the Missile:
 *
 *     xxxx
 */
static uint8_t const missile_bits[] = { 0x0FU };

/* bitmap of the Mine type-1:
 *
 *     .x.
 *     xxx
 *     .x.
 */
static uint8_t const mine1_bits[] = { 0x02U, 0x07U, 0x02U };

/* bitmap of the Mine type-2:
 *
 *     x..x
 *     .xx.
 *     .xx.
 *     x..x
 */
static uint8_t const mine2_bits[] = { 0x09U, 0x06U, 0x06U, 0x09U };

/* Mine type-2 is nastier than Mine type-1. The type-2 mine can
 * hit the Ship with any of its "tentacles". However, it can be
 * destroyed by the Missile only by hitting its center, defined as
 * the following bitmap:
 *
 *     ....
 *     .xx.
 *     .xx.
 */
static uint8_t const mine2_missile_bits[] = { 0x00U, 0x06U, 0x06U };

/*
 * The bitmap of the explosion stage 0:
 *
 *     .......
 *     ...x...
 *     ..x.x..
 *     ...x...
 */
static uint8_t const explosion0_bits[] = { 0x00U, 0x08U, 0x14U, 0x08U };

/*
 * The bitmap of the explosion stage 1:
 *
 *     .......
 *     ..x.x..
 *     ...x...
 *     ..x.x..
 */
static uint8_t const explosion1_bits[] = { 0x00U, 0x14U, 0x08U, 0x14U };

/*
 * The bitmap of the explosion stage 2:
 *
 *     .x...x.
 *     ..x.x..
 *     ...x...
 *     ..x.x..
 *     .x...x.
 */
static uint8_t const explosion2_bits[] = { 0x11U, 0x0AU, 0x04U, 0x0AU, 0x11U };

/*
 * The bitmap of the explosion stage 3:
 *
 *     x..x..x
 *     .x.x.x.
 *     ..x.x..
 *     xx.x.xx
 *     ..x.x..
 *     .x.x.x.
 *     x..x..x
 */
static uint8_t const explosion3_bits[] = { 0x49, 0x2A, 0x14, 0x6B, 0x14, 0x2A,
		0x49 };

static Bitmap const l_bitmap[MAX_BMP] = { { ship_bits, Q_DIM(ship_bits) }, {
		missile_bits, Q_DIM(missile_bits) }, { mine1_bits, Q_DIM(mine1_bits) },
		{ mine2_bits, Q_DIM(mine2_bits) }, { mine2_missile_bits, Q_DIM(
				mine2_missile_bits) },
		{ explosion0_bits, Q_DIM(explosion0_bits) }, { explosion1_bits, Q_DIM(
				explosion1_bits) }, { explosion2_bits, Q_DIM(explosion2_bits) },
		{ explosion3_bits, Q_DIM(explosion3_bits) } };

void BSP_paintBitmap(uint16_t x, uint16_t y, uint8_t bmp_id) {
	Bitmap const *bmp = &l_bitmap[bmp_id];
	paintBits(x, y, bmp->bits, bmp->height);
}
void BSP_advanceWalls(uint16_t top, uint16_t bottom) {
	uint_fast8_t y;
	for (y = 0U; y < GAME_TUNNEL_HEIGHT; ++y) {
		/* shift the walls one pixel to the left */
		l_walls[y][0] = (l_walls[y][0] >> 1) | (l_walls[y][1] << 31);
		l_walls[y][1] = (l_walls[y][1] >> 1) | (l_walls[y][2] << 31);
		l_walls[y][2] = (l_walls[y][2] >> 1) | (l_walls[y][3] << 31);
		l_walls[y][3] = (l_walls[y][3] >> 1) | (l_walls[y][4] << 31);
		l_walls[y][4] = (l_walls[y][4] >> 1) | (l_walls[y][5] << 31);
		l_walls[y][5] = (l_walls[y][5] >> 1) | (l_walls[y][6] << 31);
		l_walls[y][6] = (l_walls[y][6] >> 1) | (l_walls[y][7] << 31);
		l_walls[y][7] = (l_walls[y][7] >> 1) | (l_walls[y][8] << 31);
		l_walls[y][8] = (l_walls[y][8] >> 1) | (l_walls[y][9] << 31);
		l_walls[y][9] = (l_walls[y][9] >> 1) | (l_walls[y][10] << 31);
		l_walls[y][10] = (l_walls[y][10] >> 1) | (l_walls[y][11] << 31);
		l_walls[y][11] = (l_walls[y][11] >> 1) | (l_walls[y][12] << 31);
		l_walls[y][12] = (l_walls[y][12] >> 1) | (l_walls[y][13] << 31);
		l_walls[y][13] = (l_walls[y][13] >> 1) | (l_walls[y][14] << 31);
		l_walls[y][14] = (l_walls[y][14] >> 1) | (l_walls[y][15] << 31);
		l_walls[y][15] = (l_walls[y][15] >> 1) | (l_walls[y][16] << 31);
		l_walls[y][16] = (l_walls[y][16] >> 1) | (l_walls[y][17] << 31);
		l_walls[y][17] = (l_walls[y][17] >> 1) | (l_walls[y][18] << 31);
		l_walls[y][18] = (l_walls[y][18] >> 1) | (l_walls[y][19] << 31);
		l_walls[y][19] = (l_walls[y][19] >> 1);

		/* add new column of walls at the end */
		if (y <= top) {
			l_walls[y][19] |= (1U << 31);
		}
		if (y >= (GAME_TUNNEL_HEIGHT - bottom)) {
			l_walls[y][19] |= (1U << 31);
		}

		/* copy the walls to the frame buffer */
		l_fb[y][0] = l_walls[y][0];
		l_fb[y][1] = l_walls[y][1];
		l_fb[y][2] = l_walls[y][2];
		l_fb[y][3] = l_walls[y][3];
		l_fb[y][4] = l_walls[y][4];
		l_fb[y][5] = l_walls[y][5];
		l_fb[y][6] = l_walls[y][6];
		l_fb[y][7] = l_walls[y][7];
		l_fb[y][8] = l_walls[y][8];
		l_fb[y][9] = l_walls[y][9];
		l_fb[y][10] = l_walls[y][10];
		l_fb[y][11] = l_walls[y][11];
		l_fb[y][12] = l_walls[y][12];
		l_fb[y][13] = l_walls[y][13];
		l_fb[y][14] = l_walls[y][14];
		l_fb[y][15] = l_walls[y][15];
		l_fb[y][16] = l_walls[y][16];
		l_fb[y][17] = l_walls[y][17];
		l_fb[y][18] = l_walls[y][18];
		l_fb[y][19] = l_walls[y][19];

	}
}
void BSP_updateScore(uint16_t score) {
	char str[5];
	uint16_t s = score;

	if (score == 0U) {
		BSP_paintString(1U, BSP_SCREEN_HEIGHT - 8U, "SCORE:");
	}

	/* update the SCORE area on the screeen */
	str[4] = '\0';
	str[3] = (s % 10U) + '0';
	s /= 10U;
	str[2] = (s % 10U) + '0';
	s /= 10U;
	str[1] = (s % 10U) + '0';
	s /= 10U;
	str[0] = (s % 10U) + '0';
	BSP_paintString(6U * 6U, BSP_SCREEN_HEIGHT - 8U, str);

	QS_BEGIN_ID(SCORE_STAT, 0U)
	/* app-specific record */
			QS_U16(4, score);QS_END()
}

bool BSP_isThrottle(void) {
	/* is the throttle button depressed? */
//	return (BSP_sw0_state() == 0x1);
	char* hid_data = readHID_Data();
	return ( *(hid_data + 4 ) == HID_UP_MASK );
}
bool BSP_doBitmapsOverlap(uint8_t bmp_id1, uint16_t x1, uint16_t y1,
		uint8_t bmp_id2, uint16_t x2, uint16_t y2) {
	uint16_t y;
	uint16_t y0;
	uint16_t h;
	uint32_t bits1;
	uint32_t bits2;
	Bitmap const *bmp1;
	Bitmap const *bmp2;

	Q_REQUIRE((bmp_id1 < Q_DIM(l_bitmap)) && (bmp_id2 < Q_DIM(l_bitmap)));

	/* are the bitmaps close enough in x? */
	if (x1 >= x2) {
		if (x1 > x2 + 8U) {
			return false;
		}
		x1 -= x2;
		x2 = 0U;
	} else {
		if (x2 > x1 + 8U) {
			return false;
		}
		x2 -= x1;
		x1 = 0U;
	}

	bmp1 = &l_bitmap[bmp_id1];
	bmp2 = &l_bitmap[bmp_id2];
	if ((y1 <= y2) && (y1 + bmp1->height > y2)) {
		y0 = y2 - y1;
		h = y1 + bmp1->height - y2;
		if (h > bmp2->height) {
			h = bmp2->height;
		}
		for (y = 0; y < h; ++y) { /* scan over the overlapping rows */
			bits1 = ((uint32_t) bmp1->bits[y + y0] << x1);
			bits2 = ((uint32_t) bmp2->bits[y] << x2);
			if ((bits1 & bits2) != 0U) { /* do the bits overlap? */
				return true; /* yes! */
			}
		}
	} else {
		if ((y1 > y2) && (y2 + bmp2->height > y1)) {
			y0 = y1 - y2;
			h = y2 + bmp2->height - y1;
			if (h > bmp1->height) {
				h = bmp1->height;
			}
			for (y = 0; y < h; ++y) { /* scan over the overlapping rows */
				bits1 = ((uint32_t) bmp1->bits[y] << x1);
				bits2 = ((uint32_t) bmp2->bits[y + y0] << x2);
				if ((bits1 & bits2) != 0U) { /* do the bits overlap? */
					return true; /* yes! */
				}
			}
		}
	}
	return false; /* the bitmaps do not overlap */
}
bool BSP_isWallHit(uint8_t bmp_id, uint16_t x, uint16_t y) {
	Bitmap const *bmp = &l_bitmap[bmp_id];
	uint32_t shft = (x & 0x1FU);
	uint32_t *walls = &l_walls[y][x >> 5];
	for (y = 0; y < bmp->height; ++y, walls += (BSP_SCREEN_WIDTH >> 5)) {
		if (*walls & ((uint32_t) bmp->bits[y] << shft)) {
			return true;
		}
		if (shft > 24U) {
			if (*(walls + 1) & ((uint32_t) bmp->bits[y] >> (32U - shft))) {
				return true;
			}
		}
	}
	return false;
}

void BSP_displayOn(void) {

}
void BSP_displayOff(void) {

}

void QF_onStartup(void) {

}

Q_NORETURN Q_onAssert(char const * const module, int_t const loc) {
	/*
	 * NOTE: add here your application-specific error handling
	 */
	(void) module;
	(void) loc;
	QS_ASSERTION(module, loc, 10000U); /* report assertion to QS */

//#ifndef NDEBUG
	/* light up all LEDs */
//    GPIOF_AHB->DATA_Bits[LED_GREEN | LED_RED | LED_BLUE] = 0xFFU;
	setLED(LED_GREEN);
	setLED(LED_RED);
	setLED(LED_BLUE);
	/* for debugging, hang on in an endless loop... */
	for (;;) {
	}
//#endif

//    NVIC_SystemReset();
}

void QF_onCleanup(void) {
}

/* QS callbacks ============================================================*/
#ifdef Q_SPY
/*..........................................................................*/
uint8_t QS_onStartup(void const *arg) {
	Q_UNUSED_PAR(arg);

	static uint8_t qsTxBuf[2 * 1024]; /* buffer for QS-TX channel */
	static uint8_t qsRxBuf[100]; /* buffer for QS-RX channel */

	QS_initBuf(qsTxBuf, sizeof(qsTxBuf));
	QS_rxInitBuf(qsRxBuf, sizeof(qsRxBuf));

	return 1U; /* return success */
}
/*..........................................................................*/
void QS_onCleanup(void) {
}
/*..........................................................................*/
QSTimeCtr QS_onGetTime(void) { /* NOTE: invoked with interrupts DISABLED */
	return xTaskGetTickCount(); //0;//TIMER5->TAV;
}
/*..........................................................................*/
void QS_onFlush(void) {
	while (true) {
		/* try to get next byte to transmit */
		QF_INT_DISABLE();
		uint16_t b = QS_getByte();
		QF_INT_ENABLE();

		if (b != QS_EOD) { /* NOT end-of-data */
			/* busy-wait as long as TX FIFO has data to transmit */
//            while ((UART0->FR & UART_FR_TXFE) == 0) {
//            }
			/* place the byte in the UART DR register */
//            UART0-> //DR = b;
			outbyte(b);
		} else {
			break; /* break out of the loop */
		}
	}
}
/*..........................................................................*/
/*! callback function to reset the target (to be implemented in the BSP) */
void QS_onReset(void) {
//    NVIC_SystemReset();
}
/*..........................................................................*/
/*! callback function to execute a user command (to be implemented in BSP) */
void QS_onCommand(uint8_t cmdId, uint32_t param1, uint32_t param2,
		uint32_t param3) {
	QS_BEGIN_ID(COMMAND_STAT, 0U)
	/* app-specific record */
			QS_U8(2, cmdId);
			QS_U32(8, param1);
			QS_U32(8, param2);
			QS_U32(8, param3);QS_END()
}

#endif /* Q_SPY */
/*--------------------------------------------------------------------------*/

/* Application hooks used in this project ==================================*/
/* NOTE: only the "FromISR" API variants are allowed in vApplicationTickHook */
void vApplicationTickHook(void) {

	static QEvt const tickEvt = { TIME_TICK_SIG, 0U, 0U };

#ifdef Q_SPY
	{
		/* clear SysTick_CTRL_COUNTFLAG */
//        uint32_t volatile tmp = SysTick->CTRL;
//        (void)tmp; /* avoid compiler warning about unused local variable */
		QS_tickTime_ += QS_tickPeriod_; /* account for the clock rollover */
	}
#endif

	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	/* process time events for rate 0 */
	QTIMEEVT_TICK_FROM_ISR(0U, &xHigherPriorityTaskWoken, &l_TickHook);

	static uint8_t count = 5;
	if (count == 0) {
		QACTIVE_PUBLISH_FROM_ISR(&tickEvt, &xHigherPriorityTaskWoken,
				&l_SysTick_Handler); /* publish to all subscribers */
		count = 5;
		hcd_enqueNextPeriodicQH(hcdPtr);
	}
	count--;

	char* hid_data = readHID_Data();
	if (*(hid_data + 5) == HID_A_MASK) {
		static QEvt const trigEvt = { PLAYER_TRIGGER_SIG, 0U, 0U };
		QACTIVE_PUBLISH_FROM_ISR(&trigEvt, &xHigherPriorityTaskWoken,
				&l_SysTick_Handler);
	}
	/* notify FreeRTOS to perform context switch from ISR, if needed */
	portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}
/*..........................................................................*/
//void vApplicationIdleHook(void) {}

void vApplicationIdleHook(void) {
	/* toggle the User LED on and then off, see NOTE01 */
	QF_INT_DISABLE();
	setLED(LED_BLUE);
	clearLED(LED_BLUE);
//    GPIOF_AHB->DATA_Bits[LED_BLUE] = 0xFFU;  /* turn the Blue LED on  */
//    GPIOF_AHB->DATA_Bits[LED_BLUE] = 0U;     /* turn the Blue LED off */
	QF_INT_ENABLE();

	/* Some floating point code is to exercise the VFP... */
	float volatile x = 1.73205F;
	x = x * 1.73205F;

#ifdef Q_SPY
	QS_rxParse(); /* parse all the received bytes */

//    if ((UART0->FR & UART_FR_TXFE) != 0U) {  /* TX done? */
	uint16_t fifo = UART_TXFIFO_DEPTH; /* max bytes we can accept */
	uint8_t const *block;

	QF_INT_DISABLE();
	block = QS_getBlock(&fifo); /* try to get next block to transmit */
	QF_INT_ENABLE();

	while (fifo-- != 0) { /* any bytes in the block? */
//            UART0->DR = *block++;  /* put into the FIFO */
		outbyte(*block++);
	}
//    }
#elif defined NDEBUG
	/* Put the CPU and peripherals to the low-power mode.
	 * you might need to customize the clock management for your application,
	 * see the datasheet for your particular Cortex-M3 MCU.
	 */
	__WFI(); /* Wait-For-Interrupt */
#endif
}
/*..........................................................................*/
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
	(void) xTask;
	(void) pcTaskName;
	Q_ERROR();
}
/*..........................................................................*/
/* configSUPPORT_STATIC_ALLOCATION is set to 1, so the application must
 * provide an implementation of vApplicationGetIdleTaskMemory() to provide
 * the memory that is used by the Idle task.
 */
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
		StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize) {
	/* If the buffers to be provided to the Idle task are declared inside
	 * this function then they must be declared static - otherwise they will
	 * be allocated on the stack and so not exists after this function exits.
	 */
	static StaticTask_t xIdleTaskTCB;
	static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];

	/* Pass out a pointer to the StaticTask_t structure in which the
	 * Idle task's state will be stored.
	 */
	*ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

	/* Pass out the array that will be used as the Idle task's stack. */
	*ppxIdleTaskStackBuffer = uxIdleTaskStack;

	/* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
	 * Note that, as the array is necessarily of type StackType_t,
	 * configMINIMAL_STACK_SIZE is specified in words, not bytes.
	 */
	*pulIdleTaskStackSize = Q_DIM(uxIdleTaskStack);
}

