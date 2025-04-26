#ifndef CONTROL_PATTERNS_LED_H
#define CONTROL_PATTERNS_LED_H

#include <msp430.h>
#include <stdbool.h>
#include "shared.h"

void send_LED_Phase_Delay(long int timerCount);
void send_LED_Timer_Set(void);
void send_LED_Timer_Pause(void);
void send_LED_Pattern(int chosenPattern);
void init_LED_I2C(void); 

#endif // CONTROL_PATTERNS_LED_H