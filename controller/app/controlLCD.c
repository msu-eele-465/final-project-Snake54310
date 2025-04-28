#include <msp430.h>
#include <stdbool.h>
#include <string.h>
#include "shared.h"
#include "controlLCD.h"

// review digital page 424, 414

void sendChar(char last_input) {
    Data_Cnt = 0;
    __delay_cycles(2000);
    UCB0I2CSA = 0x0047; // choose slave address
    UCB0TBCNT = 0x02; // always send 2 bytes
    dataSend[0] = 5; // this will select the pattern selection variable on the slave
    dataSend[1] = last_input; // send value of previous/lastest input
    UCB0CTLW0 |= UCTXSTT; // generate start condition
    return;
}
void sendPosition(char send_pos) {
    Data_Cnt = 0;
    __delay_cycles(2000);
    UCB0I2CSA = 0x0047; // choose slave address
    UCB0TBCNT = 0x02; // always send 2 bytes
    dataSend[0] = 4; // this will select the pattern selection variable on the slave
    dataSend[1] = send_pos; // send value of previous/lastest input
    UCB0CTLW0 |= UCTXSTT; // generate start condition
    return;
}
void sendMessage(int message) {
    Data_Cnt = 0;
    __delay_cycles(4000);
    UCB0I2CSA = 0x0047; // choose slave address
    UCB0TBCNT = 0x02;
    dataSend[0] = 2; // this will select the pattern selection variable on the slave
    dataSend[1] = message; // send message # to LCD
    UCB0CTLW0 |= UCTXSTT; // generate start condition
    return;

}   
void enterState(int state) { // 0 is notification send, 1 is edit state, 2 is view state
    Data_Cnt = 0;
    __delay_cycles(2000);
    UCB0I2CSA = 0x0047; // choose slave address
    UCB0TBCNT = 0x02;
    dataSend[0] = 1; // this will select the pattern selection variable on the slave
    dataSend[1] = state;
    UCB0CTLW0 |= UCTXSTT; // generate start condition
    return;
}

void init_LCD_I2C() {
    UCB0CTLW0 |= UCSWRST;
    UCB0CTLW0 |= UCSSEL__SMCLK;
    UCB0BRW = 10; // divide BRCLK by 10 for 100khz

    UCB0CTLW0 |= UCMODE_3; // i2c mode
    UCB0CTLW0 |= UCMST;   // master mode
    UCB0CTLW0 |= UCTR;    // Tx mode
    UCB0I2CSA = 0x0047; // choose slave address

    UCB0CTLW1 |= UCASTP_2; 
    UCB0TBCNT = 0x02; // send two (three inclusing slave address) bytes of data for all these 
                      // commands--one for chosen variable to alter, one for the new variable data.
    P1SEL1 &= ~BIT3;
    P1SEL0 |= BIT3;
    P1SEL1 &= ~BIT2;
    P1SEL0 |= BIT2;

    PM5CTL0 &= ~LOCKLPM5;

    UCB0CTLW0 &= ~UCSWRST;

    UCB0IE |= UCTXIE0; // enable IQR
    UCB0IE |= UCRXIE0; // the other one
    return;

}