#ifndef STATE_H
#define STATE_H

typedef enum {ARMED, DISARMED, ALARM_SOUND, ENTRY_DELAY, EXIT_DELAY, UNITIALIZED} State;

static inline unsigned char * stringFromState(State s) {
    static unsigned char * strings[] = { "ARMED", "DISARMED", "ALARM SOUND", "ENTRY DELAY", "EXIT DELAY" };
    return strings[s];
}

#endif /* STATE_H */
