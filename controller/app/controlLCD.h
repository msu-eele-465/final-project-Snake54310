#ifndef CONTROL_LCD_H
#define CONTROL_LCD_H

#include <msp430.h>
#include <stdbool.h>
#include "shared.h"

void sendChar(char last_input);
void sendPosition(char send_pos);
void sendMessage(char message);
void sendHeader(char message);
void enterState(char state);

void init_LCD_I2C(void); 

#endif // CONTROL_LCD_H

// UCB0CTLW0 &= ~UCTR; 