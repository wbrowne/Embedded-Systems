#ifndef timerTask_H
#define timerTask_H

#include "queue.h"

void vStartTimerTask( unsigned portBASE_TYPE uxPriority, xQueueHandle xQueue1, xQueueHandle xQueue2 );

#endif
