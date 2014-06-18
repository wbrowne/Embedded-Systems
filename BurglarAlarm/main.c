/* Standard includes. */
#include <stdlib.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "console.h"
#include "lcd.h"
#include "lcd_hw.h"
#include "lcd_grph.h"
#include "pushButton.h"
#include "LCDMessage.h"
#include "masterTask.h"
#include "MasterCommand.h"
#include "SensorsCommand.h"
#include "AlarmCommand.h"
#include "timerTask.h"

extern void vLCD_ISREntry( void );

#define EVQ_MAX_EVENTS 10

/*
 * Configure the processor for use with the Keil demo board.  This is very
 * minimal as most of the setup is managed by the settings in the project
 * file.
 */
static void prvSetupHardware( void );

int main (void){
	xQueueHandle xLcdQ, xMasterQ, xSensorsQ, xAlarmQ;

	/* Setup the hardware for use with the Keil demo board. */
	prvSetupHardware();

	xLcdQ = xQueueCreate(EVQ_MAX_EVENTS, sizeof(LCDMessage));

	xMasterQ = xQueueCreate(EVQ_MAX_EVENTS, sizeof(MasterCommand));

	xSensorsQ = xQueueCreate(EVQ_MAX_EVENTS, sizeof(SensorsCommand));

	xAlarmQ = xQueueCreate(EVQ_MAX_EVENTS, sizeof(AlarmCommand));
	
  /* Start the console task */
	vStartConsole(1, 19200);

	/* Start the lcd state task */
	vStartLcdStateTask(1, xLcdQ);

	/* Start the lcd task */
	vStartLcd(1, xMasterQ);

	/* Start the timer task */
	vStartTimerTask(1, xMasterQ, xAlarmQ);

		/* Start the sensors task */
	vStartSensors(1, xMasterQ);

	/* Start the master task */
	vStartMasterTask(1, xMasterQ, xLcdQ, xAlarmQ, xSensorsQ);
	
	/* Start the LED task */
	vStartLEDTask(1, xSensorsQ);	
								   
	/* Start the FreeRTOS Scheduler ... after this we're pre-emptive multitasking ...
	NOTE : Tasks run in system mode and the scheduler runs in Supervisor mode.
	The processor MUST be in supervisor mode when vTaskStartScheduler is 
	called.  The demo applications included in the FreeRTOS.org download switch
	to supervisor mode prior to main being called.  If you are not using one of
	these demo application projects then ensure Supervisor mode is used here. */
	vTaskStartScheduler();

	/* Should never reach here!  If you do then there was not enough heap
	available for the idle task to be created. */
	while(1);
}

static void prvSetupHardware( void ) {
	/* Perform the hardware setup required.  This is minimal as most of the
	setup is managed by the settings in the project file. */
	
  /* Enable UART0. */
  PCONP   |= (1 << 3);                 /* Enable UART0 power */
  PINSEL0 |= 0x00000050;               /* Enable TxD0 and RxD0 */

	/* Initialise LCD hardware */
	lcd_hw_init();

	/* Setup LCD interrupts */
	PINSEL4 |= 1 << 26;				/* Enable P2.13 for EINT3 function */
	EXTMODE |= 8;					/* EINT3 edge-sensitive mode */
	EXTPOLAR &= ~0x8;				/* Falling edge mode for EINT3 */

	/* Setup VIC for LCD interrupts */
	VICIntSelect &= ~(1 << 17);		/* Configure vector 17 (EINT3) for IRQ */
	VICVectPriority17 = 15;			/* Set priority 15 (lowest) for vector 17 */
	VICVectAddr17 = (unsigned long)vLCD_ISREntry; /* Set handler vector */
}
