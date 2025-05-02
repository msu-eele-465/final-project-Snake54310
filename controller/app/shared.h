#ifndef SHARED_H
#define SHARED_H

#include <msp430.h>
#include <stdbool.h>

typedef enum { LOCKED, UNLOCKING, UNLOCKED } system_states;

extern volatile unsigned int red_counter;
extern volatile unsigned int green_counter;
extern volatile unsigned int blue_counter;

extern volatile unsigned int Data_Cnt;

extern volatile unsigned int dataSend[2];

extern volatile unsigned int ADC_Value;
extern char write_en; // 6
extern char write_status_register; // 1
extern char packetAR[4];
extern char packetAW[3];

extern volatile char Data_Sel[68];
extern volatile bool isRead;
extern volatile int position;
extern volatile unsigned int to_send;
extern volatile int Rx_Data[64];

extern volatile system_states state;

#endif // SHARED_H