#include <msp430.h>
#include <stdbool.h>
#include <string.h>
#include "shared.h"
#include "controlLCD.h"

// review digital page 424, 414

void send_Latest_Input(char last_input) {
    Data_Cnt = 0;
    UCB0CTLW0 |= UCTR; 
    is_read = false;
    __delay_cycles(2000);
    UCB0I2CSA = 0x0047; // choose slave address
    UCB0TBCNT = 0x02; // always send 2 bytes
    dataSend[0] = 1; // this will select the pattern selection variable on the slave
    //int send_value;
    /*if (last_input == '1') {
        send_value = 1;
    }
    else if (last_input == '2') {
        send_value = 2;
    }
    else if (last_input == '3') {
        send_value = 3;
    }
    else if (last_input == 'A') {
        send_value = 4;
    }
    else if (last_input == '4') {
        send_value = 5;
    }
    else if (last_input == '5') {
        send_value = 6;
    }
    else if (last_input == '6') {
        send_value = 7;
    }
    else if (last_input == 'B') {
        send_value = 8;
    }
    else if (last_input == '7') {
        send_value = 9;
    }
    else if (last_input == '8') {
        send_value = 10;
    }
    else if (last_input == '9') {
        send_value = 11;
    }
    else if (last_input == 'C') {
        send_value = 12;
    }
    else if (last_input == '*') {
        send_value = 13;
    }
    else if (last_input == '0') {
        send_value = 14;
    }
    else if (last_input == '#') {
        send_value = 15;
    }
    else if (last_input == 'D') {
        send_value = 16;
    }
    else {
        send_value = 0; // likely 'I', should never be seen
    }*/
    dataSend[1] = last_input - '0'; // send value of previous/lastest input
    UCB0CTLW0 |= UCTXSTT; // generate start condition
    return;
}
void send_Pattern_Name(int Pattern) {
    Data_Cnt = 0;
    is_read = false;
    UCB0CTLW0 |= UCTR; 
    __delay_cycles(4000);
    UCB0I2CSA = 0x0047; // choose slave address
    UCB0TBCNT = 0x02;
    dataSend[0] = 2; // this will select the pattern selection variable on the slave
    dataSend[1] = Pattern; // send pattern # to LCD
    UCB0CTLW0 |= UCTXSTT; // generate start condition
    return;

}   
void send_Blinking_toggle(int Toggle) {
    Data_Cnt = 0;
    is_read = false;
    UCB0CTLW0 |= UCTR; 
    __delay_cycles(2000);
    UCB0I2CSA = 0x0047; // choose slave address
    UCB0TBCNT = 0x02;
    dataSend[0] = 3; // this will select the pattern selection variable on the slave
    dataSend[1] = Toggle;
    UCB0CTLW0 |= UCTXSTT; // generate start condition
    return;

}
void A_send_Temp_LCD(int new_temp) {
    Data_Cnt = 0;
    is_read = false;
    UCB0CTLW0 |= UCTR; 
    __delay_cycles(4000);
    UCB0I2CSA = 0x0047; // choose slave address
    UCB0TBCNT = 0x02;
    dataSend[0] = 4; // this will select the pattern selection variable on the slave
    dataSend[1] = new_temp; // send divided time between phase changes to slave, to be expanded
                                   // again and set to count-up-to variable
    UCB0CTLW0 |= UCTXSTT; // generate start condition
    return;
}
void send_Temp_LCD_Dec(int new_temp) {
    Data_Cnt = 0;
    is_read = false;
    UCB0CTLW0 |= UCTR; 
    __delay_cycles(4000);
    UCB0I2CSA = 0x0047; // choose slave address
    UCB0TBCNT = 0x02;
    dataSend[0] = 6; // this will select the pattern selection variable on the slave
    dataSend[1] = new_temp; // send divided time between phase changes to slave, to be expanded
                                   // again and set to count-up-to variable
    UCB0CTLW0 |= UCTXSTT; // generate start condition
    return;
}
void P_send_Temp_LCD(int new_temp) {
    Data_Cnt = 0;
    is_read = false;
    __delay_cycles(4000);
    UCB0CTLW0 |= UCTR; 
    UCB0I2CSA = 0x0047; // choose slave address
    UCB0TBCNT = 0x02;
    dataSend[0] = 5; // this will select the pattern selection variable on the slave
    dataSend[1] = new_temp; // send divided time between phase changes to slave, to be expanded
                                   // again and set to count-up-to variable
    UCB0CTLW0 |= UCTXSTT; // generate start condition
    return;
}
void Send_Time_Operating(int new_time) {
    Data_Cnt = 0;
    is_read = false;
    UCB0CTLW0 |= UCTR; 
    __delay_cycles(4000);
    UCB0I2CSA = 0x0047; // choose slave address
    UCB0TBCNT = 0x02;
    dataSend[0] = 7; // this will select the pattern selection variable on the slave
    dataSend[1] = new_time; // send divided time between phase changes to slave, to be expanded
                                   // again and set to count-up-to variable
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