#ifndef LCDMESSAGE_H
#define LCDMESSAGE_H

#include <State.h>

typedef struct LCDMessage {
	State state;
	unsigned int duration;
}LCDMessage;

#endif /* LCDMESSAGE_H */

