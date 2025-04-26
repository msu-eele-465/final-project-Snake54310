#ifndef RGB_H
#define RGB_H

#include <msp430.h>
#include <stdbool.h>
#include "shared.h"

void set_timer(void);
void update_color(system_states new_state);

#endif // RGB_H