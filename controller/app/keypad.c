#include <msp430.h>
#include <stdbool.h>
#include <string.h>
#include "shared.h"
#include "RGB.h"
#include "keypad.h"
char readInput(void) {
    int rows = 0b00000000;
    int cols = 0b00000000;
    int row  = 0;
    int col = 0;
    int stored_cols;
    char pressed_character;
    while (rows == 0b00000000) { // while waiting for one of P3.4-P3.7 to be pulled high
        rows = P3IN;
        rows &= 0b11110000; // clear any values on lower 4 bits
    }
    PM5CTL0 |= LOCKLPM5;  // set I/O to determine column
    P3DIR &= ~0b00001111; // set kepypad columns to inputs pulled low
    P3OUT &= ~0b00001111;

    P3DIR |= 0b11110000;   // set keypad rows to outputs pulled high
    P3OUT |= 0b11110000;
    PM5CTL0 &= ~LOCKLPM5;

    while (rows != 0) { // find # of row we pressed
        row += 1;
        rows = rows >> 1;
    }
    cols = P3IN;
    stored_cols = cols;
    cols &= 0b00001111; // clear any values on upper 4 bits
    while (cols != 0) { // find # of column we pressed
        col += 1;
        cols = cols >> 1;
    }
    cols = P3IN; // check to see if button is still down
    while (cols == stored_cols) { // wait for button release
        cols = P3IN;
    }
    PM5CTL0 |= LOCKLPM5;  // restore IO on port 3 to default
    P3DIR |= 0b00001111;   // set keypad columns to outputs pulled high
    P3OUT |= 0b00001111;

    P3DIR &= ~0b11110000; // set all keypad rows to inputs pulled low
    P3OUT &= ~0b11110000; // pull down resistors
    PM5CTL0 &= ~LOCKLPM5;
    switch (row)
    {
        case 5: 
            switch (col)
                {
                    case 1: 
                        pressed_character = 'D';
                        break;
                    case 2: 
                        pressed_character = '#';
                        break;
                    case 3: 
                        pressed_character = '0';
                        break;
                    case 4: 
                        pressed_character = '*';
                        break;
                    
                    default: 
                        pressed_character = 'I'; // Invalid input
                        break;
                }
            break;
        case 6: 
            switch (col)
                {
                    case 1: 
                        pressed_character = 'C';
                        break;
                    case 2: 
                        pressed_character = '9';
                        break;
                    case 3: 
                        pressed_character = '8';
                        break;
                    case 4: 
                        pressed_character = '7';
                        break;
                    
                    default: 
                        pressed_character = 'I'; // Invalid input
                        break;
                }
            break;
        case 7: 
            switch (col)
                {
                    case 1: 
                        pressed_character = 'B';
                        break;
                    case 2: 
                        pressed_character = '6';
                        break;
                    case 3: 
                        pressed_character = '5';
                        break;
                    case 4: 
                        pressed_character = '4';
                        break;
                    
                    default: 
                        pressed_character = 'I'; // Invalid input
                        break;
                }
            break;
        case 8: 
            switch (col)
                {
                    case 1: 
                        pressed_character = 'A';
                        break;
                    case 2: 
                        pressed_character = '3';
                        break;
                    case 3: 
                        pressed_character = '2';
                        break;
                    case 4: 
                        pressed_character = '1';
                        break;
                    
                    default: 
                        pressed_character = 'I'; // Invalid input
                        break;
                }
            break;
        default: 
            pressed_character = 'I'; // Invalid input
    }
    return pressed_character;
}

int passkey(void) {
    // should disable all functionality besides heartbeat ISR and status LED, which must turn yellow from red
    bool locked = true;
    char triedPin[5];
    char correctPin[5] = {'5', '4', '3', '1', '\0'};
    while (locked) {
        int i;
        for (i = 0; i < 4; i++) {
            triedPin[i] = readInput();
            if (triedPin[i] == 'D') {
                TB3CCTL0 &= ~CCIE; // prevent interrput from triggering more
                TB3CCR0 = 1; // 1 for now to ensure that timer virtually starts at 0, 35000  is slightly more than 5 seconds
                return 0; // this tells us to re-lock the system and stop listening for inputs
            }
        }
        triedPin[4] = '\0';
        if (strcmp(triedPin, correctPin) == 0) {
            locked = false; // if password is correct, leave loop
        }
    }
    TB3CCTL0 &= ~CCIE; // prevent interrput from triggering more
    TB3CCR0 = 1; // 1 for now to ensure that timer virtually starts at 0, 35000  is slightly more than 5 seconds
    // enable functionality
    return 1; // unlock system, also return 1 from waitForUnlock() 
}
int waitForUnlock(void) {
    // should disable all functionality besides heartbeat ISR and status LED, which be red
    int rows = 0b00000000;
    while (rows == 0b00000000) { // while waiting for one of P3.4-P3.7 to be pulled high
        rows = P3IN;
        rows &= 0b11110000; // clear any values on lower 4 bits
    }
    char is_unlock = readInput();
    if (is_unlock == 'D') {
        state = UNLOCKING;
        update_color(state);
        int unlock = passkey();
        if (unlock == 1) {
            return 1; // enter LCD controller part of program
        }
        else {
            state = LOCKED;
            update_color(state);
            return 0; // basically system remains locked, so return to the beginning of this method
        }
    }
    else {
        state = LOCKED;
        update_color(state);
        return 0; // basically system remains locked, so return to the beginning of this method
    }

}