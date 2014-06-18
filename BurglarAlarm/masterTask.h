#ifndef masterTask_H
#define masterTask_H

#include "queue.h"

void vStartMasterTask( unsigned portBASE_TYPE uxPriority, xQueueHandle xQueue1, xQueueHandle xQueue2, xQueueHandle xQueue3, xQueueHandle xQueue4 );

#endif
