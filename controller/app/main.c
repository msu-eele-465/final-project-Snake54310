
#include <locale>
#include <msp430.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include "controlLCD.h"
#include "intrinsics.h"
#include "keypad.h"
#include "RGB.h"
#include "msp430fr2355.h"
#include "shared.h"
#include "initStuff.h"

#define RED_LED   BIT4 
#define GREEN_LED BIT5 
#define BLUE_LED  BIT6 

volatile unsigned int red_counter = 0;
volatile unsigned int green_counter = 0;
volatile unsigned int blue_counter = 0;

// need some variable to represent file name array. Indexes should match with an 'addresses start' array from memory. Name will not be more than 16 characters.
// allow user to create up to 15 files, too. Probably: volatile char[15][16] FileNames; and: volatile unsigned int[16] Addresses; 
// also need integer to track display position
// also need booleans and ints to track ADC8 and ADC9 actions (listen for next input, increment/decrement position/character/row)
// need space reserved to store active file (maybe just an array of size 2KB)
// need variable to track position in file
// need variable to track position in EEPROM
// need variable to track read value of EEPROM

//volatile float pelt_temp = 0; // whether this actually ever becomes a float remains to be seen. My gut tells me no, though.
//volatile float cur_temp = 0;  // this is what you update to the degrees celcius
volatile unsigned int send_temp = 0;
volatile unsigned int send_temp_dec = 0;

//volatile bool send_time_op = false;
//volatile bool is_read = false;
//volatile bool send_next_temp = false;
//volatile bool record_next_temp = false; // Set this varibale every .5 seconds when the system is not locked.
//volatile char next_window = '3';
//volatile char confirm_window = '3';
// ----------- ADC VARIABLES ------

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

// ---------- END ADC VARIABLES -------
// -------- SPI VARIABLES --------

char write_en = 0b00000110; // 6
char write_status_register = 0b00000001; // 1
char packetAR[] = {0b00000011, 0, 0, 0};
char packetAW[] = {0b00000010, 0, 0};

volatile char Data_Sel[64] = {};
volatile bool isRead = false;
volatile int position = 0;
volatile unsigned int to_send = 0;
volatile int Rx_Data[64];
// --------  END SPI VARIABLES



//volatile unsigned int dataRead[3] = {0, 0};
volatile unsigned int dataSend[2] = {69, 43};
volatile unsigned int Data_Cnt = 0;
// will also have record_next_temp. Send_next_temp only happens after record_next_temp 
// initiates and completes recording temp.
//volatile unsigned int limit_reached = 0;
volatile unsigned int pwms = 0;

volatile system_states state = LOCKED;

//volatile bool is_matching = false;
//volatile bool update_time = false;
// variables associated with below methods:
// file storage and navigation
volatile char fileNames[32][16] = {}; // 32 files, 16 characters each
volatile uint16_t fileMemoryLocations[32][16] = {}; // 32 files, up to 16 memory locations each
volatile uint8_t fileSizes[32] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0}; 
volatile uint16_t nextFreeMemory = 0; // this is the next free memory location to which we allocate memory. Starts at 0.
volatile uint8_t currentFileIndex = 0;
volatile uint8_t nextFileIndex = 0;
volatile char currentFile[1024] = " ";
char currentPosition;
char currentChar;
char defaultChar = ' '; // will be ' ' after debugging
volatile char CurrentFileName[16] = " "; // name may be misleading--this is only used in actual file creation. Technically unecessary, but a convenient middle step
char number_of_files = 0;


void createFile() {
    fileSizes[nextFileIndex] = 2; // format new file location pointers
    fileMemoryLocations[nextFileIndex][0] = nextFreeMemory;
    nextFreeMemory = nextFreeMemory + 64;
    fileMemoryLocations[nextFileIndex][1] = nextFreeMemory;
    nextFreeMemory = nextFreeMemory + 64;
    currentFileIndex = nextFileIndex;
    nextFileIndex += 1;

    sendHeader(0); // setup LDC and file name pointer
    //while(UCB0CTLW0 & UCTXSTT) {}
    enterState(1);
    currentPosition = 16;
    currentChar = defaultChar;
    sendPosition(currentPosition);
    sendChar(currentChar);
    sendPosition(currentPosition);
    int i;
    for (i = 0; i < 16; i++) {
        CurrentFileName[i] = defaultChar; // save default file name
    }
    CurrentFileName[currentPosition - 16] = currentChar;
    char lastInput = 'I';
    int rows;
    bool input_change = false;
    while (lastInput != '*') { // create file name
        listenDials();
        if(CW8) {
            currentChar += 1;
            CurrentFileName[currentPosition - 16] = currentChar;
            sendChar(currentChar);
            sendPosition(currentPosition);
            CW8 = false;
        }
        if(CCW8) {
            currentChar += -1;
            CurrentFileName[currentPosition - 16] = currentChar;
            sendChar(currentChar);
            sendPosition(currentPosition);
            CCW8 = false;
        }
        if(CW9) {
            currentPosition += 1;
            currentPosition = (currentPosition)%16 + 16;
            sendPosition(currentPosition);
            CW9 = false;
        }
        if(CCW9) {
            currentPosition += -1;
            currentPosition = (currentPosition)%16 + 16;
            sendPosition(currentPosition);
            CCW9 = false;
        }
        rows = P3IN; // constantly listen for an input
        rows &= 0b11110000; // clear any values on lower 4 bits
        if (rows != 0b00000000) {
            lastInput = readInput();         
            input_change = true;
        }
        if (input_change) {
            currentChar = charCatSel(lastInput);
            CurrentFileName[currentPosition - 16] = currentChar;
            sendChar(currentChar);
            sendPosition(currentPosition);
            input_change = false;
        }
    }
    for (i = 0; i < 16; i++) {
        fileNames[currentFileIndex][i] = CurrentFileName[i]; // save file name
    }

    enterState(0); // flush screen
    enterState(1); // setup LDC and file pointer
    currentPosition = 0;
    currentChar = defaultChar;
    sendPosition(currentPosition);
    currentFile[currentPosition] = currentChar;
    sendChar(currentChar);
    sendPosition(currentPosition);

    int sizeOfFile = (fileSizes[currentFileIndex] * 64); // get size of new file

    for (i = 0; i < sizeOfFile; i++) {
        currentFile[i] = defaultChar; // initialize spaces
    }

    editMode();

    saveCurrentFile();// save file to memory at defined locations
    sendMessage(0); // notify user of successful file creation
    number_of_files += 1;
    return;

}
void editFile() {
    currentFileIndex = 0;
    sendHeader(1); // "select file" message
    enterState(2); // viewing mode (view file names)
    
    fileSelect();

    loadFileFromMem(); // get selected file, store in currentFile

    enterState(0); // flush screen
    enterState(1); // setup LDC and file pointer
    currentPosition = 0;
    currentChar = defaultChar;
    sendPosition(currentPosition);
    currentFile[currentPosition] = currentChar;
    sendChar(currentChar);
    sendPosition(currentPosition);

    editMode();

    saveCurrentFile();// save file to memory at defined locations
    sendMessage(2); // notify user of successful file editing
    return;
    
}
void viewFile() {
    currentFileIndex = 0;
    sendHeader(1); // "select file" message
    enterState(2); // viewing mode (view file names)

    fileSelect();

    loadFileFromMem(); // get selected file, store in currentFile

    enterState(0); // flush screen
    enterState(2); // setup LDC and file pointer
    currentPosition = 0;
    sendPosition(currentPosition);

    int sizeOfFile = (fileSizes[currentFileIndex] * 64); // get size of new file

    lastInput = 'I';
    input_change = false;
    unsigned int currentFilePos = 0;

    while (lastInput != '*') { // view file
        listenDials(); 
        if(CW8) {
            currentPosition ^= 0b00010000; // always switch the line--this is simple
            currentFilePos += 16;
            currentFilePos = currentFilePos % sizeOfFile;
            if(currentPosition < 15) { // this means bit has been un-set, so we print the next two lines
                printFromFile(currentFilePos - currentFilePos%32);
            }

            sendPosition(currentPosition);
            CW8 = false;
        }
        if(CCW8) {
            currentPosition ^= 0b00010000; // always switch the line--this is simple
            currentFilePos += -16;
            if (currentFilePos > sizeOfFile) {
                currentFilePos = sizeOfFile - 1;
            }
            if(currentPosition > 16) { // this means bit has been set, so we print the previous two lines
                printFromFile(currentFilePos - currentFilePos%32);
            }

            printFromFile(currentFilePos - currentFilePos%32);
            sendPosition(currentPosition);
            CCW8 = false;
        }
        if(CW9) {
            currentPosition += 1;
            currentFilePos += 1;
            if(currentFilePos >= sizeOfFile) {
                currentFilePos = 0;
            }
            if(currentPosition > 31) {
                currentPosition = 0;
                printFromFile(currentFilePos);
            }
            sendPosition(currentPosition);
            CW9 = false;
        }
        if(CCW9) {
            currentPosition += -1;
            currentFilePos += -1;
            if(currentFilePos >= sizeOfFile) {
                currentFilePos = (sizeOfFile - 1);
            }
            if(currentPosition > 31) {
                currentPosition = 31;
                printFromFile(currentFilePos - 31); // print from 32 characters behind current position, inclusive
            }
            sendPosition(currentPosition);
            CCW9 = false;
        }
        rows = P3IN; // constantly listen for an input
        rows &= 0b11110000; // clear any values on lower 4 bits
        if (rows != 0b00000000) {
            lastInput = readInput();         
            input_change = true;
        }
        if (input_change) {
            input_change = false;
        }
    }
    saveCurrentFile();// save file to memory at defined locations
    sendMessage(2); // notify user of successful file editing
    return;
    
}
void allocateFileMemory() {
    currentFileIndex = 0;
    sendHeader(1); // "select file" message
    enterState(2); // viewing mode (view file names)
    
    fileSelect();

    

}
void editFileName() {
    currentFileIndex = 0;
    sendHeader(1); // "select file" message
    enterState(2); // viewing mode (view file names)
    
    fileSelect();


}
void deleteFile() {
    currentFileIndex = 0;
    sendHeader(1); // "select file" message
    enterState(2); // viewing mode (view file names)
    
    fileSelect();


}
void sendTerminal() {
    
}
void recieveTerminal() {
    
}
void changeEditChar() {
    
}
void listenDials() {
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
    return;
}
char charCatSel(char Choice) {
    char currentChar;
    switch (Choice) {
        case 'A':
            currentChar = 'A';
            break;
        case 'B':
            currentChar = 'a';
            break;
        case 'C':
            currentChar = '0';
            break;
        case '#':
            currentChar = ' ';
            break;
        default: 
            break;
    }
    return currentChar;
}
void printFromFile(int Position) {
    int i;
    sendPosition(0);
    for (i = Position; i < (Position + 32); i++) {
        if(i == Position + 16) {
            sendPosition(16);
        }
        sendChar(currentFile[i]);
    }
    return;
}
void saveCurrentFile() { 
    P2OUT |= BIT1; // LED to tell user program is writing/reading
    isRead = false;
    int sizeOfCurrent = fileSizes[currentFileIndex];
    int n;
    int positionFile;
    __delay_cycles(5000);
    for (n = 0; n < sizeOfCurrent; n++) {
        position = 0;
        to_send = 1;
        while (UCB1STATW & UCBUSY);
        Data_Sel[0] = write_en;
        UCB1TXBUF = Data_Sel[0]; // set write latch enable
        __delay_cycles(5000);

        position = 0;
        packetAW[1] = (((fileMemoryLocations[currentFileIndex][n]) & ~(0x00FF))/0xFF); // 8 MSB of memory address
        packetAW[2] = ((fileMemoryLocations[currentFileIndex][n]) & ~(0xFF00)); // 8 LSB of memory address

        to_send = sizeof(packetAW) + 64; // always write full pages
        int i;
        for (i = 0; i < sizeof(packetAW); i++) {
            Data_Sel[i] = packetAW[i];
        }
        for (; i < to_send; i++) {
            Data_Sel[i] = currentFile[positionFile];
            positionFile++;
        }
        while (UCB1STATW & UCBUSY);
        UCB1TXBUF = Data_Sel[position]; // send 64 byte file component
        __delay_cycles(5000);
    }

    P2OUT &= ~BIT1;

}

void displayCurrentFileName() {
    sendPosition(16);
    int i;
    for (i = 0; i < 16; i++) {
        sendChar(fileNames[currentFileIndex][i]);
    }
}

void loadFileFromMem() {
    P2OUT |= BIT1; // LED to tell user program is writing/reading
    UCB1IFG &= ~UCRXIFG;
    UCB1IE |= UCRXIE; // enable this interrupt when you want to read.
    UCB1IE &= ~UCTXIE; // we want read interrupt, not write
    isRead = true;
    int sizeOfCurrent = fileSizes[currentFileIndex];
    int n;
    __delay_cycles(5000);
    for (n = 0; n < sizeOfCurrent; n++) {

        position = 0;
        packetAR[1] = (((fileMemoryLocations[currentFileIndex][n]) & ~(0x00FF))/0xFF); // 8 MSB of memory address
        packetAR[2] = ((fileMemoryLocations[currentFileIndex][n]) & ~(0xFF00)); // 8 LSB of memory address

        position = 0;
        to_send = sizeof(packetAR) + 64;
        int i;
        for (i = 0; i < to_send - 64; i++) {
            Data_Sel[i] = packetAR[i];
        }
        for (; i < to_send; i++) { //i = (to_send - 11)
            Data_Sel[i] = 0xFF;
        }
        while (UCB1STATW & UCBUSY);
        UCB1TXBUF = Data_Sel[position];

        to_send = sizeof(packetAW) + 64; // always write full pages

        while (UCB1STATW & UCBUSY);
        UCB1TXBUF = Data_Sel[position]; // send 64 byte file component
        __delay_cycles(5000);
        for(i = 0; i < sizeof(Rx_Data); i++){
            currentFile[i + 64*n] = Rx_Data[i];
        }
    }
    UCB1IE &= ~UCRXIE; 
    UCB1IFG &= ~UCTXIFG;
    UCB1IE |= UCTXIE;// enable this interrupt when you want to write.
    isRead = false;
    P2OUT &= ~BIT1;
}

void editMode() {

    int sizeOfFile = (fileSizes[currentFileIndex] * 64); // get size of new file

    char lastInput = 'I';
    int rows;
    bool input_change = false;
    while (lastInput != '*') { // create file
        listenDials(); 
        if(CW8) {
            currentChar += 1;
            currentFile[currentFilePos] = currentChar;
            sendChar(currentChar);
            sendPosition(currentPosition);
            CW8 = false;
        }
        if(CCW8) {
            currentChar += -1;
            currentFile[currentFilePos] = currentChar;
            sendChar(currentChar);
            sendPosition(currentPosition);
            CCW8 = false;
        }
        if(CW9) {
            currentPosition += 1;
            currentFilePos += 1;
            if(currentFilePos >= sizeOfFile) {
                currentFilePos = 0;
            }
            if(currentPosition > 31) {
                currentPosition = 0;
                printFromFile(currentFilePos);
            }
            sendPosition(currentPosition);
            CW9 = false;
        }
        if(CCW9) {
            currentPosition += -1;
            currentFilePos += -1;
            if(currentFilePos >= sizeOfFile) {
                currentFilePos = (sizeOfFile - 1);
            }
            if(currentPosition > 31) {
                currentPosition = 31;
                printFromFile(currentFilePos - 31); // print from 32 characters behind current position, inclusive
            }
            sendPosition(currentPosition);
            CCW9 = false;
        }
        rows = P3IN; // constantly listen for an input
        rows &= 0b11110000; // clear any values on lower 4 bits
        if (rows != 0b00000000) {
            lastInput = readInput();         
            input_change = true;
        }
        if (input_change) {
            currentChar = charCatSel(lastInput);
            currentFile[currentFilePos] = currentChar;
            sendChar(currentChar);
            sendPosition(currentPosition);
            input_change = false;
        }
    }
}

void fileSelect() {
    currentPosition = 16;
    char lastInput = 'I';
    bool input_change = false;
    displayCurrentFileName();
    while (lastInput != '*') { // select file name
        listenDials();
        if(CW8) {
            CW8 = false;
        }
        if(CCW8) {
            CCW8 = false;
        }
        if(CW9) {
            currentFileIndex += 1;
            currentFileIndex = (currentFileIndex)%number_of_files;
            displayCurrentFileName();
            CW9 = false;
        }
        if(CCW9) {
            currentPosition += -1;
            currentFileIndex = (currentFileIndex)%number_of_files;
            displayCurrentFileName();
            CCW9 = false;
        }
        rows = P3IN; // constantly listen for an input
        rows &= 0b11110000; // clear any values on lower 4 bits
        if (rows != 0b00000000) {
            lastInput = readInput();         
            input_change = true;
        }
        if (input_change) {
            input_change = false;
        }
    }
}

int main(void)
{
    // Stop watchdog timer

    WDTCTL = WDTPW | WDTHOLD;               // Stop watchdog timer
    P3DIR |= 0b00001111;   // set keypad columns to outputs pulled high
    P3OUT |= 0b00001111;

    //heartbeat LED
    P1DIR |= BIT0;                              // Sets P1.0 as an output
    P1OUT &= ~BIT0;                             // Initializes LED to OFF
    
    P2OUT &= ~BIT1;
    P2DIR |= BIT1;

    P3DIR &= ~0b11110000; // set all keypad rows to inputs pulled low
    P3REN |= 0b11111111; // permanently set all of port 3 to have resistors
    P3OUT &= ~0b11110000; // pull down resistors

    P1DIR |= RED_LED | GREEN_LED | BLUE_LED;
    P1OUT |= RED_LED | GREEN_LED | BLUE_LED;  // Start with all ON

    //heartbeat interrupt
    TB1CCTL0 |= CCIE;                            //CCIE enables Timer B0 interrupt
    TB1CCR0 = 32768;                            //sets Timer B0 to 1 second (32.768 kHz)
    TB1CTL |= TBSSEL_1 | ID_0 | MC__UP | TBCLR;    //ACLK, No divider, Up mode, Clear timer

    /*TB2CCTL0 |= CCIE;                            //CCIE enables Timer B0 interrupt
    TB2CCR0 = 16000;                            //sets Timer B0 to 1 second (32.768 kHz)
    TB2CTL |= TBSSEL_1 | ID_0 | MC__UP | TBCLR;    //ACLK, No divider, Up mode, Clear timer*/

    P5DIR |= BIT1 | BIT2;
    P5OUT &= ~(BIT1 | BIT2); // peltier heating/cooling default off
    // Disable the GPIO power-on default high-impedance mode to activate
    // previously configure port settings

    set_timer(); 

    initUART();
    initSPI();
    initADC();

    init_LCD_I2C(); // what it says, but this also likely works for LCD controller

    __enable_interrupt(); 
    int unlock = 0;

    initMemStatReg(); // ensure there are no protected bits

    while(true)
    {
        unlock = 0;
        state = LOCKED;
        update_color(state);
        while (unlock == 0) {
            unlock = waitForUnlock(); // stays here until complete passkey has been entered 
        }
        // turn status LED Blue here
        state = UNLOCKED;
        update_color(state);
        char lastInput = 'X';
        while ((lastInput != 'D' && !(lastInput < ':')) || (lastInput == '0')) {
            lastInput = readInput(); // stays here until user chooses a pattern, or chooses to lock the system
        }
        int rows;
        bool input_change = true;
        while (lastInput != 'D') {
            Data_Cnt = 0;     
            if (lastInput == '1' && input_change) { // create file
                input_change = false;
                createFile();
                rows = P3IN; // constantly listen for an input
                rows &= 0b11110000; // clear any values on lower 4 bits
                if (rows != 0b00000000) {
                    lastInput = readInput();
                    input_change = true;
                }
            }
            else if (lastInput == '2'  && input_change) { // Edit file
                input_change = false;
                editFile();
                rows = P3IN; // constantly listen for an input
                rows &= 0b11110000; // clear any values on lower 4 bits
                if (rows != 0b00000000) {
                    lastInput = readInput();
                    input_change = true;
                }
            }
            else if (lastInput == '3'  && input_change) { // view file
                input_change = false;
                viewFile();
                rows = P3IN; // constantly listen for an input
                rows &= 0b11110000; // clear any values on lower 4 bits
                if (rows != 0b00000000) {
                    lastInput = readInput();
                    input_change = true;
                }
            }
            else if (lastInput == '4'  && input_change) { // allocate memory to file
                input_change = false;
                allocateFileMemory();
                rows = P3IN; // constantly listen for an input
                rows &= 0b11110000; // clear any values on lower 4 bits
                if (rows != 0b00000000) {
                    lastInput = readInput();
                    input_change = true;
                }
            }
            else if (lastInput == '5'  && input_change) { // edit file name
                input_change = false;
                editFileName();
                rows = P3IN; // constantly listen for an input
                rows &= 0b11110000; // clear any values on lower 4 bits
                if (rows != 0b00000000) {
                    lastInput = readInput();
                     
                    input_change = true;
                }
            }
            else if (lastInput == '6'  && input_change) { // delete file
                input_change = false;
                deleteFile();
                rows = P3IN; // constantly listen for an input
                rows &= 0b11110000; // clear any values on lower 4 bits
                if (rows != 0b00000000) {
                    lastInput = readInput();
                     
                    input_change = true;
                }
            }
            else if (lastInput == '7'  && input_change) { // save files to terminal
                input_change = false;
                sendTerminal();
                rows = P3IN; // constantly listen for an input
                rows &= 0b11110000; // clear any values on lower 4 bits
                if (rows != 0b00000000) {
                    lastInput = readInput();
                     
                    input_change = true;
                }
            }
            else if (lastInput == '8'  && input_change) { // read files from terminal, pipeline them to memory
                input_change = false;
                recieveTerminal();
                rows = P3IN; // constantly listen for an input
                rows &= 0b11110000; // clear any values on lower 4 bits
                if (rows != 0b00000000) {
                    lastInput = readInput();
                     
                    input_change = true;
                }
            }
            else if (lastInput == '9'  && input_change) { // change default editing character
                input_change = false;
                changeEditChar();
                rows = P3IN; // constantly listen for an input
                rows &= 0b11110000; // clear any values on lower 4 bits
                if (rows != 0b00000000) {
                    lastInput = readInput();
                     
                    input_change = true;
                }
            }
            else {
                rows = P3IN; // constantly listen for an input
                rows &= 0b11110000; // clear any values on lower 4 bits
                if (rows != 0b00000000) {
                    lastInput = readInput();
                     
                    input_change = true;
                }
            }
        }
        enterState(0); // totally clear LCD
        //send_LED_Timer_Pause(); // disable LCD-pattern-trigger timer interrupt here (system returns to locked state)
    }
}

#pragma vector = TIMER0_B0_VECTOR
__interrupt void TimerB0_ISR(void) {

    pwms = (pwms + 1) % 256;

    // Red LED
    if (pwms == red_counter){
        P1OUT &= ~RED_LED;
    }
    if (pwms == 0) {
        P1OUT |= RED_LED;
    }

    // Green LED
    if (pwms == green_counter) {
        P1OUT &= ~GREEN_LED;
    }
    if (pwms == 0) {
        P1OUT |= GREEN_LED;
    }

    // Blue LED
    if (pwms == blue_counter) {
        P1OUT &= ~BLUE_LED;
    }
    if (pwms == 0) {
        P1OUT |= BLUE_LED;
    }
    TB0CCTL0 &= ~CCIFG;

}

#pragma vector = TIMER1_B0_VECTOR               //time B0 ISR
__interrupt void TIMERB1_ISR(void) {
    P1OUT ^= BIT0;                              //toggles P1.0 LED
    TB1CCTL0 &= ~CCIFG;
}

#pragma vector=EUSCI_B0_VECTOR
__interrupt void EUSCI_B0_I2C_ISR(void) {
    if (Data_Cnt == 0) {
        Data_Cnt = 1;
        UCB0TXBUF = dataSend[0];
        //UCB0IFG |= UCTXIFG0; 
    }
    else {
        Data_Cnt = 0;
        UCB0TXBUF = dataSend[1];
        UCB0IFG &= ~UCTXIFG0; 
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
    else if (selected_A8 && voltage < 0.10) {
        listen8 = true;
    }

    else if (selected_A9 && listen9){
        if (voltage >= 3.1){  
            listen9 = false;
             CW9 = true;
                }
        else if (voltage > 0.1 && voltage < 3.1) {  
            listen9 = false;
             CCW9 = true;
                    }
        else if (voltage < 0.1) { //  
            listen9 = true;
                    }
        else {

        }
    }
    else if (selected_A9 && voltage < 0.1) {
        listen9 = true;
    }
    else {} 
    __enable_interrupt();
    complete = true;
}
//------- END ADC ISR ----------------
//------- SPI ISR -----------
#pragma vector = EUSCI_B1_VECTOR
__interrupt void ISR_EUSCI_B1(void) { 
    switch(UCB1IV) {
    case 2:
        {   
            position++;
            if (position >= sizeof(packetAR) && position < to_send) {
                UCB1TXBUF = Data_Sel[position];
                Rx_Data[position - sizeof(packetAR)] = UCB1RXBUF & ~(0b100000000); // Store actual data, clear msb as temporary countermeasure to issue
            } 
            else if (position < sizeof(packetAR) && position < to_send) {
                UCB1TXBUF = Data_Sel[position];
                volatile char junk = UCB1RXBUF;     // Discard dummy RX
            }
            else {}
        }
        break;
    case 4:
         {
            //volatile char junk = UCB1RXBUF;
            position++;
            if (position < to_send) {
                UCB1TXBUF = Data_Sel[position];
            }
            else {
                UCB1IFG &= UCTXIFG;
            }
        }
        break;
    default:
        break;
    }
}
//----------- END SPI ISR -----------------
