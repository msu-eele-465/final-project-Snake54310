

#include <msp430.h>
#include "lcd_shared.h"
#include "lcd_commands.h"
#include <stdint.h>
#include <string.h>
#include <stdint.h>

// P1.2 is data pin (I2C), P1.3 is clock

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

    /*P2OUT &= ~BIT0;                      // status LED
    P2DIR |= BIT0; */
    // status interrupt
    /*TB0CCTL0 |= CCIE;                            //CCIE enables Timer B0 interrupt
    TB0CCR0 = 32768;                            //sets Timer B0 to 1 second (32.768 kHz)
    TB0CTL |= TBSSEL_1 | ID_0 | MC__UP | TBCLR;    //ACLK, No divider, Up mode, Clear timer*/

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
        if (dataRdy == 1 || dataRdy2 == 1) { // varint: '2' writes pre-determined message to bottom, 
        // 1 enters state,
        // 4 changes cursor location, 
        // 5 writes a character,
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
            if(varint == 1) { // choose system state (mostly just controls cursor)
                // 0 clears LCD and turns off cursor (for notification or reset), 1 turns on cursor and as 
                // blinking (editing), 2 turns cursor on as non-blinking (viewing)
                switch (dataint) {
                    case 0:
                        clearLCD();
                        goToDDRLCD(0x00);
                        break;
                    case 1: 
                        sendCommand(0b00001111); 
                    case 2: 
                        sendCommand(0b00001110); 
                        break;
                    default: 
                        break;
                }
            }
            else if (varint == 2) { // dataint is pattern name integer
                clearLCD();
                goToDDRLCD(0x40);
                if (dataint == 0) {
                    writeMessage("File Created");
                }
                else if (dataint == 1) {
                    writeMessage("Size Updated");
                }
                else if (dataint == 2) {
                    writeMessage("File Updated");
                }
                else if (dataint == 3) {
                    writeMessage("Name Updated");
                }
                else if (dataint == 4) {
                    writeMessage("File Deleted");
                }
                else if (dataint == 5) {
                    writeMessage("Def Char Updated");
                }
                else if (dataint == 6) {
                    writeMessage("File Closed");
                }
            }
            else if (varint == 3) { // recieve header
                clearLCD();
                goToDDRLCD(0x00);
                if (dataint == 0) {
                    writeMessage("Choose File Name");
                }
                else if (dataint == 1) {
                    writeMessage("Select File");
                }
                else if (dataint == 2) {
                    writeMessage("Sel Default Char");
                }
                else if (dataint == 3) {
                    writeMessage("Select Size");
                }
            }
            else if (varint == 4) { // send position

                if(dataint > 15) {
                    dataint = (dataint - 16);
                    dataint |= 0b01000000;
                }
                goToDDRLCD(dataint);
            }
            else if (varint == 5) { // send character
                writeChar(dataint);
            }
            else {
                // do something or nothing in case of invalid send
            }
        }
    }
}

#pragma vector = EUSCI_B0_VECTOR
__interrupt void I2C_ISR(void) {
    if (UCB0IFG & UCRXIFG0) {
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
    }
}