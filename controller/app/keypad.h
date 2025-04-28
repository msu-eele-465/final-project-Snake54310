#ifndef KEYPAD_H
#define KEYPAD_H

#include <msp430.h>
#include <stdbool.h>

char read(void);
int passkey(void);
int waitForUnlock(void);

#endif // KEYPAD_H