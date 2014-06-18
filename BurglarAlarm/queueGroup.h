#ifndef QUEUEGROUP_H
#define QUEUEGROUP_H

#include "queue.h"

typedef struct QueueGroup 
{
    xQueueHandle h1;
	xQueueHandle h2;
	xQueueHandle h3;
	xQueueHandle h4;
} QueueGroup;

#endif /* QUEUEGROUP_H */
