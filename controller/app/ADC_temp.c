#include <msp430.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "shared.h"
#include "ADC_temp.h"

// cur_temp = 0;  this is what you update to the degrees celcius

float voltage;
volatile float average_1[9] = {0}; // could be changed for double digit window size
//float average[20]; // array size of 20 for the extra credit of double digit window size
double temp_C;
// volatile float cur_temp;
int ave_cnt = 8; // same as above
//int ave_cnt = [20]; // for double digit window size
volatile float total = 0;

void config_ADC() {
        P5SEL1 |= BIT0; // configure P5.0 for A8
        P5SEL0 |= BIT0;


        // __enable_interrupt();

        // ADC Config
        ADCCTL0 &= ~ADCSHT; //  Clear ADCSHT from def. of ADCSHT = 01
        ADCCTL0 |= ADCSHT_2; // 16 conversion cycles
        ADCCTL0 |= ADCON; // turn ADC on
        // ADCCTL0 |= ADCSRED_5; // ext pos ref
        ADCCTL1 |= ADCSSEL_2; // ADC Clock Source
        ADCCTL1 |= ADCSHP; // sample signal source = sampling timer
        ADCCTL2 &= ~ADCRES; // clear ADCRES from def. of  ADCRES=01
        ADCCTL2 |= ADCRES_2; // 12 bit resolution

        ADCMCTL0 |= ADCINCH_8; // ADC input Channel = A8 CHANGE THIS FOR THE A PORT WE ARE PLANNING ON GOING TO

        return;
}

void get_temp(int window) {  // cur_temp, ADC_Value
   
    //ADC_Start = 0; // resets
    ADCIE |= ADCIE0;
    ADCCTL0 |= ADCENC | ADCSC; // starts adc
    TB0CCTL0 &= ~CCIFG; // clears timer
    while((ADCIFG & ADCIFG0) == 0){} // wait for ADC to clear

    voltage = (ADC_Value*3.3)/(4095); // gets voltage value from equation
    double in = 2196200 + ((1.8639-voltage)/.00000388); // from equation
    double root = sqrt(in); //from equation
    temp_C = -1481.96 + root; // combine equation for temp
    int i = 8;
    int t = 8;
    if(ave_cnt != 0){ // populates array when its empty
        ave_cnt--;
        average_1[ave_cnt] = temp_C;
    }

    else{ // shifts bits up to make room for new data (temps)
        for(t=8; t>0; t--){
            average_1[t] = average_1[t-1]; // populate the array if empty
        }
        average_1[0] = temp_C;
        for(i=0; i<window; i++){ // window size n for average temp of n
            total = (total + average_1[i]);
        }
        cur_temp = (total/(float)window); // convert to celcius average
        total = 0;
    }
//    if (cur_temp < 15) {
//        cur_temp = 15;
//    }
    return;
}
