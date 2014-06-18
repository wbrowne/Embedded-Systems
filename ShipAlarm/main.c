#include <stdlib.h>
#include <LPC24xx.h>

//State
unsigned int note = 0x3FF;
unsigned int playing = 0;
unsigned int step = 1;
unsigned int potentiometerValue = 0x0;

//External assembly handler functions
extern void buttonHandlerARM(void);
extern void alarmSoundHandlerARM(void);
extern void buzzHandlerARM(void);
extern void soundAlarmARM(void);

// Initializer function for all state variables
void init() {
	note = 0x3FF;
	step = 1;
	playing = 0;
	potentiometerValue = 0x0;
}

// Interrupt handler for Push Button interrupts
void button_handler(void) {

	if(!EXTPOLAR) {			// If falling edge - button down
		T0TCR = 0x1;		// Start TIMER0 
	} else {	   
		T0TCR = 0x2;	   	// Stop and reset TIMER0 (maybe overkill since it will stop and reset anyway on next 5s) 
	}

	EXTPOLAR = ~EXTPOLAR;	// Invert edge mode for EINT0 
}

// Interrupt handler for alarm loop
void alarm_sound_handler(void) {
	
	if(T2TCR != 0x1) {		//If alarm is not already sounding (in alarm loop)
		T2TCR = 0x1;		// Start TIMER2  
	} else {	
		T0TCR = 0x2;		// Stop and reset TIMER0 
		T1TCR = 0x2;		// Stop and reset TIMER1 
		T2TCR = 0x2;		// Stop and reset TIMER2 
		init();				// Reinitialize all variables
	}
}

// Interrupt handler for speaker output
void buzz_handler(void) {

	if(note == 0x0) {		// If note is 0, set to 1023
	   note = 0x3FF;
	   DACR &= ~(note << 6);// Clear all bits (stop sound)
	} else {
	   DACR |= (note << 6);	// Set all bits (play)
	   note = 0x0;
	}
}

// Interrupt handler for alarm  control
void sound_alarm(void) {
	
	potentiometerValue = (AD0DR0 & 0xFFC0) >> 6;	// Read potentiometer value
	potentiometerValue = potentiometerValue * 5000;	// Add offset 

	T2MR0 = 6000000 + potentiometerValue;			// Add potentiometer reading with alarm sound duration

	step++;

	if(step == 16) { // If on last alarm sound
		step = 0;
		T2MR0 = 30000000 + potentiometerValue;
	}

	if(!playing) {
		T1TCR = 0x1;		// Start TIMER1 
	} else {
		T1TCR = 0x2;		// Stop and reset TIMER1 
	}

	playing = !playing;		// Invert state of whether note is playing or not
}

void setupPotentiometer() {
	// configure potentiometer 
	PCONP |= 1 << 12;  		// Turn on ADC power 

	AD0CR |= 1 << 21;  		// Enable the ADC 
	AD0CR |= 1;	    		// Select pin AD0.0
	AD0CR |= 1 << 16;	  	// Set burst bit

	PINSEL1 &= ~(1 << 14);	// Setup AD0[0] 
	PINSEL1 |= 1 << 14;			 	
}

void setupInitiationTimer() {
	// Configure TIMER0 to count for 5 seconds 
	T0IR = 0xFF;			// Clear any previous interrupts 
	T0TCR = 0x2;			// Stop and reset TIMER0 
	T0CTCR = 0x0;			// Timer mode 
	T0MR0 = 60000000;		// Match every 5 seconds 
	T0MCR = 0x7;			// Interrupt, reset and re-start on match 
	T0PR = 0x0;				// Prescale = 1 

	// Configure vector 4 (TIMER0) for IRQ 
	VICIntSelect &= ~(1 << 4);
    
	// Set vector priority to 8 (mid-level priority) 
  	VICVectPriority4 = 8;
    
  	// Set handler vector. This uses a "function pointer" which is just
  	// the address of the function. The function is defined above.
  	VICVectAddr4 = (unsigned long)alarmSoundHandlerARM;
    
 	VICIntEnable = 1 << 4; // Enable interrupts on vector 4 
}

void setupAlarmSoundTimer() {
	PINSEL1 &= ~(1 << 20);	// configure sound output 
	PINSEL1 |= 1 << 21;		// configure sound output 

	// Configure TIMER1 to count fast enough for middle C
	T1IR = 0xFF;			// Clear any previous interrupts 
	T1TCR = 0x2;			// Stop and reset TIMER1 
	T1CTCR = 0x0;			// Timer mode 
	T1MR0 = 45977;			// Match every ~3ms?
	T1MCR = 0x3;			// Interrupt, reset and re-start on match 
	T1PR = 0x0;				// Prescale = 1 

  	// Configure vector 5 (TIMER1) for IRQ 
  	VICIntSelect &= ~(1 << 5);
    
  	// Set vector priority to 6 
  	VICVectPriority5 = 6;
    
  	// Set handler vector. This uses a "function pointer" which is just
  	// the address of the function. The function is defined above.
  	VICVectAddr5 = (unsigned long)buzzHandlerARM;
    
  	VICIntEnable = 1 << 5; // Enable interrupts on vector 5. 
}

void setupAlarmIntervalTimer() {
	PCONP |= 1 << 22;		// Turn on TIMER2 
	
	// Configure TIMER2 to count for 0.5 seconds 
	T2IR = 0xFF;			// Clear any previous interrupts 
	T2TCR = 0x2;			// Stop and reset TIMER2 
	T2CTCR = 0x0;			// Timer mode 
	T2MR0 = 6000000;		// Match every 0.5 seconds 
	T2MCR = 0x3;			// Interrupt, reset and re-start on match 
	T2PR = 0x0;				// Prescale = 1 

  	// Configure vector 26 (TIMER2) for IRQ 
  	VICIntSelect &= ~(1 << 26);
    
  	// Set vector priority to 7
  	VICVectPriority26 = 7;
    
  	// Set handler vector. This uses a "function pointer" which is just
  	// the address of the function. The function is defined above.
  	VICVectAddr26 = (unsigned long)soundAlarmARM;
      
  	VICIntEnable = 1 << 26; // Enable interrupts on vector 26 
}

void setupButton() {
	PINSEL4 |= 1 << 20;		// Enable P2.10 for EINT0 function 
	EXTMODE |= 1;			// EINT0 edge-sensitive mode 
	EXTPOLAR &= 0;			// Rising edge mode for EINT0 
	EXTINT = 1;				// Reset EINT0 

	VICIntSelect &= ~(1 << 14);	// Configure vector 14 (EINT0) for IRQ 
	VICVectPriority14 = 15;		// Set priority 15 (lowest) for vector 14 
	VICVectAddr14 = (unsigned long)buttonHandlerARM;// Set handler vector 
	VICIntEnable |= 1 << 14;	// Enable interrupts on vector 14 
}

int main(void) {
	setupButton();
	setupInitiationTimer();
	setupAlarmSoundTimer();
	setupAlarmIntervalTimer();
	setupPotentiometer();

	while(1);					
}
