

#include <msp430.h>
#include "lcd_shared.h"
#include "lcd_commands.h"
#include <stdint.h>
#include <string.h>
#include <stdint.h>

// P1.2 is data pin (I2C), P1.3 is clock

volatile uint8_t CursorState = 0; // tracks cursor current state

volatile uint8_t index = 0;
volatile uint8_t dataRead[2] = {0, 0};
volatile uint8_t dataRead2[2] = {0, 0};
volatile uint8_t dataRdy = 0;
volatile uint8_t dataRdy2 = 0;
#define I2C_ADDRESS 0x47                //i2c address for LCD bar

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;               // Stop watchdog timer
    P2OUT &= ~0b11000000;                   // 2.7, 2.6, 1.1, 1.0 will be MSB-LSB of 4 write bits
    P2DIR |= 0b11000000;
    P1OUT &= ~0b00110011;                    // P1.5 is Enable Pin
    P1DIR |= 0b00110011;                    // P1.4 is RS pin

    P2OUT &= ~BIT0;                      // status LED
    P2DIR |= BIT0; 
    // status interrupt
    TB0CCTL0 |= CCIE;                            //CCIE enables Timer B0 interrupt
    TB0CCR0 = 32768;                            //sets Timer B0 to 1 second (32.768 kHz)
    TB0CTL |= TBSSEL_1 | ID_0 | MC__UP | TBCLR;    //ACLK, No divider, Up mode, Clear timer

    // I2C slave code
    UCB0CTLW0 = UCSWRST;                //puts eUSCI_B0 into a reset state
    UCB0CTLW0 |= UCMODE_3 | UCSYNC;     //sets up synchronous i2c mode
    UCB0I2COA0 |= I2C_ADDRESS | UCOAEN;  //sets and enables address
    P1SEL0 |= BIT2 | BIT3; // Set P1.2 and P1.3 as I2C SDA and SCL
    P1SEL1 &= ~(BIT2 | BIT3); // this is missing from Zane's code
    UCB0CTLW0 &= ~UCSWRST;              
    UCB0IE |= UCRXIE0; 
    __enable_interrupt(); 
   // P1OUT &= ~BIT0;                         // Clear P1.0 output latch for a defined power-on state
    //P1DIR |= BIT0;                          // Set P1.0 to output direction

    // Disable low-power mode / GPIO high-impedance
    PM5CTL0 &= ~LOCKLPM5;


    initLCD();
    clearLCD();

    while(1)
    {   
        if (dataRdy == 1 || dataRdy2 == 1) { // varint: '1' writes pre-determined message to top, 
        // 3 writes a file name,
        // 4 changes cursor location, 
        // 5 writes a character,
        // 6 clears display and writes temporary notification
        // make sure to use cursor states to enable/disable cursor when appropriate.
            unsigned int varint;
            unsigned int dataint;
            if (dataRdy == 1) {
                varint = dataRead[0];
                dataint = dataRead[1];
                dataRdy = 0;
            }
            else {
                varint = dataRead2[0];
                dataint = dataRead2[1];
                dataRdy2 = 0;
            }
            if(varint == 1) {
            goToDDRLCD(0x40); // go to 3rd to last character of second row
            writeChar('0' + dataint); // write our window size
            }
            else if (varint == 2) { // dataint is pattern name integer
                goToDDRLCD(0x00); // go to first character of first row
                writeMessage("                ");
                goToDDRLCD(0x00);
                if (dataint == 0) {
                    writeMessage("heat");
                }
                else if (dataint == 1) {
                    writeMessage("cool");
                }
                else if (dataint == 2) {
                    writeMessage("off");
                }
                else if (dataint == 3) {
                    writeMessage("match");
                }
                /*else if (dataint == 4) {
                    writeMessage("down counter");
                }
                else if (dataint == 5) {
                    writeMessage("rotate 1 left");
                }
                else if (dataint == 6) {
                    writeMessage("rotate 7 right");
                }
                else if (dataint == 7) {
                    writeMessage("fill left");
                }*/
                /*else if (dataint == 4) {
                    writeMessage("set");
                }*/
                /*else if (dataint == 9) {
                    writeMessage("Set Pattern");
                }*/
            }
            else if (varint == 3) { // dataint is blinking toggle state
                // if 0, turn off LCD
                // if 1, toggle cursor blinking
                // if 2, toggle cursor

                if (dataint == 0) {
                    clearLCD();
                    CursorState = 0;
                }
                else if (dataint == 1) { // C keeps display on
                    CursorState ^= 0b00000001;
                    sendCommand(0x0C | CursorState); 
                }
                else {
                    CursorState ^= 0b00000010;
                    sendCommand(0x0C | CursorState); 
                }
            }
            else if (varint == 4) { // dataint is pattern speed
                goToDDRLCD(0x08); // go to first character of first row
                writeMessage("A:");
                unsigned char tens = (dataint/10) + '0';
                int ones_int = (dataint%10);
                unsigned char ones = ones_int % 10 + '0'; 
                writeChar(tens);
                writeChar(ones);
            }
            else if (varint == 5) {
                goToDDRLCD(0x48); // go to first character of first row
                writeMessage("P:");
                unsigned char tens = (dataint/10) + '0'; 
                int ones_int = (dataint%10);
                unsigned char ones = ones_int % 10 + '0'; 
                writeChar(tens);
                writeChar(ones);
            }
            else if (varint == 6) {
                writeChar('.');
                unsigned char dec = (dataint % 10) + '0';
                writeChar(dec);
                writeChar(11011111);
                writeChar('C');
            }
            else if (varint == 7) { /// write time
                goToDDRLCD(0x42);
                unsigned char hunds = (dataint/100) + '0';
                unsigned char tens = (dataint/10)%10 + '0';
                unsigned char ones = (dataint/1)%10 + '0';
                writeChar(hunds);
                writeChar(tens);
                writeChar(ones);
                writeChar('s');
                writeChar(' ');
                writeChar(' ');
            }
            else {
                // do something or nothing in case of invalid send
            }
            TB0CCTL0 |= CCIE;
        }
    }
}

#pragma vector = EUSCI_B0_VECTOR
__interrupt void I2C_ISR(void) {
    if (UCB0IFG & UCRXIFG0) {
        TB0CCTL0 &= ~CCIE;
        if (dataRdy == 1) {
            if (index == 1) {
                dataRead2[1] = UCB0RXBUF;
                index = 0;
                dataRdy2 = 1;
            }
            else {
                dataRead2[0] = UCB0RXBUF;
                index = 1;
            }
        }
        else {
            if (index == 1) {
                dataRead[1] = UCB0RXBUF;
                index = 0;
                dataRdy = 1;
            }
            else {
                dataRead[0] = UCB0RXBUF;
                index = 1;
            }
        }
        TB0CCTL0 |= CCIE;
    }
}