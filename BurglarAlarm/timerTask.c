#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "lpc24xx.h"
#include "queueGroup.h"
#include "MasterCommand.h"
#include "AlarmCommand.h"
#include "timers.h"

static xTimerHandle alarmTimerHandle, entryDelayTimerHandle, exitDelayTimerHandle, lcdTimerDisplayHandle;
static xQueueHandle xMasterQCopy;
unsigned int note = 0x3FF;

/* FreeRTOS definitions */
#define timerTaskSTACK_SIZE			( ( unsigned portBASE_TYPE ) 256 )

/* The master task. */
static void vTimerTask( void *pvParameters );

static void vAlarmTimerCallback(xTimerHandle);
static void vEntryDelayTimerCallback(xTimerHandle);
static void vExitDelayTimerCallback(xTimerHandle);
static void vlcdTimerDisplayCallback(xTimerHandle);

void buzz_handler(void) {
		
	if(note == 0x0) {
	   note = 0x3FF;
	   DACR &= ~(note << 6);	 /* Clear all bits */
	} else {
	   DACR |= (note << 6);		 /* Set all bits */
	   note = 0x0;
	}
}

void vStartTimerTask( unsigned portBASE_TYPE uxPriority, xQueueHandle xQueue1, xQueueHandle xQueue2 ) {
	static QueueGroup group;

	group.h1 = xQueue1;
	group.h2 = xQueue2;
	
	xMasterQCopy = group.h1;
	
	alarmTimerHandle = xTimerCreate("Alarm Timer", 2, pdTRUE,(void *) 1, vAlarmTimerCallback);
	entryDelayTimerHandle = xTimerCreate("Entry Delay Timer", 5000, pdFALSE,(void *) 1, vEntryDelayTimerCallback);
	exitDelayTimerHandle = xTimerCreate("Exit Delay Timer", 5000, pdFALSE,(void *) 1, vExitDelayTimerCallback);	
	lcdTimerDisplayHandle = xTimerCreate("LCD Display Timer", 1000, pdTRUE,(void *) 1, vlcdTimerDisplayCallback);

	/* Spawn the console task . */
	xTaskCreate( vTimerTask, ( signed char * ) "TimerTask", timerTaskSTACK_SIZE, &group, uxPriority, ( xTaskHandle * ) NULL );
}

void vAlarmTimerCallback( xTimerHandle timer ) {
	buzz_handler();
}

void vEntryDelayTimerCallback( xTimerHandle timer) {
	MasterCommand cmd = SOUND_ALARM;
	xQueueSendToBack(xMasterQCopy, &cmd, portMAX_DELAY);
}

void vExitDelayTimerCallback(xTimerHandle timer) {
	MasterCommand cmd = CODE_SUCCESS;
	xQueueSendToBack(xMasterQCopy, &cmd, portMAX_DELAY);
}

void vlcdTimerDisplayCallback(xTimerHandle timer) {
	MasterCommand cmd = LCD_TIME_UPDATE;
	xQueueSendToBack(xMasterQCopy, &cmd, portMAX_DELAY);
}

static portTASK_FUNCTION( vTimerTask, pvParameters ) {
  QueueGroup group;
	xQueueHandle xAlarmQ;
	AlarmCommand messageFromMaster;
    
  group = * ( ( QueueGroup * ) pvParameters );	
	
	xAlarmQ = group.h2;
	
	PINSEL1 &= ~(1 << 20);	 /* configure sound output */
	PINSEL1 |= 1 << 21;
	
	while(1) {
		xQueueReceive(xAlarmQ, &messageFromMaster, portMAX_DELAY);
		
		if(messageFromMaster == START_ALARM) {
			xTimerStart(alarmTimerHandle, 0);
			xTimerStop(exitDelayTimerHandle, 0);
			xTimerStop(lcdTimerDisplayHandle, 0);	
		} else if(messageFromMaster == STOP_ALARM) {
			xTimerStop(alarmTimerHandle, 0);
			xTimerStop(lcdTimerDisplayHandle, 0);
		} else if(messageFromMaster == START_ENTRY_DELAY) {
			xTimerStart(entryDelayTimerHandle, 0);
			xTimerStart(lcdTimerDisplayHandle, 0);
		}  else if(messageFromMaster == STOP_ENTRY_DELAY) {
			xTimerStop(alarmTimerHandle, 0);
			xTimerStop(entryDelayTimerHandle, 0);
			xTimerStop(exitDelayTimerHandle, 0);
			xTimerStop(lcdTimerDisplayHandle, 0);
		} else if(messageFromMaster == START_EXIT_DELAY) {
			xTimerStart(lcdTimerDisplayHandle, 0);
			xTimerStart(exitDelayTimerHandle, 0);
		} else if(messageFromMaster == STOP_EXIT_DELAY) {
			xTimerStop(alarmTimerHandle, 0);
			xTimerStop(exitDelayTimerHandle, 0);
			xTimerStop(lcdTimerDisplayHandle, 0);
		}
	}
}
