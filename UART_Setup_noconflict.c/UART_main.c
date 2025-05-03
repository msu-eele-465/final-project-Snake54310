
#include "intrinsics.h"
#include <msp430.h>
#include <stdbool.h>
#include <string.h>

//** USE COM9 FOR UART COMMUNICATION!!!
//** BAUD rate 57.6 K!!!x
// TX is PIN 23
// RX is PIN 24
char out_string_first[] = "\n\r Motor advanced 1 step \r\n";
char out_string_last[] = "\n\r Motor reversed 1 rotation. \r\n";
char *out_string;
int bit_check;
int position;
int positionr = 0;
char Received[64] = "";
char new_string[70]; 

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;               // Stop watchdog timer
    
     //-- 2. Configure eUSCI_A1 into SW reset
        UCA1CTLW0 |= UCSSEL__SMCLK; // choose SMCLK as BRCLK

        UCA1BRW = 17;    // n=1M/57600 = 17.3611, 'mid frequency baud rate mode', prescaler UCA1BRW = 17
        UCA1MCTLW |= 0x4A00; // modulation UCA1MCTLW |= 0x4A00

        //-- 3. Configure Ports
        P4SEL1 &= ~BIT3;  // configure P4.3 to use UCA1TXD with:
        P4SEL0 |= BIT3;     // P4SEL1(3):P4SEL0(3) = 01

        P4SEL1 &= ~BIT2;  // configure P4.2 to use UCA1RXD with:
        P4SEL0 |= BIT2;     // P4SEL1(2):P4SEL0(2) = 01

    PM5CTL0 &= ~LOCKLPM5;                   // Disable the GPIO power-on default high-impedance mode
                                            // to activate previously configured port settings
    //UCA1IE |= UCTXIE; 
    UCA1CTLW0 &= ~UCSWRST;    // Take eUSCI_A1 out of SW reset with UCSWRST=0
    __enable_interrupt();
    while(1)
    {
        // send test
        out_string = out_string_last; // set string to reverse string
        position = 0;
        UCA1IE |= UCTXCPTIE;  // enables IQR
        UCA1IFG &= ~UCTXCPTIFG; // clears flag
        UCA1TXBUF = out_string[position]; // prints first character, triggering IQR
        int i;
        for(i = 0; i < 30000; i = i + 1){} // insert delay to avoid overwriting buffer before bits have completed shifting
        //  recieve test
        positionr = 0;
        position = -1;
        UCA1IE |= UCRXIE;
        while(UCA1IE & UCRXIE){} // insert delay to avoid overwriting buffer before bits have completed shifting
    }
}
//-- UART TX ISR -------------------------------
#pragma vector=EUSCI_A1_VECTOR
__interrupt void ISR_EUSCI_A1(void)
{   //if ((UCTXCPTIFG & UCA1IFG) == 0) { // does not work because UCTXCPTIFG is always has bit 3 set here
    if (position == -1) { // if position has not been set to zero, or is otherwise positive
        Received[positionr] = UCA1RXBUF;
        positionr ++;
        if(positionr > 63) {
            UCA1IE &= ~UCTXCPTIE; // clear flag
            UCA1IE &= ~UCRXIE; // disable this interrupt
        }
        else {
            // do nothing and wait for next character
        }
    }
    else { // if position has been set to 0 or is currently in use
        if (position == strlen(out_string)){ // if position now equals the string length
            UCA1IE &= ~UCTXCPTIE; // clear flag
            position = -1; // set position back to default of -1
        }
        else { // if string still has unsent characters
            position ++;
            __delay_cycles(200);
            UCA1TXBUF = out_string[position]; // send nth character of string
        }
    }
    UCA1IFG &= ~UCTXCPTIFG;
}

