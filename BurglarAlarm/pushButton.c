#include <stdlib.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "console.h"
#include "lpc24xx.h"
#include "masterTask.h"
#include "queueGroup.h"
#include "pushButton.h"
#include "MasterCommand.h"
#include "SensorsCommand.h"

/* I2C definitions */
#define I2C_AA 0x00000004 // I2C Assert acknowledge flag 
#define I2C_SI 0x00000008 // I2C interrupt flag 
#define I2C_STO 0x00000010 // I2C STOP flag 
#define I2C_STA 0x00000020 // I2C START flag 
#define I2C_I2EN 0x00000040 // I2C enable

/* Maximum task stack size */
#define sensorsSTACK_SIZE			( ( unsigned portBASE_TYPE ) 256 )

/* The Sensors task. */
static void vSensorsTask( void *pvParameters );

/* The LED task. */
static void vLEDTask( void *pvParameters );

unsigned int buttonStates = 0;
unsigned char ledData;

void vStartSensors( unsigned portBASE_TYPE uxPriority, xQueueHandle xQueue ) {
	static xQueueHandle xMasterQ;

	xMasterQ = xQueue;
    
	/* Spawn the console task. */
	xTaskCreate( vSensorsTask, ( signed char * ) "Sensors", sensorsSTACK_SIZE, &xMasterQ, uxPriority, ( xTaskHandle * ) NULL );
}

void vStartLEDTask( unsigned portBASE_TYPE uxPriority, xQueueHandle xQueue ) {
	static xQueueHandle  xSensorsQ;

	xSensorsQ = xQueue;

	/* Spawn the LED task. */
	xTaskCreate( vLEDTask, ( signed char * ) "LEDs", sensorsSTACK_SIZE, &xSensorsQ, uxPriority, ( xTaskHandle * ) NULL );
}

/* Get I2C button status */
unsigned char getButtons() {
	unsigned char buttonData;

	/* Initialise */
	I20CONCLR = I2C_AA | I2C_SI | I2C_STA | I2C_STO;

	/* Request send START */
	I20CONSET = I2C_STA;

	/* Wait for START to be sent */
	while (!(I20CONSET & I2C_SI));

	/* Request send PCA9532 ADDRESS and R/W bit and clear SI */
	I20DAT = 0xC0;
	I20CONCLR = I2C_SI | I2C_STA;

	/* Wait for ADDRESS and R/W to be sent */
	while (!(I20CONSET & I2C_SI));
		
	/* Send control word to read PCA9532 INPUT0 register */
	I20DAT = 0x00;
	I20CONCLR = I2C_SI;

	/* Wait for DATA with control word to be sent */
	while (!(I20CONSET & I2C_SI));

	/* Request send repeated START */
	I20CONSET = I2C_STA;
	I20CONCLR = I2C_SI;

	/* Wait for START to be sent */
	while (!(I20CONSET & I2C_SI));

	/* Request send PCA9532 ADDRESS and R/W bit and clear SI */ 
	I20DAT = 0xC1;
	I20CONCLR = I2C_SI | I2C_STA;

	/* Wait for ADDRESS and R/W to be sent */
	while (!(I20CONSET & I2C_SI));

	I20CONCLR = I2C_SI;

	/*  */
  while (!(I20CONSET & I2C_SI));
	
	buttonData = I20DAT;

	/* Request send NAQ and STOP */
	I20CONSET = I2C_STO;
	I20CONCLR = I2C_SI | I2C_AA;

	/* Wait for STOP to be sent */
	while (I20CONSET & I2C_STO);

	/* return button state, inverting the least significant 4 bits */
	return buttonData ^ 0xf;
}

void setupI2C() {
	I20CONCLR = I2C_AA | I2C_SI | I2C_STA | I2C_I2EN;
	
	I20SCLL = 0x80;
	I20SCLH = 0x80;
	
	I20CONSET = I2C_I2EN;
	
	PINSEL1 |= 1 << 22;	/* SDA0 */
	PINSEL1 &= ~(1 << 23);	/* SDA0 */
	
	PINSEL1 |= 1 << 24;	/* SCL0 */
	PINSEL1 &= ~(1 << 25);	/* SCL0 */
}

void setLEDS(unsigned char leds) {
	
	if (leds == 0){          /* Turn off all LEDs */
    	ledData=0;
    }
    if (leds & 0x01) {
    	ledData |= 0x01;
    }
    if (leds & 0x02) {
    	ledData |= 0x4;
    }
    if (leds & 0x04) {
    	ledData |= 0x10;
    }
    if (leds & 0x08) {
    	ledData |= 0x40;
    }                                  

    /* Initialise */
    I20CONCLR =  I2C_AA | I2C_SI | I2C_STA | I2C_STO;
    
    /* Request send START */
    I20CONSET =  I2C_STA;

    /* Wait for START to be sent */
    while (!(I20CONSET & I2C_SI));

    /* Request send PCA9532 ADDRESS and R/W bit and clear SI */                
    I20DAT    =  0xC0;
    I20CONCLR =  I2C_SI | I2C_STA;

    /* Wait for ADDRESS and R/W to be sent */
    while (!(I20CONSET & I2C_SI));

    /* Send control word to write PCA9532 LS2 register */
    I20DAT = 0x08;
    I20CONCLR =  I2C_SI;

    /* Wait for DATA with control word to be sent */
    while (!(I20CONSET & I2C_SI));

    /* Send data to write PCA9532 LS2 register */ 
	I20DAT = ledData;
    I20CONCLR =  I2C_SI;

    /* Wait for DATA with control word to be sent */
    while (!(I20CONSET & I2C_SI));

    /* Request send NAQ and STOP */
    I20CONSET =  I2C_STO;
    I20CONCLR =  I2C_SI;

    /* Wait for STOP to be sent */
    while (I20CONSET & I2C_STO);
}

static portTASK_FUNCTION( vLEDTask, pvParameters ) {
	xQueueHandle xSensorsQ;
	SensorsCommand messageForSensors;

	/* Just to stop compiler warnings. */
	( void ) pvParameters;
	xSensorsQ = * ( ( xQueueHandle * ) pvParameters );

	while(1) {
		xQueueReceive(xSensorsQ, &messageForSensors, portMAX_DELAY);					
		if(messageForSensors == LED_ON){
			setLEDS(buttonStates);
		}	else if(messageForSensors == LEDS_OFF) {
			setLEDS(0);
			buttonStates = 0;
			ledData = 0;			
		}
	}
}

static portTASK_FUNCTION( vSensorsTask, pvParameters ) {
	xQueueHandle xMasterQ;
	portTickType xLastWakeTime;
	unsigned char buttonState;
	unsigned char lastButtonState;
	unsigned char changedState;
	unsigned int i;
	unsigned char mask;

	/* Just to stop compiler warnings. */
	( void ) pvParameters;
	xMasterQ = * ( ( xQueueHandle * ) pvParameters );
	
	setupI2C();
	
	 /* initialise lastState with all buttons off */
	lastButtonState = 0;

	/* initial xLastWakeTime for accurate polling interval */
	xLastWakeTime = xTaskGetTickCount();
		
	while(1) {
		buttonState = getButtons();
			 
		changedState = buttonState ^ lastButtonState;

		if (buttonState != lastButtonState) {
			for (i = 0; i < 4; i++) {
				mask = 1 << i;

				if (changedState & mask) {
				
					if(buttonState & mask) {
						MasterCommand cmd;
								
						buttonStates |= buttonState;
												
						if(buttonState == 1) {
							cmd = PRIMARY_SENSOR;			
							xQueueSendToBack(xMasterQ, &cmd, portMAX_DELAY);
						} else {
							cmd = SECONDARY_SENSOR;
							xQueueSendToBack(xMasterQ, &cmd, portMAX_DELAY);
						}					
						
					} 
				}
			}
			lastButtonState = buttonState;
		}
		vTaskDelayUntil( &xLastWakeTime, 20);
	}
	
}
