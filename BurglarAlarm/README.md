[See PDF report for full description]

The problem is to design an embedded system to control a simple domestic burglar alarm, 
making use of the FreeRTOS operating system using the LPC2468 development kit. The burglar 
alarm system  consists of four sensors and is controlled by a touchscreen LCD display. 
Each sensor is simulated by a push button on the LPC2468 board. The LCD should display ten numerals (for user input) 
and the current alarm status (armed, entry delay, unarmed etc.). LEDs physically adjacent to the push buttons should 
be used to indicate which of the sensors have been triggered, and should only be turned off when the alarm is disarmed.

My solution to the problem is designed as a state machine. There is a single task (master/controller) whose role is to control the current state of the system and communicate with each of the peripheral devices (LCD, pushbuttons, speaker/timers).
The different states of the system are as follows:

ARMED – the system is currently active and the alarm will sound if sensors 2-4 are triggered, or if sensor 1 (door) is tripped and the alarm code isn’t entered within a certain timeframe

DISARMED – the system is currently inactive and triggering the sensors has no effect

ALARM SOUND – the alarm is currently sounding, indicating a burglary

ENTRY DELAY – the door sensor has been triggered and there is an small amount of time to enter the alarm code before the alarm is sounded

EXIT DELAY – the alarm code has been entered and the house must be exited within a certain amount of time before the system is armed

UNITIALIZED – the system has not yet been initialized and requires a code to be configured by the user
