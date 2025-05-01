#ifndef INITSTUFF_H
#define INITSTUFF_H

#include <msp430.h>
#include <stdbool.h>

void initADC(void);
void initSPI(void);
void initUART(void);
void initMemStatReg(void); 

#endif // INITSTUFF_H
