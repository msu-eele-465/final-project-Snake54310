
#include "intrinsics.h"
#include "msp430fr2355.h"
#include <msp430.h>
#include <stdbool.h>
#include <string.h>

// TO FIX: Occasionally, if the last byte of a character send is a 1, the next character will have bit
// 7 set as it is read by the EEPROM. I could either fix with haxx (always clear bit7 reading), 
// or, I could actually resolve the issue.
char write_en = 0b00000110; // 6
char write_status_register = 0b00000001; // 1
char packetAR[] = {0b00000011, 0, 1, 0};
char packetAW[] = {0b00000010, 0, 1};
char packetD[] = {'H', 'E', 'L', 'L', 'O', ' ', 'W', 'O', 'R', 'L', 'D', 'M', 'O', 'R', 'E', 'B', 'I', 'T', 'S'};

volatile char Data_Sel[64] = {};

volatile bool isRead = false;

volatile int position = 0;

volatile unsigned int to_send = 0;

volatile int Rx_Data[30];

int main(void)
{

    WDTCTL = WDTPW | WDTHOLD;               // Stop watchdog timer

    UCB1CTLW0 |= UCSWRST;

    UCB1CTLW0 |= UCSSEL__SMCLK;

   // UCB1BR0 = 10;
    //UCB1BR1 = 0;

    UCB1BRW = 50;

    UCB1CTLW0 |= UCCKPL;
    UCB1CTLW0 |= UCCKPH;
    UCB1CTLW0 |= UCSYNC;
    UCB1CTLW0 |= UCMST | UCMSB;

    UCB1CTLW0 |= UCMODE1;// | UCCKPL;
    UCB1CTLW0 &= ~UCMODE0;
    UCB1CTLW0 |= UCSTEM;
    
    P4SEL1 &= ~BIT5; // clock
    P4SEL0 |= BIT5;

    P4SEL1 &= ~BIT6; // SIMO
    P4SEL0 |= BIT6;

    P4SEL1 &= ~BIT7; // SOMI
    P4SEL0 |= BIT7;

    P4SEL1 &= ~BIT4; // Select STE
    P4SEL0 |= BIT4;


    PM5CTL0 &= ~LOCKLPM5;                   // Disable the GPIO power-on default high-impedance mode
                                            // to activate previously configured port settings

    UCB1CTLW0 &= ~UCSWRST;

    UCB1IFG &= ~UCTXIFG;
    UCB1IE |= UCTXIE;
    // UCB1IFG &= ~UCRXIFG;
   // UCB1IE |= UCRXIE; // enable this interrupt when you want to read.

    __bis_SR_register(GIE);


    while(1)
    {

        int i;
        __delay_cycles(50000);             // Delay for 100000*(1/MCLK)=0.05s
        position = 0;
        to_send = 1;
        while (UCB1STATW & UCBUSY);
        Data_Sel[0] = write_en;
        UCB1TXBUF = Data_Sel[0];
        __delay_cycles(50000);   

        position = 0;
        to_send = 2;
        while (UCB1STATW & UCBUSY);
        Data_Sel[0] = write_status_register;
        Data_Sel[1] = 0b00000000; // BIT3 and Bit2 being 0 means no protected data,
        // Bit0 is Write-in-Progress bit, prevents writes if write in progress (is read only), 
        // and bit1 being 1 is write enable latch (read only, controlled by instruction)
        // Bit7 is the write protect enable, 0 means write protect is disabled
        UCB1TXBUF = Data_Sel[0];

        __delay_cycles(50000);             // Delay for 100000*(1/MCLK)=0.05s
        position = 0;
        to_send = 1;
        while (UCB1STATW & UCBUSY);
        Data_Sel[0] = write_en;
        UCB1TXBUF = Data_Sel[0];
        __delay_cycles(50000);             // Delay for 100000*(1/MCLK)=0.05s
        position = 0;
        to_send = sizeof(packetAW) + sizeof(packetD);
        for (i = 0; i < sizeof(packetAW); i++) {
            Data_Sel[i] = packetAW[i];
        }
        for (; i < to_send; i++) {
            Data_Sel[i] = packetD[i - sizeof(packetAW)];
        }
        while (UCB1STATW & UCBUSY);
        UCB1TXBUF = Data_Sel[position];

        __delay_cycles(50000);             // Delay for 100000*(1/MCLK)=0.05s

        UCB1IFG &= ~UCRXIFG;
        UCB1IE |= UCRXIE; // enable this interrupt when you want to read.
        UCB1IE &= ~UCTXIE; // we want read interrupt, not write
        position = 0;
        to_send = 2; // read from status register
        Data_Sel[0] = 0b000000101;
        Data_Sel[1] = 0xFF;
        //while (UCB1STATW & UCBUSY);
        UCB1TXBUF = Data_Sel[position];
        __delay_cycles(50000);
        
        position = 0;
        to_send = sizeof(packetAR) + 11;
        for (i = 0; i < to_send - 11; i++) {
            Data_Sel[i] = packetAR[i];
        }
        for (; i < to_send; i++) { //i = (to_send - 11)
            Data_Sel[i] = 0xFF;
        }
        //while (UCB1STATW & UCBUSY);
        UCB1TXBUF = Data_Sel[position];
        __delay_cycles(50000);
        UCB1IE &= ~UCRXIE; 
        UCB1IFG &= ~UCTXIFG;
        UCB1IE |= UCTXIE;// enable this interrupt when you want to write.
    }
}


#pragma vector = EUSCI_B1_VECTOR
__interrupt void ISR_EUSCI_B1(void) { 
    switch(UCB1IV) {
    case 2:
        {   
            position++;
            if (position >= sizeof(packetAR) && position < to_send) {
                UCB1TXBUF = Data_Sel[position];
                Rx_Data[position - sizeof(packetAR)] = UCB1RXBUF; // Store actual data
            } 
            else if (position < sizeof(packetAR) && position < to_send) {
                UCB1TXBUF = Data_Sel[position];
                volatile char junk = UCB1RXBUF;     // Discard dummy RX
            }
            else {}
        }
        break;
    case 4:
         {
            //volatile char junk = UCB1RXBUF;
            position++;
            if (position < to_send) {
                UCB1TXBUF = Data_Sel[position];
            }
            else {
                UCB1IFG &= UCTXIFG;
            }
        }
        break;
    default:
        break;
    }
}