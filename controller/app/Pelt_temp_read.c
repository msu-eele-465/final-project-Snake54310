#include <msp430.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "shared.h"
#include "pelt_temp_read.h"

int ave_cnt_1 = 8; // same as above
volatile float average[9]= {0};
volatile float total_1 = 0;

void get_pelt_temp(int window) { 
    Data_Cnt = 0;
    is_read = true;
    __delay_cycles(2000);
    // data_cnt = 0;
    UCB0I2CSA = 0b01001000; // choose slave address
    UCB0CTLW0 &= ~UCTR; // data read
    dataRead[0] = 0b00000000;
    UCB0TBCNT = 0x02;
   
    UCB0CTLW0 |= UCTXSTT; // generate start condition
    while((UCB0IFG & UCSTPIFG) == 0) {}
    __delay_cycles(5000);
    UCB0IFG &= ~UCSTPIFG; 

    int tmp_msb = dataRead[1];
    int tmp_lsb = dataRead[2];

    float temp_B = ((tmp_msb & ~(0b10000111)) * 0x02) + ((tmp_msb & ~(0b11111000)) * 0x02) + ((tmp_lsb & ~(0b01111111))/0x0F) + ((tmp_lsb & ~(0b10000111))/0x80);

    /* voltage = (ADC_Value*3.3)/(4095); // gets voltage value from equation
    double in = 2196200 + ((1.8639-voltage)/.00000388); // from equation
    double root = sqrt(in); */ //from equation
    // temp_B = -1481.96 + root; // combine equation for temp

   // temp b is what the peltier gives i think
    int i;
    if(ave_cnt_1 != 0){ // populates array when its empty
        ave_cnt_1--;
        average[ave_cnt_1] = temp_B;
    }
    else{ // shifts bits up to make room for new data (temps)
        for(i=8; i>0; i--){
            average[i] = average[i-1]; // populate the array if empty
        }
        average[0] = temp_B;

        for(i=0; i<window; i++){ // window size n for average temp of n
            total_1 = (total_1 + average[i]);
        }
        pelt_temp = (total_1/window); // convert to celcius average
        total_1 = 0;
    }
    UCB0TBCNT = 0x02;
    is_read = false;
    UCB0CTLW0 |= UCTR;
    return;
}