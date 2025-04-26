#include <msp430.h>
#include "intrinsics.h"
#include "lcd_shared.h"
#include "msp430fr2355.h"
#include "lcd_commands.h"
#include <string.h>

void clearLCD(void) {
    P1OUT &= ~BIT4;
    sendCommand(0x01);
    __delay_cycles(2000);
    sendCommand(0x0C); // turn cursor off
    __delay_cycles(2000);
    return;
}
void initLCD(void) {
    P1OUT &= ~BIT4;       // write control bytes
    __delay_cycles(15000);    // power on delay
    setData(0x03); // attention!
    latch();
    __delay_cycles(5000);
    latch();
    __delay_cycles(100);
    latch();
    __delay_cycles(5000);
    setData(0x2); // set 4 bit mode
    latch();
    __delay_cycles(100);
    sendCommand(0x28);   // 4 bit mode, 1/16 duty, 5x8 font
    sendCommand(0x08);   // display off
    sendCommand(0x0F);   // display on, blink curson on
    sendCommand(0x06);   // entry mode 
    return;
}
void sendCommand(unsigned char data) {
    P1OUT &= ~BIT4;     // write the command 
    setData((data >> 4));
    latch();
    setData((data & 0x0F));
    latch();
    __delay_cycles(40);
    return;
}
void setData(unsigned char data) {
    P1OUT = (P1OUT & ~(BIT0 | BIT1)) | ((data & 0x01) ? BIT0 : 0) | ((data & 0x02) ? BIT1 : 0);

    P2OUT = (P2OUT & ~(BIT6 | BIT7)) | ((data & 0x04) ? BIT6 : 0) | ((data & 0x08) ? BIT7 : 0);
    return;
}
void goToDDRLCD(unsigned char addr) {
    P1OUT &= ~BIT4;
    sendCommand(0x80+addr);
    return;
}
void writeChar(unsigned char Last_char) {
    P1OUT |= BIT4;   // write characters
    setData((Last_char >> 4));
    latch();
    setData((Last_char & 0x0F));
    latch();
    __delay_cycles(40);
    return;
}
void latch(void) {
    P1OUT |= BIT5;
    __delay_cycles(10);
    P1OUT &= ~BIT5;
    return;
}
void writeMessage(const unsigned char* message) {
    int i;
    int lengthos = strlen((const char*)message);
    for (i = 0; i < lengthos; i++) {
        writeChar(message[i]);
    }
    return;
}