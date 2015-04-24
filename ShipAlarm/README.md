The project specification is divided into 3 separate sections:

1. Design and implement a program to simulate a ship’s alarm system on an LPC2468 board making use of a push button that will initiate the alarm 
signal loop, sounded through the board’s speaker device. The alarm consists of 7 short blasts (0.5s) followed by a final long blast (2.5s), with 
each blast separated by silence (0.5s). The alarm signal is toggled when the push button is held for a period >= 5s.
2. Extend the functionality by allowing a potentiometer to control the duration of the blasts emitted by the speaker.
3. Modify the system to allow for re-entrant exception handlers that makes use of prioritised interrupts.

In order to provide the base functionality I made use of 4 different interrupt sources:
• Push Button (EINT0)  

• TIMER0 – 5s counter  

• TIMER1 – 0.00383141666667s counter (261.6 Hz)  

• TIMER2 – 0.5s counter  

Use of timers:
• TIMER0 was used to count for a period of 5s to determine whether or not the alarm signal should begin
• TIMER1 was used in order to provide the actual sound out for the speaker
• TIMER2 was used to control the loop of the alarm signal
