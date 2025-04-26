#ifndef LCD_COMMANDS_H
#define LCD_COMMANDS_H

#include <msp430.h>
#include <stdbool.h>

void clearLCD(void);
void initLCD(void);
void sendCommand(unsigned char data);
void setData(unsigned char data);
void latch(void);
void goToDDRLCD(unsigned char addr);
void writeChar(unsigned char Last_char);
void writeMessage(const unsigned char* message);

#endif // LCD_SHARED_H