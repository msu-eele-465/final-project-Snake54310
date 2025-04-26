#ifndef CONTROL_LCD_H
#define CONTROL_LCD_H

#include <msp430.h>
#include <stdbool.h>
#include "shared.h"

void send_Latest_Input(char last_input);
void send_Pattern_Name(int Pattern);
void send_Blinking_toggle(int Toggle);
//void send_Pattern_Speed(long int new_speed);
void A_send_Temp_LCD(int new_temp);
void send_Temp_LCD_Dec(int new_temp);
void P_send_Temp_LCD(int new_temp);
void Send_Time_Operating(int new_time);
void init_LCD_I2C(void); 

#endif // CONTROL_LCD_H

// UCB0CTLW0 &= ~UCTR; 