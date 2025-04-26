#include <msp430.h>
#include <stdbool.h>
#include <string.h>
#include "shared.h"
#include "RGB.h"

void set_timer(void) {
    TB0CTL |= TBSSEL__SMCLK | MC__UP | TBCLR; 
    TB0CCR0 = 78; 
    TB0CCTL0 |= CCIE;
    return; 
}


void update_color(system_states new_state) { // system_states new_state) {
   system_states state = new_state;

    switch (state) {
        case LOCKED:  // Red (#c43e1d)
            red_counter = 220; // baseline 196
            green_counter = 12; // baseline 62/5
            blue_counter = 1; // baseline 29/10
            break;

        case UNLOCKING:  // Yellow (#c4921d)
            red_counter = 170; // baseline 196
            green_counter = 34; // baseline 146/5
            blue_counter = 4; // baseline 29/10
            break;

        case UNLOCKED:  // Blue (#1da2c4)
            red_counter = 29; // baseline 29
            green_counter = 25; // baseline 162/5
            blue_counter = 25; // baseline 196/10
            break;
    }
    return;
}