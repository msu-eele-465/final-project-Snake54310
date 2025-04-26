#ifndef SHARED_H
#define SHARED_H

#include <msp430.h>
#include <stdbool.h>

typedef enum { LOCKED, UNLOCKING, UNLOCKED } system_states;

extern volatile unsigned int red_counter;
extern volatile unsigned int green_counter;
extern volatile unsigned int blue_counter;

extern volatile unsigned int Data_Cnt;

extern volatile unsigned int dataRead[3];
extern volatile unsigned int dataSend[2];
extern volatile bool send_next_temp;
extern volatile char next_window;
extern volatile char confirm_window;
extern volatile bool record_next_temp;
extern volatile float cur_temp; // this is what you update to thedegrees celcius
extern volatile float pelt_temp;
extern volatile unsigned int send_temp;
extern volatile unsigned int send_temp_dec;

extern volatile unsigned int ADC_Value;

extern volatile bool is_read;

extern volatile system_states state;

#endif // SHARED_H