
#include "intrinsics.h"
#include <msp430.h>
#include <stdbool.h>
#include <string.h>

volatile unsigned int ADC_Value = 0;
volatile float voltage;
volatile int counter8;
volatile int counter9;
/*volatile bool confirm8H = true;
volatile bool confirm9H = true;
volatile bool confirm8L = true;
volatile bool confirm9L = true;*/
volatile bool listen8 = true;
volatile bool listen9 = true;
volatile bool selected_A8 = true;
volatile bool selected_A9 = false;
volatile bool complete = true;

int main(void)
{
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


        PM5CTL0 &= ~LOCKLPM5;                   // Disable the GPIO power-on default high-impedance mode
                                            // to activate previously configured port settings
        __enable_interrupt();       // enable maskable IQRs
        counter8 = 0;
        counter9 = 0;
    while(1)
    {
        complete = false;
        ADCCTL0 |= ADCENC | ADCSC;      // Enable and Start conversion
        //while((ADCIFG & ADCIFG0) == 0){}
        while(!complete){}
        if(selected_A8) { // toggle between A8 and A9 for reading
            PM5CTL0 |= LOCKLPM5; 
            ADCMCTL0 &= ~ADCINCH_8;      //clear ADC Input Channel = A8)
            ADCMCTL0 |= ADCINCH_9;      //ADC Input Channel = A9)
            PM5CTL0 &= ~LOCKLPM5; 
            selected_A8 = false;
            selected_A9 = true;
        }
        else {
            PM5CTL0 |= LOCKLPM5; 
            ADCMCTL0 &= ~ADCINCH_9;      //clear ADC Input Channel = A9)
            ADCMCTL0 |= ADCINCH_8;      //ADC Input Channel = A8)
            PM5CTL0 &= ~LOCKLPM5; 
            selected_A8 = true;
            selected_A9 = false;
        }

    }
}

//------- ADC ISR ----------------
#pragma vector = ADC_VECTOR
__interrupt void ADC_ISR(void) {
    __bic_SR_register_on_exit(LPM0_bits);   // wake CPU
    ADC_Value = ADCMEM0;
     ADCCTL0 &= ~(ADCENC | ADCSC);
    voltage = ADC_Value * 0.0032227;       // read ADC result
    __disable_interrupt();
    if(selected_A8 && listen8){
        if (voltage >= 3.1){  
            listen8 = false;
            counter8 = (counter8 + 1);
            //confirm8H = true;
                }
        else if (voltage > 0.2 && voltage < 3.1) {  
            listen8 = false;
            counter8 = (counter8 - 1);
            //confirm8L = true;
                    }
        else if (voltage < 0.10) { //  
            listen8 = true;
                    }
        else {

        }
    }
    /*else if(selected_A8 && confirm8H){
     if (voltage >= 3.1){  
            listen8 = false;
            confirm8H = false;
                }
        else if (voltage > 0.2 && voltage < 3.1) {  
            counter8 = (counter8 + 1);
            listen8 = false;
            confirm8H = false;
                    }
        else if (voltage < 0.10) { //  
            listen8 = true;
            confirm8H = false;
                    }
        else {
            confirm8H = false;
            listen8 = false;
        }
    }
    else if(selected_A8 && confirm8L){
     if (voltage >= 3.1){  
            counter8 = (counter8 - 1);
            listen8 = false;
            confirm8L = false;
                }
        else if (voltage > 0.2 && voltage < 3.1) {  
            listen8 = false;
            confirm8L = false;
                    }
        else if (voltage < 0.10) { //  
            listen8 = true;
            confirm8L = false;
                    }
        else {
            confirm8L = false;
            listen8 = false;
        }
    } */
    else if (selected_A8) {
        if (voltage < 0.10) {
            listen8 = true;
            //confirm8H = false;
            //confirm8L = false;
        }
    }
    else {} 

    if(selected_A9 && listen9){
        if (voltage >= 3.1){  
            listen9 = false;
            counter9 = (counter9 + 1);
            //confirm9H = true;
                }
        else if (voltage > 0.2 && voltage < 3.1) {  
            listen9 = false;
            counter9 = (counter9 - 1);
            //confirm9L = true;
                    }
        else if (voltage < 0.10) { //  
            listen9 = true;
                    }
        else {

        }
    }
    /*else if(selected_A9 && confirm9H){
     if (voltage >= 3.1){  
            listen9 = false;
            confirm9H = false;
                }
        else if (voltage > 0.2 && voltage < 3.1) {  
            counter9 = (counter9 + 1);
            listen9 = false;
            confirm9H = false;
                    }
        else if (voltage < 0.10) { //  
            listen9 = true;
            confirm9H = false;
                    }
        else {
            confirm9H = false;
            listen9 = false;
        }
    }
    else if(selected_A9 && confirm9L){
     if (voltage >= 3.1){  
            counter9 = (counter9 - 1);
            listen9 = false;
            confirm9L = false;
                }
        else if (voltage > 0.2 && voltage < 3.1) {  
            listen9 = false;
            confirm9L = false;
                    }
        else if (voltage < 0.10) { //  
            listen9 = true;
            confirm9L = false;
                    }
        else {
            confirm9L = false;
            listen9 = false;
        }
    } */
    else if (selected_A9) {
        if (voltage < 0.10) {
            listen9 = true;
            //confirm9H = false;
            //confirm9L = false;
        }
    }
    else {} 
    __enable_interrupt();
    complete = true;
}
//------- END ADC ISR ----------------