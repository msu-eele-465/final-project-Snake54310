
#include "intrinsics.h"
#include <msp430.h>
#include <stdbool.h>
#include <string.h>

volatile unsigned int ADC_Value = 0;
volatile float voltage;
static int counter8;
static int counter9;
volatile bool CW8 = false;
volatile bool CCW8 = false;
volatile bool CW9 = false;
volatile bool CCW9 = false;
volatile bool listen8 = false;
volatile bool listen9 = false;
volatile bool selected_A8 = true;
volatile bool selected_A9 = false;
volatile bool complete = false;

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
        ADCCTL0 &= ~(ADCENC | ADCSC);
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
        if(CW8) {
            counter8 += 1;
            CW8 = false;
        }
        if(CCW8) {
            counter8 += -1;
            CCW8 = false;
        }
        if(CW9) {
            counter9 += 1;
            CW9 = false;
        }
        if(CCW9) {
            counter9 += -1;
            CCW9 = false;
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
            CW8 = true;
                }
        else if (voltage > 0.2 && voltage < 3.1) {  
            listen8 = false;
            CCW8 = true;
                    }
        else if (voltage < 0.10) { //  
            listen8 = true;
                    }
        else {

        }
    }
    else if (selected_A8) {
        if (voltage < 0.10) {
            listen8 = true;
        }
    }
    else {} 

    if(selected_A9 && listen9){
        if (voltage >= 3.1){  
            listen9 = false;
             CW9 = true;
                }
        else if (voltage > 0.2 && voltage < 3.1) {  
            listen9 = false;
             CCW9 = true;
                    }
        else if (voltage < 0.10) { //  
            listen9 = true;
                    }
        else {

        }
    }
    else if (selected_A9) {
        if (voltage < 0.10) {
            listen9 = true;
        }
    }
    else {} 
    __enable_interrupt();
    complete = true;
}
//------- END ADC ISR ----------------