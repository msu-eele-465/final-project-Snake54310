#include <msp430.h>
#include <stdbool.h>
#include <string.h>
#include "intrinsics.h"
#include "shared.h"
#include "controlPatternsLED.h"

// review digital page 424, 414

void send_LED_Phase_Delay(long int timerCount) {
    Data_Cnt = 0;
    UCB0CTLW0 |= UCTR; 
    is_read = false;
    __delay_cycles(2000);
    UCB0I2CSA = 0x0045; // choose slave address
    UCB0TBCNT = 0x02; // always send 2 bytes
    dataSend[0] = 1; // this will select the pattern selection variable on the slave
    dataSend[1] = timerCount/6250; // send divided time between phase changes to slave, to be expanded
                                   // again and set to count-up-to variable
    UCB0CTLW0 |= UCTXSTT; // generate start condition
    return;

}
void send_LED_Timer_Set(void) {
    Data_Cnt = 0;
    UCB0CTLW0 |= UCTR; 
    is_read = false;
    __delay_cycles(2000);
    UCB0I2CSA = 0x0045; // choose slave address
    UCB0TBCNT = 0x02;
    dataSend[0] = 2; // this will select the pattern selection variable on the slave
    dataSend[1] = 1; // enable LED changing timer interrupt
    UCB0CTLW0 |= UCTXSTT; // generate start condition
    return;

}   
void send_LED_Timer_Pause(void) {
    Data_Cnt = 0;
    UCB0CTLW0 |= UCTR; 
    is_read = false;
    __delay_cycles(2000);
    UCB0I2CSA = 0x0045; // choose slave address
    UCB0TBCNT = 0x02;
    dataSend[0] = 2; // this will select the pattern selection variable on the slave
    dataSend[1] = 0; // disable LED changing timer interrupt
    UCB0CTLW0 |= UCTXSTT; // generate start condition
    return;

}
void send_LED_Pattern(int chosenPattern) {
    Data_Cnt = 0;
    UCB0CTLW0 |= UCTR; 
    is_read = false;
    __delay_cycles(2000);
    UCB0I2CSA = 0x0045; // choose slave address
    UCB0TBCNT = 0x02;
    dataSend[0] = 3; // this will select the pattern selection variable on the slave
    dataSend[1] = chosenPattern; // this will send the selected pattern to the slave
    UCB0CTLW0 |= UCTXSTT; // generate start condition
    return;
}
void init_LED_I2C() {
    UCB0CTLW0 |= UCSWRST;
    UCB0CTLW0 |= UCSSEL__SMCLK;
    UCB0BRW = 10; // divide BRCLK by 10 for 100khz

    UCB0CTLW0 |= UCMODE_3; // i2c mode
    UCB0CTLW0 |= UCMST;   // master mode
    UCB0CTLW0 |= UCTR;    // Tx mode
    UCB0I2CSA = 0x0045; // choose slave address

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