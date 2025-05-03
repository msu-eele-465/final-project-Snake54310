
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

// ----------- UART VARIABLES ----------
//char out_string_first[] = "\n\r Motor advanced 1 step \r\n";
//char out_string_last[] = "\n\r Motor reversed 1 rotation. \r\n";
char *out_string;
// int bit_check;
volatile int positionU;
volatile int positionUr = 0;
// char Received[64] = ""; replace with currentFile, size 1043 (1024 bytes for file, one byte for safety, 
//16 bytes for file name, 1 byte for file size, one byte for safety), 1 for EOFR (end of file read)
// char new_string[70]; // also replace this with currentFile. it is crucial to limit size of on-board datastructures
// ------------- END UART VARIABLES ----------

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
char packetAR[5] = {0b00000011, 0, 0, 0};
char packetAW[4] = {0b00000010, 0, 0};

volatile char Data_Sel[69] = {};
volatile bool isRead = false;
volatile int position = 0;
volatile unsigned int to_send = 0;
volatile int Rx_Data[65];
// --------  END SPI VARIABLES

volatile unsigned int dataSend[2] = {69, 43};
volatile unsigned int Data_Cnt = 0;
volatile unsigned int pwms = 0;

volatile system_states state = LOCKED;

volatile char fileNames[33][17] = {}; // 32 files, 16 characters each
volatile uint16_t fileMemoryLocations[33][17] = {}; // 32 files, up to 16 memory locations each
volatile uint8_t fileSizes[33] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0}; 
volatile uint16_t nextFreeMemory = 0; // this is the next free memory location to which we allocate memory. Starts at 0.
volatile uint8_t currentFileIndex = 0;
volatile uint8_t nextFileIndex = 0;
volatile char currentFile[1045] = {};
volatile char currentPosition = 0;
volatile char currentChar = 0;
volatile char defaultChar = ' '; // will be ' ' after debugging
volatile char CurrentFileName[17] = {}; // name may be misleading--this is only used in actual file creation. Technically unecessary, but a convenient middle step
volatile char number_of_files = 0;


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

    int i;
    for (i = 0; i < 16; i++) {
        CurrentFileName[i] = defaultChar; // save default file name
        sendChar(defaultChar);
    }
    sendPosition(currentPosition);
    
    FileNameEditor();

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

    char lastInput = 'I';
    bool input_change = false;
    unsigned int currentFilePos = 0;
    int rows;

    while (lastInput != '*') { // view file
        listenDials(); 
        if(CW8) {
            currentPosition ^= 0b00010000; // always switch the line--this is simple
            currentFilePos += 16;
            currentFilePos = currentFilePos % sizeOfFile;
            if(currentPosition < 16) { // this means bit has been un-set, so we print the next two lines
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
            if(currentPosition > 15) { // this means bit has been set, so we print the previous two lines
                printFromFile(currentFilePos - currentFilePos%32);
            }

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
    // saveCurrentFile();// save file to memory at defined locations -- was useful for testing, but is unnessesary for viewing.
    sendMessage(6); // notify user of end file viewing
    return;
    
}
void allocateFileMemory() {
    currentFileIndex = 0;
    sendHeader(1); // "select file" message
    enterState(2); // viewing mode (view file names)
    
    fileSelect();

    loadFileFromMem(); // this is so we can update it after allocating more memory. 

    sendHeader(3);
    enterState(2);

    currentPosition = 16;
    char lastInput = 'I';
    bool input_change = false;

    volatile char prevSize = fileSizes[currentFileIndex];
    volatile char addedSize = 0;
    volatile char maxAdd = 16 - prevSize;
    bool displayNewSize = false;
    int rows;

    while (lastInput != '*') { // select file size
        listenDials();
        if(CW8) {
            addedSize = (addedSize + 1)%(maxAdd + 1);
            CW8 = false;
            displayNewSize = true;
        }
        if(CCW8) {
            addedSize = addedSize - 1;
            if(addedSize > maxAdd) {
                addedSize = maxAdd;
            }
            CCW8 = false;
            displayNewSize = true;
        }
        if(CW9) {
            CW9 = false;
        }
        if(CCW9) {
            CCW9 = false;
        }
        if (((addedSize + prevSize) > 9) && (displayNewSize)) {
            sendPosition(currentPosition);
            sendChar('1');
            int displaysize = addedSize + prevSize - 10;
            sendChar(displaysize + '0');
            displayNewSize = false;
        }
        else if (displayNewSize) {
            sendPosition(currentPosition);
            sendChar(' ');
            sendChar(' ');
            sendPosition(currentPosition);
            int displaysize = addedSize + prevSize;
            sendChar(displaysize + '0');
            displayNewSize = false;
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
    fileSizes[currentFileIndex] = addedSize + prevSize;
    int i;
    for (i = 0; i < addedSize; i++) {
        fileMemoryLocations[currentFileIndex][prevSize + i] = nextFreeMemory;
        nextFreeMemory = nextFreeMemory + 64;
    }
    for(i = (prevSize*64); i < ((addedSize + prevSize)*64); i++) { // keep it fresh, as they say. Or, delete this for haxx to access previous files
        currentFile[i] = defaultChar;
    }

    saveCurrentFile();

    sendMessage(1);
    return;
}
void editFileName() {
    currentFileIndex = 0;
    sendHeader(1); // "select file" message
    enterState(2); // viewing mode (view file names)
    
    fileSelect();

    sendHeader(0); // setup LDC and file name pointer
    enterState(1);
    currentPosition = 16;
    sendPosition(currentPosition);

    int i;
    for (i = 0; i < 16; i++) {
        CurrentFileName[i] = fileNames[currentFileIndex][i]; // load file name
        sendChar(CurrentFileName[i]);
    }

    sendPosition(currentPosition);

    FileNameEditor();

    sendMessage(3); // name updated message
    return;
}
void deleteFile() {
    currentFileIndex = 0;
    sendHeader(1); // "select file" message
    enterState(2); // viewing mode (view file names)
    
    fileSelect(); 

    fileSizes[currentFileIndex] = 19; // code for "addesses assocaited with this file position are free"
    int i;
    for (i = 0; i < 16; i++) {
        fileNames[currentFileIndex][i] = 'D';
    }
    // do not decrease file counter-- this file and it's memory locations technically still exist, but we want to track freed memory addresses
    sendMessage(4); // file deleted message
    return;
}
void sendTerminal() {
    TB0CCTL0 &= ~CCIE; // disable rgb timer inturrupt
    TB1CCTL0 &= ~CCIE;
    ADCIE &= ~ADCIE0;
    // sift through all files, pulling every one (besides deleted ones: file size 19) into currentFile (using loafFileFromMem()), and sending them over uart 
    // in the form: file name (16 bytes), fileSize(as a character representing the number of pages), file contents (correct number exactly)
    int i;
    volatile int checkFileSize = fileSizes[0];
    for (i = 0; i < number_of_files; i++) {
        __delay_cycles(300);
        checkFileSize = fileSizes[i];
        __delay_cycles(300);
        if (checkFileSize != 19) { // do not send deleted files
            currentFileIndex = i;
            loadFileFromMem();
            volatile int n;
            volatile int endPosition = fileSizes[currentFileIndex]*64;
            for(n = 1; n < 17; n++) { // leave a space
                volatile int nameIndex = endPosition + n;
                volatile char currentNameChar = fileNames[currentFileIndex][n - 1];
                __delay_cycles(20);
                currentFile[nameIndex] = currentNameChar;
            }
            currentFile[endPosition + 17] = fileSizes[currentFileIndex] + '0';
            currentFile[endPosition + 18] = 243;
            out_string = currentFile; // set string to reverse string
            positionU = 0;
            UCA1IE |= UCTXCPTIE;  // enables IQR
            UCA1IFG &= ~UCTXCPTIFG; // clears flag
            UCA1TXBUF = out_string[positionU]; // prints first character, triggering IQR
            __delay_cycles(300000); // .0002 * (1044 + 6) = .21 < .3
        }
    }
    TB0CCTL0 |= CCIE; // enable rgb timer inturrupt
    TB1CCTL0 |= CCIE;
    ADCIE |= ADCIE0;
    sendMessage(7);
    return;
}
void recieveTerminal() {
    TB0CCTL0 &= ~CCIE; // disable rgb timer inturrupt
    TB1CCTL0 &= ~CCIE;
    ADCIE &= ~ADCIE0;

    positionUr = 0;
    positionU = -1;
    UCA1IE |= UCRXIE;
    while(UCA1IE & UCRXIE){} 

    int sizeRecieved = currentFile[positionUr - 2] - '0';
    if(sizeRecieved > 16) {
        sizeRecieved = 2; // stopgap to prevent issues with improper recieves
    }
    fileSizes[nextFileIndex] = sizeRecieved; // format new file location pointers

    int i;
    for (i = 0; i < sizeRecieved; i++) {
        fileMemoryLocations[nextFileIndex][i] = nextFreeMemory;
        nextFreeMemory = nextFreeMemory + 64;
    }
    currentFileIndex = nextFileIndex;
    nextFileIndex += 1;
    number_of_files += 1;

    int rawFileSize = 64 * sizeRecieved;

    for (i = 0; i < 16; i++) { // take into account space *** ACTUALLY SPACE DISAPPEARS IN TERMINAL RECIEVE *** THIS IS NOW CORRECT ***
        fileNames[currentFileIndex][i] = currentFile[rawFileSize + i]; // save file name
    }

    saveCurrentFile();

    TB0CCTL0 |= CCIE; // enable rgb timer inturrupt
    TB1CCTL0 |= CCIE;
    ADCIE |= ADCIE0;
    sendMessage(8);
    return; 
}
void changeEditChar() {
    sendHeader(2); // "select default character" message
    enterState(1); // editing mode (view file names)

    char lastInput = 'I';
    int rows;
    bool input_change = false;
    int currentPosition = 16;
    sendPosition(currentPosition);
    while (lastInput != '*') { // create file name
        listenDials();
        if(CW8) {
            defaultChar += 1;
            sendChar(defaultChar);
            sendPosition(currentPosition);
            CW8 = false;
        }
        if(CCW8) {
            defaultChar += -1;
            sendChar(defaultChar);
            sendPosition(currentPosition);
            CCW8 = false;
        }
        if(CW9) {
            CW9 = false;
        }
        if(CCW9) {
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
            if(currentChar != 0x02) {
                defaultChar = currentChar;
            }
            sendChar(defaultChar);
            sendPosition(currentPosition);
            input_change = false;
        }
    }

    sendMessage(5);
}


void FileNameEditor() {
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
            char nextChar = charCatSel(lastInput);
            if (nextChar != 0x02) {
                currentChar = nextChar;
            }
            CurrentFileName[currentPosition - 16] = currentChar;
            sendChar(currentChar);
            sendPosition(currentPosition);
            input_change = false;
        }
    }
    int i;
    for (i = 0; i < 16; i++) {
        fileNames[currentFileIndex][i] = CurrentFileName[i]; // save file name
    }
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
    char currentChar = 0x02;
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
        case '3':
            currentChar = 'N';
            break;
        case '6':
            currentChar = 'n';
            break;
        case '2':
            currentChar = 'Z';
            break;
        case '5':
            currentChar = 'z';
            break;
        case '1':
            currentChar = '(';
            break;
        default: 
            break;
    }
    return currentChar;
}
void printFromFile(int filePosition) {
    int i;
    sendPosition(0);
    for (i = filePosition; i < (filePosition + 32); i++) {
        if(i == filePosition + 16) {
            sendPosition(16);
        }
        sendChar(currentFile[i]);
    }
    return;
}
void saveCurrentFile() { 
    TB0CCTL0 &= ~CCIE; // disable rgb timer inturrupt
    TB1CCTL0 &= ~CCIE;
    UCA1CTLW0 &= ~UCSWRST;
    ADCIE &= ~ADCIE0;
    P2OUT |= BIT1; // LED to tell user program is writing/reading
    isRead = false;
    int sizeOfCurrent = fileSizes[currentFileIndex];
    int n;
    int positionFile = 0;
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

        to_send = 67; // always write full pages
        int i;
        for (i = 0; i < 3; i++) {
            Data_Sel[i] = packetAW[i];
        }
        for (; i < to_send; i++) {
            Data_Sel[i] = currentFile[positionFile];
            positionFile++;
        }
        while (UCB1STATW & UCBUSY);
        UCB1TXBUF = Data_Sel[position]; // send 64 byte file component
        __delay_cycles(50000);
    }

    P2OUT &= ~BIT1;
    TB0CCTL0 |= CCIE; // enable rgb timer inturrupt
    TB1CCTL0 |= CCIE;
    UCA1CTLW0 &= ~UCSWRST;
    ADCIE |= ADCIE0;

}

void displayCurrentFileName() {
    sendPosition(16);
    int i;
    for (i = 0; i < 16; i++) {
        sendChar(fileNames[currentFileIndex][i]);
    }
}

void loadFileFromMem() {
    TB0CCTL0 &= ~CCIE; // disable timer interrupt
    TB1CCTL0 &= ~CCIE; 
    UCA1CTLW0 &= ~UCSWRST;
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

        to_send = 68; // always read full pages
        int i;
        for (i = 0; i < to_send - 64; i++) {
            Data_Sel[i] = packetAR[i];
        }
        for (; i < to_send; i++) { //i = (to_send - 64)
            Data_Sel[i] = 0xFF;
        }
        while (UCB1STATW & UCBUSY);
        UCB1TXBUF = Data_Sel[position];

        while (UCB1STATW & UCBUSY);
        UCB1TXBUF = Data_Sel[position]; // send 64 byte file component
        __delay_cycles(50000);
        for(i = 0; i < 64; i++){
            currentFile[i + 64*n] = Rx_Data[i];
        }
    }
    UCB1IE &= ~UCRXIE; 
    UCB1IFG &= ~UCTXIFG;
    UCB1IE |= UCTXIE;// enable this interrupt when you want to write.
    isRead = false;
    P2OUT &= ~BIT1;
    UCA1CTLW0 &= ~UCSWRST;
    TB0CCTL0 |= CCIE; // re-enable timer inturrupt
    TB1CCTL0 |= CCIE;
}

void editMode() {

    int sizeOfFile = (fileSizes[currentFileIndex] * 64); // get size of new file

    char lastInput = 'I';
    int rows;
    bool input_change = false;
    unsigned int currentFilePos = 0;
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
            char nextChar = charCatSel(lastInput);
            if (nextChar != 0x02) {
                currentChar = nextChar;
            }
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
    int rows;
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
            if (currentFileIndex >= number_of_files) {
                currentFileIndex = 0;
            }
            if(fileSizes[currentFileIndex] == 19) {
                currentFileIndex += 1;
            }
            if (currentFileIndex >= number_of_files) {
                currentFileIndex = 0;
            }
            displayCurrentFileName();
            CW9 = false;
        }
        if(CCW9) {
            currentFileIndex += -1;
            if (currentFileIndex >= number_of_files) {
                currentFileIndex = number_of_files;
                currentFileIndex += -1;
            }
            if(fileSizes[currentFileIndex] == 19) {
                currentFileIndex += -1;
            }
            if (currentFileIndex >= number_of_files) {
                currentFileIndex = number_of_files;
                currentFileIndex += -1;
            }
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
            else if (lastInput == '2'  && input_change && number_of_files > 0) { // Edit file
                input_change = false;
                editFile();
                rows = P3IN; // constantly listen for an input
                rows &= 0b11110000; // clear any values on lower 4 bits
                if (rows != 0b00000000) {
                    lastInput = readInput();
                    input_change = true;
                }
            }
            else if (lastInput == '3'  && input_change && number_of_files > 0) { // view file
                input_change = false;
                viewFile();
                rows = P3IN; // constantly listen for an input
                rows &= 0b11110000; // clear any values on lower 4 bits
                if (rows != 0b00000000) {
                    lastInput = readInput();
                    input_change = true;
                }
            }
            else if (lastInput == '4'  && input_change && number_of_files > 0) { // allocate memory to file
                input_change = false;
                allocateFileMemory();
                rows = P3IN; // constantly listen for an input
                rows &= 0b11110000; // clear any values on lower 4 bits
                if (rows != 0b00000000) {
                    lastInput = readInput();
                    input_change = true;
                }
            }
            else if (lastInput == '5'  && input_change && number_of_files > 0) { // edit file name
                input_change = false;
                editFileName();
                rows = P3IN; // constantly listen for an input
                rows &= 0b11110000; // clear any values on lower 4 bits
                if (rows != 0b00000000) {
                    lastInput = readInput();
                     
                    input_change = true;
                }
            }
            else if (lastInput == '6'  && input_change && number_of_files > 0) { // delete file
                input_change = false;
                deleteFile();
                rows = P3IN; // constantly listen for an input
                rows &= 0b11110000; // clear any values on lower 4 bits
                if (rows != 0b00000000) {
                    lastInput = readInput();
                     
                    input_change = true;
                }
            }
            else if (lastInput == '7'  && input_change && number_of_files > 0) { // save files to terminal
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
            if (position >= 4 && position < to_send) {
                UCB1TXBUF = Data_Sel[position];
                Rx_Data[position - 4] = UCB1RXBUF & ~(0b10000000); // Store actual data, clear msb as temporary countermeasure to issue
            } 
            else if (position < 4 && position < to_send) {
                UCB1TXBUF = Data_Sel[position];
                volatile char junk = UCB1RXBUF;     // Discard dummy RX
            }
            else {}
        }
        break;
    case 4:
         {
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
//-- UART ISR -------------------------------
#pragma vector=EUSCI_A1_VECTOR
__interrupt void ISR_EUSCI_A1(void)
{   //if ((UCTXCPTIFG & UCA1IFG) == 0) { // does not work because UCTXCPTIFG is always has bit 3 set here
    if (positionU == -1) { // if position has not been set to zero, or is otherwise positive. Might have something to do with notepad or copy/paste, too.
        currentFile[positionUr] = UCA1RXBUF;
        positionUr ++;
        if(currentFile[positionUr - 1] == 195 || positionUr == 1044 || currentFile[positionUr - 1] == 179 || currentFile[positionUr - 1] == 243) { // make sure we don't go over max array index, also, for some reason the 
        // terminal thinks it should send 'ó' as 195 179, or something. Whatever--I'll just watch for everything.
            UCA1IE &= ~UCTXCPTIE; // clear flag
            UCA1IE &= ~UCRXIE; // disable this interrupt
        }
        else {
            // do nothing and wait for next character
        }
    }
    else { // if position has been set to 0 or is currently in use
        if (positionU == (fileSizes[currentFileIndex]*64 + 18)){ // if position now equals the string length
            UCA1IE &= ~UCTXCPTIE; // clear flag
            positionU = -1; // set position back to default of -1
        }
        else { // if string still has unsent characters
            positionU ++;
            __delay_cycles(200); // 1,000,000/60 * (8 + 2) < .0002
            UCA1TXBUF = out_string[positionU]; // send nth character of string
        }
    }
    UCA1IFG &= ~UCTXCPTIFG;
}
//-- END UART ISR -------------------------------