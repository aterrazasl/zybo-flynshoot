#include "qpc.h"
#include "dpp.h"
#include "bsp.h"

#include "xparameters.h"
#include "xgpio.h"
#include "xil_exception.h"

#include "xscugic.h"
#include "xil_printf.h"


#define LEDBASE_DATA 0x41200000 + 8
#define SWBASE_DATA  0x41200000 + 0
#define LED_RED     (0)
#define LED_GREEN   (2)
#define LED_BLUE    (1)
#define BTN_SW0   (1U << 0)
#define BTN_SW1   (1U << 1)
#define BTN_SW2   (1U << 2)
#define BTN_SW3   (1U << 3)

Q_DEFINE_THIS_FILE  /* define the name of this file for assertions */

/* Local-scope objects ------------------------------------------------------*/
static uint32_t l_rnd; // random seed
#ifdef Q_SPY

    /* QS identifiers for non-QP sources of events */
    static uint8_t const l_TickHook = 0U;
    static uint8_t const l_GPIOPortA_IRQHandler = 0U;

    #define UART_BAUD_RATE      115200U
    #define UART_FR_TXFE        (1U << 7)
    #define UART_FR_RXFE        (1U << 4)
    #define UART_TXFIFO_DEPTH   16U

    enum AppRecords { /* application-specific trace records */
        PHILO_STAT = QS_USER,
        PAUSED_STAT,
        COMMAND_STAT
    };

#endif

unsigned int BSP_sw0_state(void){
	return Xil_In32(SWBASE_DATA) & 0x1;
}
unsigned int BSP_sw1_state(void){
	return Xil_In32(SWBASE_DATA) & 0x2;
}

void setLED(u8 led) {
	Xil_Out32(LEDBASE_DATA, Xil_In32(LEDBASE_DATA) | (1 << led));
}
void clearLED(u8 led) {
	Xil_Out32(LEDBASE_DATA, Xil_In32(LEDBASE_DATA) & ~(1 << led));
}



/* Application hooks used in this project ==================================*/
/* NOTE: only the "FromISR" API variants are allowed in vApplicationTickHook */
void vApplicationTickHook(void) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /* process time events for rate 0 */
    QTIMEEVT_TICK_FROM_ISR(0U, &xHigherPriorityTaskWoken, &l_TickHook);

    /* Perform the debouncing of buttons. The algorithm for debouncing
    * adapted from the book "Embedded Systems Dictionary" by Jack Ganssle
    * and Michael Barr, page 71.
    */
    /* state of the button debouncing, see below */
    static struct ButtonsDebouncing {
        uint32_t depressed;
        uint32_t previous;
    } buttons = { 0U, 0U };
    uint32_t current = BSP_sw1_state(); //current = ~GPIOF_AHB->DATA_Bits[BTN_SW1 | BTN_SW2]; /* read SW1&SW2 */
    uint32_t tmp = buttons.depressed; /* save debounced depressed buttons */
    buttons.depressed |= (buttons.previous & current); /* set depressed */
    buttons.depressed &= (buttons.previous | current); /* clear released */
    buttons.previous   = current; /* update the history */
    tmp ^= buttons.depressed;     /* changed debounced depressed */
    if ((tmp & BTN_SW1) != 0U) {  /* debounced SW1 state changed? */
        if ((buttons.depressed & BTN_SW1) != 0U) { /* is SW1 depressed? */
            /* demonstrate the "FromISR APIs:
            * QACTIVE_PUBLISH_FROM_ISR() and Q_NEW_FROM_ISR()
            */
            QACTIVE_PUBLISH_FROM_ISR(Q_NEW_FROM_ISR(QEvt, PAUSE_SIG),
                                &xHigherPriorityTaskWoken,
                                &l_TickHook);
        }
        else { /* the button is released */
            /* demonstrate the ISR APIs: POST_FROM_ISR and Q_NEW_FROM_ISR */
            QACTIVE_POST_FROM_ISR(AO_Table,
                                  Q_NEW_FROM_ISR(QEvt, SERVE_SIG),
                                  &xHigherPriorityTaskWoken,
                                  &l_TickHook);
        }
    }

    /* notify FreeRTOS to perform context switch from ISR, if needed */
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}
/*..........................................................................*/
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
    QS_rxParse();  /* parse all the received bytes */

//    if ((UART0->FR & UART_FR_TXFE) != 0U) {  /* TX done? */
        uint16_t fifo = UART_TXFIFO_DEPTH;   /* max bytes we can accept */
        uint8_t const *block;

        QF_INT_DISABLE();
        block = QS_getBlock(&fifo);  /* try to get next block to transmit */
        QF_INT_ENABLE();

        while (fifo-- != 0) {  /* any bytes in the block? */
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
    (void)xTask;
    (void)pcTaskName;
    Q_ERROR();
}
/*..........................................................................*/
/* configSUPPORT_STATIC_ALLOCATION is set to 1, so the application must
* provide an implementation of vApplicationGetIdleTaskMemory() to provide
* the memory that is used by the Idle task.
*/
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    uint32_t *pulIdleTaskStackSize )
{
    /* If the buffers to be provided to the Idle task are declared inside
    * this function then they must be declared static - otherwise they will
    * be allocated on the stack and so not exists after this function exits.
    */
    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

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




void BSP_init(void){
	clearLED(LED_GREEN);
	clearLED(LED_RED);
	clearLED(LED_BLUE);
	BSP_randomSeed(1234U);

    /* initialize the QS software tracing... */
    if (QS_INIT((void *)0) == 0U) {
        Q_ERROR();
    }
    QS_OBJ_DICTIONARY(&l_TickHook);
    QS_OBJ_DICTIONARY(&l_GPIOPortA_IRQHandler);
    QS_USR_DICTIONARY(PHILO_STAT);
    QS_USR_DICTIONARY(PAUSED_STAT);
    QS_USR_DICTIONARY(COMMAND_STAT);

    /* setup the QS filters... */
    QS_GLB_FILTER(QS_ALL_RECORDS); /* all records */
    QS_GLB_FILTER(-QS_QF_TICK);    /* exclude the clock tick */

}
void BSP_displayPaused(uint8_t paused){
//    GPIOF_AHB->DATA_Bits[LED_BLUE] = ((paused != 0U) ? LED_BLUE : 0U);
    setLED(((paused != 0U) ? LED_BLUE : 0U));

    QS_BEGIN_ID(PAUSED_STAT, 0U) /* app-specific record */
        QS_U8(1, paused);  /* Paused status */
    QS_END()
}
void BSP_displayPhilStat(uint8_t n, char const *stat){
//    GPIOF_AHB->DATA_Bits[LED_GREEN] = ((stat[0] == 'e') ? LED_GREEN : 0U);
    setLED(((stat[0] == 'e') ? LED_GREEN : 0U));

    QS_BEGIN_ID(PHILO_STAT, AO_Philo[n]->prio) /* app-specific record */
        QS_U8(1, n);  /* Philosopher number */
        QS_STR(stat); /* Philosopher status */
    QS_END()
}
void BSP_terminate(int16_t result){
    (void)result;
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
    l_rnd = l_rnd * (3U*7U*11U*13U*23U);
    xTaskResumeAll(); /* unlock the FreeRTOS scheduler */

    return l_rnd >> 8;
}

void QF_onStartup(void) {

}

Q_NORETURN Q_onAssert(char const * const module, int_t const loc) {
    /*
    * NOTE: add here your application-specific error handling
    */
    (void)module;
    (void)loc;
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

    static uint8_t qsTxBuf[2*1024]; /* buffer for QS-TX channel */
    static uint8_t qsRxBuf[100];    /* buffer for QS-RX channel */

    QS_initBuf  (qsTxBuf, sizeof(qsTxBuf));
    QS_rxInitBuf(qsRxBuf, sizeof(qsRxBuf));

    return 1U; /* return success */
}
/*..........................................................................*/
void QS_onCleanup(void) {
}
/*..........................................................................*/
QSTimeCtr QS_onGetTime(void) {  /* NOTE: invoked with interrupts DISABLED */
    return  xTaskGetTickCount( ); //0;//TIMER5->TAV;
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
        }
        else {
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
void QS_onCommand(uint8_t cmdId,
                  uint32_t param1, uint32_t param2, uint32_t param3)
{
    QS_BEGIN_ID(COMMAND_STAT, 0U) /* app-specific record */
        QS_U8(2, cmdId);
        QS_U32(8, param1);
        QS_U32(8, param2);
        QS_U32(8, param3);
    QS_END()
}

#endif /* Q_SPY */
/*--------------------------------------------------------------------------*/
