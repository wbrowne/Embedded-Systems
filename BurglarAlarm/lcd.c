/* 
	Sample task that initialises the EA QVGA LCD display
	with touch screen controller and processes touch screen
	interrupt events.

	Jonathan Dukes (jdukes@scss.tcd.ie)
*/

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "lcd.h"
#include "lcd_hw.h"
#include "lcd_grph.h"
#include "MasterCommand.h"
#include "queueGroup.h"
#include "LCDMessage.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* Maximum task stack size */
#define lcdSTACK_SIZE			( ( unsigned portBASE_TYPE ) 256 )

/* Interrupt handlers */
extern void vLCD_ISREntry( void );
void vLCD_ISRHandler( void );

/* The LCD task. */
static void vLcdTask( void *pvParameters );

/* LCD state task. */
static void vLcdStateTask( void *pvParameters );

/* Semaphore for ISR/task synchronisation */
xSemaphoreHandle xLcdSemphr;

/* LCD variables */
const int ROWS = 4;
const int COLS = 3;
const int CODE_LENGTH = 4;
const unsigned char * CODE[CODE_LENGTH] = {"", "", "", ""};
unsigned char * keys[12] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "<", "0", ">"};
unsigned char * codeInputBuffer[4] = {"", "", "", ""};
unsigned char * WHITESPACE = "                                         ";
int xOffset = 0;
int codeIndex = 0;
int codeSet = 0;

void vStartLcd( unsigned portBASE_TYPE uxPriority, xQueueHandle xQueue ) {
	static xQueueHandle xMasterQ;

	xMasterQ = xQueue;
	
	vSemaphoreCreateBinary(xLcdSemphr);
    
	/* Spawn the LCD task. */
	xTaskCreate( vLcdTask, ( signed char * ) "Lcd", lcdSTACK_SIZE, &xMasterQ, uxPriority, ( xTaskHandle * ) NULL );
}

void vStartLcdStateTask( unsigned portBASE_TYPE uxPriority, xQueueHandle xQueue) {
	static xQueueHandle xLcdQ;

	xLcdQ = xQueue;
    
	/* Spawn the LCD State task. */
	xTaskCreate( vLcdStateTask, ( signed char * ) "Lcd State", lcdSTACK_SIZE, &xLcdQ, uxPriority, ( xTaskHandle * ) NULL );
}

int keypadInput(unsigned char * key) {
	
	/* Back key pressed */
	if((strcmp((char*) key, "<") == 0)) {
		if(codeIndex > 0) {
			xOffset -= 6;
			codeIndex--;	
			lcd_putString(30 + xOffset, 10, "_");	
		}
	} else if(strcmp((char*) key, ">") == 0) { /* Submit key pressed */
		if(codeIndex == 4) { /* If code is fully typed */
			return 1;
		}
	} else {/* Check if is digit and input is not full */ 		
		if(codeIndex < 4) {
			/*  Add to buffer and print to screen*/
			codeInputBuffer[codeIndex] = key;
			lcd_putString(30 + xOffset, 10, key);
			codeIndex++;
			xOffset += 6;
		}
	}
	return 0;
}

/* Function used to register user keypad press and determine
	what keypad digit is pressed */
int getKeypadPress(unsigned int* xPos, unsigned int* yPos) {
	int col = floor(*xPos / 70);
	int row = floor((*yPos-40) / 65);

	if(*yPos > 40 && col < 3 && row < 4) {			
		return (row * (ROWS-1)) + col;
	} else {
		return -1;
	}		
}

void clearState() {
	lcd_putString(0, 30, WHITESPACE); /* Empty space to remove previous state display */
}

void displayBadCodeMessage() {
	lcd_putString(0,30, "The code you have entered is incorrect");
}


/* Function used to display current system state on screen */
void displayState(LCDMessage m) {
	char countdown[15];

	if(m.state != UNITIALIZED) {
		clearState(); /* Empty space to remove previous state display */
		lcd_putString(0, 30, stringFromState(m.state));
	}

	if(m.state == ENTRY_DELAY || m.state == EXIT_DELAY) {
		sprintf(countdown, "%d", m.duration);
		lcd_putString(75, 30, (unsigned char *) countdown);
	} 
}

/* Function used to display feedback to user */
void displayEnterCodeMessage() {
	lcd_putString(0,30, "Please enter a new code for your alarm");
}

/* Function used to draw the keypad */
void drawKeypad() {	
	int x1, y1, x2, y2;
	int i, j;
	int number = 0;
	
	lcd_fillScreen(BLACK);
	
	lcd_putString(0, 10, "Code: ");
	lcd_putString(30, 10, "____  ");
	
	/* Draw button grid	*/
	for(i=0;i<ROWS;i++) {
		x1 = 4;
		x2 = 74;															  
		
		y1 = 40 + (i*70);
		y2 = 105 + (i*70);
		
		for(j=0;j<COLS;j++) {
			lcd_fillRect(x1, y1, x2, y2, RED);			
			lcd_putString(x1 + 33, y1 + 32, keys[number]);
				
			x1 += 80;				
			x2 += 80;
			number++;
		}		
	}
}

/* Function to check if the inputted code is correct */ 
int codeCorrect() {
	int i;
	for(i=0;i<CODE_LENGTH;i++) {
		if(codeInputBuffer[i] != CODE[i]) {
			return 0;	
		}
	}
	return 1;
}

/* Function to clear code input buffer */
void clearInputBuffer() {
	int i;
	
	for(i=0;i<CODE_LENGTH;i++) {
		codeInputBuffer[i] = " ";
	}
	
	codeIndex = 0;
	xOffset = 0;
	lcd_putString(30, 10, "____");
}

/* Function to set alarm code */
void setCode() {
	int i;
	
	for(i=0;i<CODE_LENGTH;i++) {
		CODE[i] = codeInputBuffer[i];
	}
}

static portTASK_FUNCTION( vLcdTask, pvParameters ) {
	unsigned int pressure, xPos, yPos;
	int touchResult, keyIndex;
	portTickType xLastWakeTime;
	xQueueHandle xMasterQ;
	
  xMasterQ = * ( ( xQueueHandle * ) pvParameters );

	/* Just to stop compiler warnings. */
	(void) pvParameters;

	/* Initialise LCD display */
	lcd_init();
	
	drawKeypad();
	
	displayEnterCodeMessage();

	/* Infinite loop blocks waiting for a touch screen interrupt event from
	 * the queue. */
	while(1) {
		/* Clear TS interrupts (EINT3) */
		/* Reset and (re-)enable TS interrupts on EINT3 */
		EXTINT = 8;						/* Reset EINT3 */

		/* Enable TS interrupt vector (VIC) (vector 17) */
		VICIntEnable = 1 << 17;			/* Enable interrupts on vector 17 */

		/* Block on a quete waiting for an event from the TS interrupt handler */
		xSemaphoreTake(xLcdSemphr, portMAX_DELAY);
				
		/* Disable TS interrupt vector (VIC) (vector 17) */
		VICIntEnClr = 1 << 17;

		/* Measure next sleep interval from this point */
		xLastWakeTime = xTaskGetTickCount();

		/* Start polling the touchscreen pressure and position ( getTouch(...) ) */
		/* Keep polling until pressure == 0 */
		getTouch(&xPos, &yPos, &pressure);

		while (pressure > 0) {
			/* Get current pressure */
			getTouch(&xPos, &yPos, &pressure);
				
			touchResult = getKeypadPress(&xPos, &yPos);

			/* Prevent from getting weird touch reading */
			if(touchResult != -1) {
				keyIndex = touchResult;
			}

			/* Delay to give us a 25ms periodic TS pressure sample */
			vTaskDelayUntil( &xLastWakeTime, 25 );
		}		

		/* +++ This point in the code can be interpreted as a screen button release event +++ */
		if(keyIndex >= 0 && keyIndex < 12) {	
			int submitCode = keypadInput(keys[keyIndex]);/* Put key on LCD and decide if submit has been pressed */
	
			if(submitCode) {/* If submit is pressed */
				if(codeSet == 0) {/* If user is setting code for the first time is pressed */
					MasterCommand messageToMaster;
					setCode();				
					clearState();

					/* Tell master controller that a code has been set */
					messageToMaster = INITIALIZE;
					xQueueSendToBack(xMasterQ, &messageToMaster, portMAX_DELAY);
					
					codeSet = 1;				
				}else if(codeCorrect()) {
					MasterCommand messageToMaster;
					
					/* Tell master controller that a correct code has been registered */
					messageToMaster = CODE_SUCCESS;
					xQueueSendToBack(xMasterQ, &messageToMaster, portMAX_DELAY);
				} else {
					displayBadCodeMessage();
				}
				clearInputBuffer();/* Clear keypad code buffer */
			} 
		}	
	}
}

static portTASK_FUNCTION( vLcdStateTask, pvParameters ) {
	xQueueHandle xLcdQ;
	LCDMessage message;	
  xLcdQ = * ( ( xQueueHandle * ) pvParameters );
	
	while(1) {
		xQueueReceive(xLcdQ, &message, portMAX_DELAY);
		displayState(message);
	}
}

void vLCD_ISRHandler(void) {
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

	/* Process the touchscreen interrupt */
	/* We would want to indicate to the task above that an event has occurred */
	xSemaphoreGiveFromISR(xLcdSemphr, &xHigherPriorityTaskWoken);

	EXTINT = 8;					/* Reset EINT3 */
	VICVectAddr = 0;			/* Clear VIC interrupt */

	/* Exit the ISR.  If a task was woken by either a character being received
	or transmitted then a context switch will occur. */
	portEXIT_SWITCHING_ISR( xHigherPriorityTaskWoken );
}
