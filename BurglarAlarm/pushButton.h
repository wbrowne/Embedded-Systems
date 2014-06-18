#ifndef PUSHBUTTON_H
#define PUSHBUTTON_H

#include "queue.h"

void vStartSensors( unsigned portBASE_TYPE uxPriority, xQueueHandle xQueue);

void vStartLEDTask( unsigned portBASE_TYPE uxPriority, xQueueHandle xQueue);

#endif
