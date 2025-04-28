
#include <msp430.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include "controlLCD.h"
#include "keypad.h"
#include "RGB.h"
#include "msp430fr2355.h"
#include "shared.h"
#include "initStuff.h"

#define RED_LED   BIT4 
#define GREEN_LED BIT5 
#define BLUE_LED  BIT6 

volatile unsigned int ADC_Value = 0;
volatile unsigned int red_counter = 0;
volatile unsigned int green_counter = 0;
volatile unsigned int blue_counter = 0;

// need some variable to represent file name array. Indexes should match with an 'addresses start' array from memory. Name will not be more than 16 characters.
// allow user to create up to 15 files, too. Probably: volatile char[15][16] FileNames; and: volatile unsigned int[16] Addresses; 
// also need integer to track display position
// also need booleans and ints to track ADC8 and ADC9 actions (listen for next input, increment/decrement position/character/row)
// need space reserved to store active file (maybe just an array of size 2KB)
// need variable to track position in file
// need variable to track position in EEPROM
// need variable to track read value of EEPROM

//volatile float pelt_temp = 0; // whether this actually ever becomes a float remains to be seen. My gut tells me no, though.
//volatile float cur_temp = 0;  // this is what you update to the degrees celcius
volatile unsigned int send_temp = 0;
volatile unsigned int send_temp_dec = 0;

//volatile bool send_time_op = false;
//volatile bool is_read = false;
//volatile bool send_next_temp = false;
//volatile bool record_next_temp = false; // Set this varibale every .5 seconds when the system is not locked.
//volatile char next_window = '3';
//volatile char confirm_window = '3';

//volatile unsigned int dataRead[3] = {0, 0};
volatile unsigned int dataSend[2] = {69, 43};
volatile unsigned int Data_Cnt = 0;
// will also have record_next_temp. Send_next_temp only happens after record_next_temp 
// initiates and completes recording temp.
//volatile unsigned int limit_reached = 0;
volatile unsigned int pwms = 0;

volatile system_states state = LOCKED;

//volatile bool is_matching = false;
//volatile bool update_time = false;
// variables associated with below methods:
// file storage and navigation
volatile char fileNames[32][16] = {}; // 32 files, 16 characters each
volatile uint16_t fileMemoryLocations[32][16] = {}; // 32 files, up to 16 memory locations each
volatile uint16_t nextFreeMemory = 0; // this is the next free memory location to which we allocate memory. Starts at 0.
volatile uint8_t currentFileIndex = 0;
volatile char currentFile[1024] = {};


void createFile() {

}
void editFile() {
    
}
void viewFile() {
    
}
void allocateFileMemory() {

}
void editFileName() {

}
void deleteFile() {
    
}
void sendTerminal() {
    
}
void recieveTerminal() {
    
}
void changeEditChar() {
    
}



int main(void)
{
    // Stop watchdog timer

    WDTCTL = WDTPW | WDTHOLD;               // Stop watchdog timer
    P3DIR |= 0b00001111;   // set keypad columns to outputs pulled high
    P3OUT |= 0b00001111;

    //heartbeat LED
    P1DIR |= BIT0;                              // Sets P1.0 as an output
    P1OUT &= ~BIT0;                             // Initializes LED to OFF

    P3DIR &= ~0b11110000; // set all keypad rows to inputs pulled low
    P3REN |= 0b11111111; // permanently set all of port 3 to have resistors
    P3OUT &= ~0b11110000; // pull down resistors

    P1DIR |= RED_LED | GREEN_LED | BLUE_LED;
    P1OUT |= RED_LED | GREEN_LED | BLUE_LED;  // Start with all ON

    //heartbeat interrupt
    TB1CCTL0 |= CCIE;                            //CCIE enables Timer B0 interrupt
    TB1CCR0 = 32768;                            //sets Timer B0 to 1 second (32.768 kHz)
    TB1CTL |= TBSSEL_1 | ID_0 | MC__UP | TBCLR;    //ACLK, No divider, Up mode, Clear timer

    TB2CCTL0 |= CCIE;                            //CCIE enables Timer B0 interrupt
    TB2CCR0 = 16000;                            //sets Timer B0 to 1 second (32.768 kHz)
    TB2CTL |= TBSSEL_1 | ID_0 | MC__UP | TBCLR;    //ACLK, No divider, Up mode, Clear timer

    P5DIR |= BIT1 | BIT2;
    P5OUT &= ~(BIT1 | BIT2); // peltier heating/cooling default off
    // Disable the GPIO power-on default high-impedance mode to activate
    // previously configure port settings

    set_timer(); 

    initUART();
    initSPI();
    initADC();

    init_LCD_I2C(); // what it says, but this also likely works for LCD controller



    __enable_interrupt(); 
    int unlock = 0;

    while(true)
    {
        unlock = 0;
        state = LOCKED;
        update_color(state);
        while (unlock == 0) {
            unlock = waitForUnlock(); // stays here until complete passkey has been entered 
        }
        // turn status LED Blue here
        state = UNLOCKED;
        update_color(state);
        char lastInput = 'X';
        while ((lastInput != 'D' && !(lastInput < ':')) || (lastInput == '0')) {
            lastInput = readInput(); // stays here until user chooses a pattern, or chooses to lock the system
        }
        int rows;
        bool input_change = true;
        while (lastInput != 'D') {
            Data_Cnt = 0;     
            if (lastInput == '1' && input_change) { // create file
                input_change = false;
                createFile();
                rows = P3IN; // constantly listen for an input
                rows &= 0b11110000; // clear any values on lower 4 bits
                if (rows != 0b00000000) {
                    lastInput = readInput();
                    input_change = true;
                }
            }
            else if (lastInput == '2'  && input_change) { // Edit file
                input_change = false;
                editFile();
                rows = P3IN; // constantly listen for an input
                rows &= 0b11110000; // clear any values on lower 4 bits
                if (rows != 0b00000000) {
                    lastInput = readInput();
                    input_change = true;
                }
            }
            else if (lastInput == '3'  && input_change) { // view file
                input_change = false;
                viewFile();
                rows = P3IN; // constantly listen for an input
                rows &= 0b11110000; // clear any values on lower 4 bits
                if (rows != 0b00000000) {
                    lastInput = readInput();
                    input_change = true;
                }
            }
            else if (lastInput == '4'  && input_change) { // allocate memory to file
                input_change = false;
                allocateFileMemory();
                rows = P3IN; // constantly listen for an input
                rows &= 0b11110000; // clear any values on lower 4 bits
                if (rows != 0b00000000) {
                    lastInput = readInput();
                    input_change = true;
                }
            }
            else if (lastInput == '5'  && input_change) { // edit file name
                input_change = false;
                editFileName();
                rows = P3IN; // constantly listen for an input
                rows &= 0b11110000; // clear any values on lower 4 bits
                if (rows != 0b00000000) {
                    lastInput = readInput();
                     
                    input_change = true;
                }
            }
            else if (lastInput == '6'  && input_change) { // delete file
                input_change = false;
                deleteFile();
                rows = P3IN; // constantly listen for an input
                rows &= 0b11110000; // clear any values on lower 4 bits
                if (rows != 0b00000000) {
                    lastInput = readInput();
                     
                    input_change = true;
                }
            }
            else if (lastInput == '7'  && input_change) { // save files to terminal
                input_change = false;
                sendTerminal();
                rows = P3IN; // constantly listen for an input
                rows &= 0b11110000; // clear any values on lower 4 bits
                if (rows != 0b00000000) {
                    lastInput = readInput();
                     
                    input_change = true;
                }
            }
            else if (lastInput == '8'  && input_change) { // read files from terminal, pipeline them to memory
                input_change = false;
                recieveTerminal();
                rows = P3IN; // constantly listen for an input
                rows &= 0b11110000; // clear any values on lower 4 bits
                if (rows != 0b00000000) {
                    lastInput = readInput();
                     
                    input_change = true;
                }
            }
            else if (lastInput == '9'  && input_change) { // change default editing character
                input_change = false;
                changeEditChar();
                rows = P3IN; // constantly listen for an input
                rows &= 0b11110000; // clear any values on lower 4 bits
                if (rows != 0b00000000) {
                    lastInput = readInput();
                     
                    input_change = true;
                }
            }
            else {
                rows = P3IN; // constantly listen for an input
                rows &= 0b11110000; // clear any values on lower 4 bits
                if (rows != 0b00000000) {
                    lastInput = readInput();
                     
                    input_change = true;
                }
            }
        }
        enterState(0); // totally clear LCD
        //send_LED_Timer_Pause(); // disable LCD-pattern-trigger timer interrupt here (system returns to locked state)
    }
}

#pragma vector = TIMER0_B0_VECTOR
__interrupt void TimerB0_ISR(void) {

    pwms = (pwms + 1) % 256;

    // Red LED
    if (pwms == red_counter){
        P1OUT &= ~RED_LED;
    }
    if (pwms == 0) {
        P1OUT |= RED_LED;
    }

    // Green LED
    if (pwms == green_counter) {
        P1OUT &= ~GREEN_LED;
    }
    if (pwms == 0) {
        P1OUT |= GREEN_LED;
    }

    // Blue LED
    if (pwms == blue_counter) {
        P1OUT &= ~BLUE_LED;
    }
    if (pwms == 0) {
        P1OUT |= BLUE_LED;
    }
    TB0CCTL0 &= ~CCIFG;

}

#pragma vector = TIMER1_B0_VECTOR               //time B0 ISR
__interrupt void TIMERB1_ISR(void) {
    P1OUT ^= BIT0;                              //toggles P1.0 LED
    TB1CCTL0 &= ~CCIFG;
}

#pragma vector=EUSCI_B0_VECTOR
__interrupt void EUSCI_B0_I2C_ISR(void) {
    if (Data_Cnt == 0) {
        Data_Cnt = 1;
        UCB0TXBUF = dataSend[0];
        UCB0IFG |= UCTXIFG0; 
    }
    else {
        Data_Cnt = 0;
        UCB0TXBUF = dataSend[1];
        UCB0IFG &= ~UCTXIFG0; 
    }
}

#pragma vector=ADC_VECTOR
__interrupt void ADC_ISR(void){
    ADC_Value = ADCMEM0; // get ADC value
    ADCIFG |= ADCIFG0;
    ADCIE &= ~ADCIE0;
}

#pragma vector = TIMER2_B0_VECTOR               //time B0 ISR
__interrupt void TIMERB2_ISR(void) {
    /* record_next_temp = true;                              //toggles P1.0 LED
    if (update_time && send_time_op) {
        time_operating += 1;
        Send_Time_Operating(time_operating % 256);
        update_time = false;
    }
    else {
        update_time = true;
    } */
    TB2CCTL0 &= ~CCIFG;
}
