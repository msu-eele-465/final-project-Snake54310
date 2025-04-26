
#include <msp430.h>
#include <stdbool.h>
#include <string.h>

char out_string_first[] = "\n\r Motor advanced 1 step \r\n";
char out_string_last[] = "\n\r Motor reversed 1 rotation. \r\n";
char *out_string;
int bit_check;
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

    while(1)
    {
        out_string = out_string_last; // set string to reverse string
        position = 0;
        UCA1IE |= UCTXCPTIE;  // enables IQR
        UCA1IFG &= ~UCTXCPTIFG; // clears flag
        UCA1TXBUF = out_string[position]; // prints first character, triggering IQR
        for(i = 0; i < 30000; i = i + 1){} // insert delay to avoid overwriting buffer before bits have completed shifting
    }
}
//-- UART TX ISR -------------------------------
#pragma vector=EUSCI_A1_VECTOR
__interrupt void ISR_EUSCI_A1(void)
{   //if ((UCTXCPTIFG & UCA1IFG) == 0) { // does not work because UCTXCPTIFG is always has bit 3 set here
    if (position == -1) { // if position has not been set to zero, or is otherwise positive
        if (UCA1RXBUF == '1') // checks if input is specific character--reading input automatically clears ISR Flag
            {
            P1OUT ^= BIT0; // if input is 1, toggle red led
            }
        else if (UCA1RXBUF == '2') {
            P6OUT ^= BIT6; // if input is 2, toggle green led
        }
        UCA1IE &= ~UCTXCPTIE; // clear flag
    }
    else { // if position has been set to 0 or is currently in use
        if (position == strlen(out_string)){ // if position now equals the string length
            UCA1IE &= ~UCTXCPTIE; // clear flag
            position = -1; // set position back to default of -1
        }
        else { // if string still has unsent characters
            position ++;
            for(j = 0; j < 1000; j++);
            UCA1TXBUF = out_string[position]; // send nth character of string
        }
    }
    UCA1IFG &= ~UCTXCPTIFG;
}

