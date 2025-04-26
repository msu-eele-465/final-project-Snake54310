
#include <msp430.h>
#include <stdbool.h>
#include <string.h>
#include "controlLCD.h"
#include "keypad.h"
#include "RGB.h"
#include "msp430fr2355.h"
#include "shared.h"

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

    config_ADC();

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
        while (lastInput != 'A' && lastInput != 'B' && lastInput != 'D' && lastInput != 'C' && !(lastInput < ':')) {
            lastInput = readInput(); // stays here until user chooses a pattern, or chooses to lock the system
            //send_Latest_Input(lastInput);
        }
        int rows;
        long int phaseTime = 12500; // 1 second
        send_LED_Phase_Delay(phaseTime);
        //send_Pattern_Speed(phaseTime); 
        // code to set initial temp-sense window
        time_operating = 0;
        bool input_change = true;
        send_time_op = true;
        while (lastInput != 'D') {
            Data_Cnt = 0;     
            if (lastInput == 'A' && input_change) {
                input_change = false;
                is_matching = false;
                send_Pattern_Name(0);
                time_operating = 0;
                send_LED_Pattern(1);
                // code to set peltier to heat
                P5OUT &= ~BIT2;
                P5OUT |= BIT1;
                rows = P3IN; // constantly listen for an input
                rows &= 0b11110000; // clear any values on lower 4 bits
                if (rows != 0b00000000) {
                    lastInput = readInput();
                    //send_Latest_Input(lastInput);
                    input_change = true;
                }
            }
            else if (lastInput == 'B'  && input_change) {
                input_change = false;
                is_matching = false;
                send_Pattern_Name(1);
                time_operating = 0;
                send_LED_Pattern(2);
                // code to set peltier to cool
                P5OUT &= ~BIT1;
                P5OUT |= BIT2;
                rows = P3IN; // constantly listen for an input
                rows &= 0b11110000; // clear any values on lower 4 bits
                if (rows != 0b00000000) {
                    lastInput = readInput();
                    //send_Latest_Input(lastInput);
                    input_change = true;
                }
            }
            else if (lastInput == 'C'  && input_change) {
                input_change = false;
                send_Pattern_Name(3);
                time_operating = 0;
                send_LED_Pattern(0);
                // code to set peltier to match
                is_matching = true;
                rows = P3IN; // constantly listen for an input
                rows &= 0b11110000; // clear any values on lower 4 bits
                if (rows != 0b00000000) {
                    lastInput = readInput();
                    //send_Latest_Input(lastInput);
                    input_change = true;
                }
            }
            else if (input_change && lastInput != '*' && lastInput < ':') { 
                input_change = false;
                next_window = lastInput;
                // if valid input, record the number
                rows = P3IN; // constantly listen for an input
                rows &= 0b11110000; // clear any values on lower 4 bits
                if (rows != 0b00000000) {
                    lastInput = readInput();
                    send_Latest_Input(lastInput);
                    input_change = true;
                }
            }
            else if (input_change & lastInput == '*') {
                send_Latest_Input(next_window); // enter key pressed, process new window
                input_change = false;
                confirm_window = next_window;
                // code to update to new window on ADC scanner
                rows = P3IN; // constantly listen for an input
                rows &= 0b11110000; // clear any values on lower 4 bits
                if (rows != 0b00000000) {
                    lastInput = readInput();
                    //send_Latest_Input(lastInput);
                    input_change = true;
                }
            }
            else if (input_change) { // clear any invalid input
                input_change = false;
                rows = P3IN; // constantly listen for an input
                rows &= 0b11110000; // clear any values on lower 4 bits
                if (rows != 0b00000000) {
                    lastInput = readInput();
                    //send_Latest_Input(lastInput);
                    input_change = true;
                }
            }
            else if (record_next_temp) {
                send_next_temp = true;
                record_next_temp = false;
                get_temp(confirm_window - '0');
                //get_pelt_temp(confirm_window - '0');
                pelt_temp = 20.2;
                // code to read temperature
                // cur_temp variable gets set to read value

            }
            else if (send_next_temp) {
                send_next_temp = false;
                send_temp = cur_temp/1; // say temp is 15.3455 degrees, it becomes 1534.55, then 153
                // then, it is interpreted as 15.3 by LCD code
                A_send_Temp_LCD(send_temp%256); // will work for celcius, as 15-40.6 is an acceptable range. However,
                send_temp_dec = ((cur_temp - (send_temp%256))*10)/1;
                send_Temp_LCD_Dec(send_temp_dec); 

                send_temp = pelt_temp/1;
                send_temp_dec = ((pelt_temp - (send_temp%256))*10)/1;
                P_send_Temp_LCD(send_temp%256);
                send_Temp_LCD_Dec(send_temp_dec); 
            }
            else {
                rows = P3IN; // constantly listen for an input
                rows &= 0b11110000; // clear any values on lower 4 bits
                if (rows != 0b00000000) {
                    lastInput = readInput();
                    //send_Latest_Input(lastInput);
                    input_change = true;
                }
            }
            if (is_matching) {
                if (((cur_temp*10)/1) > ((pelt_temp*10)/1)) {
                    P5OUT &= ~BIT2;
                    P5OUT |= BIT1;
                }
                else if (((cur_temp*10)/1) < ((pelt_temp*10)/1)) {
                    P5OUT &= ~BIT1;
                    P5OUT |= BIT2;
                }
            }
        }
        send_next_temp = false;
        is_matching = false;
        send_time_op = false;
        time_operating = 0;
        next_window = '3';
        P5OUT &= ~(BIT1 | BIT2); // turn peltier off
        send_LED_Pattern(8); // turn off LED bar
        send_Blinking_toggle(0); // totally clear LCD
        send_LED_Timer_Pause(); // disable LCD-pattern-trigger timer interrupt here (system returns to locked state)
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
    record_next_temp = true;                              //toggles P1.0 LED
    if (update_time && send_time_op) {
        time_operating += 1;
        Send_Time_Operating(time_operating % 256);
        update_time = false;
    }
    else {
        update_time = true;
    }
    TB2CCTL0 &= ~CCIFG;
}
