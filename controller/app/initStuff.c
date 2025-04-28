#include <msp430.h>
#include <stdbool.h>
#include <string.h>
#include "shared.h"
#include "initStuff.h"

char initADC() {
        WDTCTL = WDTPW | WDTHOLD;               // Stop watchdog timer
    
        //-- Configure ADC
        ADCCTL0 &= ~ADCSHT;         // Clear ADCSHT from def. of ADCSHT=01
        ADCCTL0 |= ADCSHT_2;        // Conversion Cycles = 16 (ADCSHT = 10)
        ADCCTL0 |= ADCON;           // Turn ADC ON

        ADCCTL1 |= ADCSSEL_2;       // ADC Clock Source = SMCLK
        ADCCTL1 |= ADCSHP;          // Sample signal source = sampling timer

        ADCCTL2 &= ~ADCRES;       // Clear ADCRES from def of ADCRES = 01
        ADCCTL2 |= ADCRES_1;        // Resolution = 10-bit (ADCRES=01)

        ADCMCTL0 |= ADCINCH_8;      //ADC Input Channel = A8)
        ADCIE |= ADCIE0;            // enable ADC Conv Complete IRQ
}
int initSPI() {
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

    UCB1CTLW0 &= ~UCSWRST;

    UCB1IFG &= ~UCTXIFG;
    UCB1IE |= UCTXIE;

}
int initUART() {
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

}