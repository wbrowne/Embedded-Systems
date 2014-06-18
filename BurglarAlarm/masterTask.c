#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "lpc24xx.h"
#include "queueGroup.h"
#include "LCDMessage.h"
#include "MasterCommand.h"
#include "AlarmCommand.h"
#include "SensorsCommand.h"

/* Needed for LCD countdown */
const int CODE_ENTRY_DELAY = 5;
int currentCodeEntryDelay = 0;

/* FreeRTOS definitions */
#define masterTaskSTACK_SIZE			( ( unsigned portBASE_TYPE ) 256 )

/* The master task */
static void vMasterTask( void *pvParameters );

void vStartMasterTask( unsigned portBASE_TYPE uxPriority, xQueueHandle xQueue1, xQueueHandle xQueue2, xQueueHandle xQueue3, xQueueHandle xQueue4 ) {
  static QueueGroup group;

	group.h1 = xQueue1;
	group.h2 = xQueue2;
	group.h3 = xQueue3;
	group.h4 = xQueue4;

	/* Spawn the console task . */
	xTaskCreate( vMasterTask, ( signed char * ) "Master", masterTaskSTACK_SIZE, &group, uxPriority, ( xTaskHandle * ) NULL );
}

static portTASK_FUNCTION( vMasterTask, pvParameters ) {
	xQueueHandle xMasterQ, xLcdQ, xAlarmQ, xSensorsQ;
	QueueGroup group;
	SensorsCommand messageToSensors;
	AlarmCommand messageToAlarm;
	LCDMessage messageToLCD;
	MasterCommand messageForMaster;  	
	State masterState = UNITIALIZED;
	
  group = * ( ( QueueGroup * ) pvParameters );
	xMasterQ = group.h1;
	xLcdQ = group.h2;	
	xAlarmQ = group.h3;
	xSensorsQ = group.h4;
	
	while(1) {
	  /* Get command from master Q */
	  xQueueReceive(xMasterQ, &messageForMaster, portMAX_DELAY); 	
		
		/* State machine */
		switch(masterState) {
			case ARMED:		
					if(messageForMaster == CODE_SUCCESS) {
						masterState = DISARMED;
					} else if(messageForMaster == PRIMARY_SENSOR) {
						messageToAlarm = START_ENTRY_DELAY;
						xQueueSendToBack(xAlarmQ, &messageToAlarm, portMAX_DELAY);
						
						masterState = ENTRY_DELAY;
						currentCodeEntryDelay = 0;
					} else if (messageForMaster == SECONDARY_SENSOR) {
						messageToAlarm = START_ALARM;
						xQueueSendToBack(xAlarmQ, &messageToAlarm, portMAX_DELAY);
						
						masterState = ALARM_SOUND;
					}
					
					if(masterState == ALARM_SOUND || masterState == ENTRY_DELAY) {
						messageToSensors = LED_ON;
						xQueueSendToBack(xSensorsQ, &messageToSensors, portMAX_DELAY);
					}
					break;
			case DISARMED:
					if(messageForMaster == CODE_SUCCESS) {
						messageToAlarm = START_EXIT_DELAY;
						xQueueSendToBack(xAlarmQ, &messageToAlarm, portMAX_DELAY);	
						
						masterState = EXIT_DELAY;
						currentCodeEntryDelay = 0;
					}
					
					if(messageForMaster == PRIMARY_SENSOR || messageForMaster == SECONDARY_SENSOR) {
						messageToSensors = LEDS_OFF;
						xQueueSendToBack(xSensorsQ, &messageToSensors, portMAX_DELAY);
					}
					break;
			case ALARM_SOUND:	
					if(messageForMaster == CODE_SUCCESS) {
						messageToAlarm = STOP_ALARM;
						xQueueSendToBack(xAlarmQ, &messageToAlarm, portMAX_DELAY);
						
						masterState = DISARMED;
						
						messageToSensors = LEDS_OFF;
						xQueueSendToBack(xSensorsQ, &messageToSensors, portMAX_DELAY);
					}
					
					if(messageForMaster == PRIMARY_SENSOR || messageForMaster == SECONDARY_SENSOR) {
						messageToSensors = LED_ON;
						xQueueSendToBack(xSensorsQ, &messageToSensors, portMAX_DELAY);
					}
					break;
			case ENTRY_DELAY:
					if (messageForMaster == SECONDARY_SENSOR) {
						messageToAlarm = START_ALARM;
						xQueueSendToBack(xAlarmQ, &messageToAlarm, portMAX_DELAY);

						messageToSensors = LED_ON;
						xQueueSendToBack(xSensorsQ, &messageToSensors, portMAX_DELAY);

						masterState = ALARM_SOUND;
					}

					if(messageForMaster == CODE_SUCCESS) {
						messageToAlarm = STOP_ENTRY_DELAY;
						xQueueSendToBack(xAlarmQ, &messageToAlarm, portMAX_DELAY);
						masterState = DISARMED;

						messageToSensors = LEDS_OFF;
						xQueueSendToBack(xSensorsQ, &messageToSensors, portMAX_DELAY);

						currentCodeEntryDelay = 0;
					}
					
					if(messageForMaster == LCD_TIME_UPDATE) {
						//messageToLCD.state = ENTRY_DELAY;
						currentCodeEntryDelay++;
					}
		
					if(messageForMaster == SOUND_ALARM) {
						messageToAlarm = START_ALARM;
						xQueueSendToBack(xAlarmQ, &messageToAlarm, portMAX_DELAY);
	
						masterState = ALARM_SOUND;
					}
					break;
			case EXIT_DELAY:
					if(messageForMaster == SOUND_ALARM) {
						messageToAlarm = START_ALARM;
						xQueueSendToBack(xAlarmQ, &messageToAlarm, portMAX_DELAY);
	
						masterState = ALARM_SOUND;
					}
			
					if(messageForMaster == PRIMARY_SENSOR ) {
						messageToSensors = LEDS_OFF;
						xQueueSendToBack(xSensorsQ, &messageToSensors, portMAX_DELAY);
					}

					if (messageForMaster == SECONDARY_SENSOR) {
						messageToAlarm = START_ALARM;
						xQueueSendToBack(xAlarmQ, &messageToAlarm, portMAX_DELAY);

						messageToSensors = LED_ON;
						xQueueSendToBack(xSensorsQ, &messageToSensors, portMAX_DELAY);

						masterState = ALARM_SOUND;
					}

					if(messageForMaster == CODE_SUCCESS) {
						messageToAlarm = STOP_EXIT_DELAY;
						xQueueSendToBack(xAlarmQ, &messageToAlarm, portMAX_DELAY);
						
						masterState = ARMED;

						currentCodeEntryDelay = 0;
					}
		
					if(messageForMaster == LCD_TIME_UPDATE) {
						//messageToLCD.state = EXIT_DELAY;
						currentCodeEntryDelay++;
					}
					break;
			case UNITIALIZED:
					if(messageForMaster == PRIMARY_SENSOR || messageForMaster == SECONDARY_SENSOR) {
						messageToSensors = LEDS_OFF;
						xQueueSendToBack(xSensorsQ, &messageToSensors, portMAX_DELAY);
					}

					if(messageForMaster == INITIALIZE) {
						masterState = DISARMED;
					}
		}
		
		/* Update LCD with new state */
		messageToLCD.state = masterState;

		/* If system is in a state where countdown needs to be displayed, send current countdown value */
		if(messageToLCD.state == ENTRY_DELAY || messageToLCD.state == EXIT_DELAY) {
		 	messageToLCD.duration = CODE_ENTRY_DELAY - currentCodeEntryDelay;	
		}
		
		xQueueSendToBack(xLcdQ, &messageToLCD, portMAX_DELAY);
	}

	
}
