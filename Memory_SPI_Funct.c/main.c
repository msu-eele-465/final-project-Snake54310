
#include "intrinsics.h"
#include "msp430fr2355.h"
#include <msp430.h>
#include <stdbool.h>
#include <string.h>
char write_en = 0b00000110; // 6
char write_status_register = 0b00000001; // 1
char packetAR[] = {0b00000011, 0, 0};
char packetAW[] = {0b00000010, 0, 0};
char packetD[] = {'H', 'E', 'L', 'L', 'O', ' ', 'W', 'O', 'R', 'L', 'D'};

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

    UCB1CTLW0 &= ~UCCKPL;
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
        Data_Sel[1] = 0x00000011; // BIT3 and Bit2 being 0 means no protected data,
        // Bit0 being 1 enables write protect (is read only, dependent upon actual pin), 
        // and bit1 being 1 does not disable writing (dependent upon whether something is already happening, read only)
        // Bit7 is the write enabele latch, but I think that gets reset after then instruction anyways
        UCB1TXBUF = Data_Sel[0];

        __delay_cycles(50000);             // Delay for 100000*(1/MCLK)=0.05s
        position = 0;
        to_send = 1;
        while (UCB1STATW & UCBUSY);
        Data_Sel[0] = write_en;
        UCB1TXBUF = Data_Sel[0];
        __delay_cycles(50000);             // Delay for 100000*(1/MCLK)=0.05s
        position = 0;
        to_send = sizeof(packetAR) + sizeof(packetD);
        for (i = 0; i < sizeof(packetAR); i++) {
            Data_Sel[i] = packetAW[i];
        }
        for (; i < to_send; i++) {
            Data_Sel[i] = packetD[i - sizeof(packetAR)];
        }
        while (UCB1STATW & UCBUSY);
        UCB1TXBUF = Data_Sel[position];

        __delay_cycles(50000);             // Delay for 100000*(1/MCLK)=0.05s
        /*position = 0;
        to_send = sizeof(packetD);
        for (i = 0; i < to_send; i++) {
            Data_Sel[i] = packetD[i];
        }
        while (UCB1STATW & UCBUSY);
        UCB1TXBUF = Data_Sel[position];
        __delay_cycles(5000);
        */
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
        to_send = sizeof(packetAW) + 11;
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
            if (position >= 3 && position < to_send) {
                UCB1TXBUF = Data_Sel[position];
                Rx_Data[position - 3] = UCB1RXBUF; // Store actual data
            } 
            else if (position < 3) {
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