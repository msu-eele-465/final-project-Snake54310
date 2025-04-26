#ifndef ADC_TEMP_H
#define ADC_TEMP_H

#include <msp430.h>
#include <stdbool.h>

void config_ADC(void); 
void get_temp(int window);
#endif // ADC_TEMP_H