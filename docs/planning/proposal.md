# Final project proposal

- [V] I have reviewed the project guidelines.
- [V] I will be working alone on this project.
- [V] No significant portion of this project will be (or has been) used in other course work.

## Embedded System Description

I will create a text editor, which will be visible on the LCD display. The user will be able to create files with specific file names, edit files or file names, and approximately control file sizes. The files will be stored on a subservient memory device, and only exist on the MSP430 during editing. The files will be sendable to and readable (MSP430 can recieve and store formatted files) from a PC terminal. 

The inputs include the membrane keypad, the PC terminal (uart) and two potentiometers. 
The outputs include the LCD display, the RGB led, an LED indicating reads and writes from the EEPROM, and the PC terminal (uart)

## Hardware Setup

What hardware will you require? Provide a conceptual circuit diagram and/or block diagram to help the reviewers understand your proposal. Be sure to introduce and discuss your figures in the text.
- LCD and MSP 430 variants (FR2355 1x and FR2311 1x) wired as usual (baseline)
- EEPROM module connected to MSP 430 via SPI, otherwise wired for necessary functionality
- MSP wired through test bench to PC to enable UART communication over USB

## Software overview

Discuss, at a high level, a concept of how your code will work. Include a *high-level* flowchart. This is a high-level concept that should concisely communicate the project's concept.
Main controller (initial/home states): 
 - starts locked, may be locked to clear cache and all operations (memory stored in EEPROM is retained). A pin is required to unlock
 - Constantly runs a status RGB LED off of a timer interrupt
 - When unlocked, listens for the following commands from keypad: 
 1. Create New File (prompts user for name, user enters name using editor funtionality)
 2. Edit File (user selects existing name, enters editing mode) 
 3. View File (user selects existing name, enters viewing mode)
 4. Allocate more memory to file (user selects existing name, may allocate additonal memory in 128 byte increments)
 5. Edit file name (user selects existing name, prompts user for name)
 6. Delete file (user selects existing name, file is removed from system memory, EEPROM space re-listed as 'free' to be used for next file creation)
 7. save files to terminal (memory is read from EEPROM and sent to terminal in format that can be re-downloaded)
 8. read files from terminal (memory previously sent to terminal is re-read from terminal, if the user send them. Can be canceled.)
 9. change default editing character
 D. lock system

 - All file names are stored here, along with their 'start' location in EEPROM memory

Main controller (Editing Mode): 
 - Selected is loaded onto MSP430 from EEPROM
 - one potentiometer is used to scroll through document
 - a sepatate potentiometer is used to iterate through charater numbers
 User may also run through characters in the following way:
 A. beginning of capital letters
 B. beginning of lowercase letters
 C. Beginning of numbers
 #. beginning of symbols
 - user may use * key to save file at any time
 - user may use 1 key to avoid saving changes.
 - current location in file is carefully tracked
 - currently selected character is carefully tracked
 - current position on LCD is carefully tracked
 - current file line is carefully tracked
 - non-edited characters are all spaces, to begin
  
 Main controller (Viewing Mode): 
- Selected is loaded onto MSP430 from EEPROM
 - one potentiometer is used to scroll through document characters
 - one potentiometer is used to scroll through document lines
 - user may use * key to exit document
 - current location in file is carefully tracked
 - current position on LCD is carefully tracked
 - current file line is carefully tracked

 Saved file details (in EEPROM): 
 - all files begin with a verifiable 'start' byte (maybe 0x9E, for example)
 - after every 128 byte sequence, this follows:
 1. ending byte (maybe 0x9F, for example)
 2. indicator for whether file continues (0x01 if not, 0x20 if so)
 2. address of file continuation (if it exists. Space for this is reserved regardless)
 3. 4 bytes of empty space
 4. start of next file or jump

 - main controller records location to save next file
 - main controller records location to save next expansion.

 LCD controller:
 - Displays information pertaining to currently selected action
 - In editing or viewing mode, displays 32 file characters at a time
 - displays short notifications when tasks are completed:
 1. File saved
 2. Files Uploaded (To PC)
 3. File created
 4. File not saved
 5. Files recorded (from PC)
 - clears LCD whevever system is locked

## Testing Procedure

I will begin with a circuit diagram and system architecture diagram, and also sufficient information to provide a general code-flow overview. 
We have already verified I2C reads and writes, so that will not be necessary. UART comminication will be verifiable by functionality demonstration.
I will provide screenshots of SPI read and write communications to verify information storage and retrieval to/from the EEPROM.
All other requirement verification will be seen during the demo.

## Prescaler

Desired Prescaler level: 

- [V] 100%
- [ ] 95% 
- [ ] 90% 
- [ ] 85% 
- [ ] 80% 
- [ ] 75% 

### Prescalar requirements 

**Outline how you meet the requirements for your desired prescalar level**

- I will meet the I/O requirements
- My project has a specific goal to fulfill a specific purpose
- My project is relatively complex (more-so than anything we have done thus far)
- My project will introduce me to new concepts (SPI communications, external memory management)
- My project encorperates slave/master functionality in SPI, I2C, and (potentially) UART (the laptop terminal is always listening for communication, 
but the MSP430 will decide whether it is prepared to receive comminication at any time)
 
**The inputs to the system will be:**
1.  Membrane Keypad
2.  My laptop terminal
3.  2x potentiometer
4.  Either SPI or I2C EEPROM (25LC256 or 24LC256) (maybe not considered an input, but I still have 3)

**The outputs of the system will be:**
1.   My laptop terminal
2.   LCD display
3.   RGB status LED
4    Read/write LED for EEPROM (on during EEPROM communications)
5.   Either SPI or I2C EEPROM (25LC256 or 24LC256) (maybe not considered an output, but I still have 4)

**The project objective is**

I will create a text editor. The LCD will display file names, file contents, and other useful information. I will interface the MSP430 with EEPROM using either I2C or SPI communication, and use it to store .txt files. The user will be able to select an initial file size, or update the file size if they wish to add more content. The user input devices will enable the user to seamlessly create, edit, and save files in an efficent manner. The user may save the file contents to a PC terminal via UART communication, and load files into the MSP430 and the EEPROM in a similar way.

**The new hardware or software modules are:**
1. Short description of new hardware or software module
2. 2x potentiometer


The Master will be responsible for:
- maintaining and current system state
- communicating with Laptop terminal via UART
- managing and tracking current system data
- managing and storing data in EEPROM

The Slave(s) will be responsible for:
* LCD Controller ~
- Displaying text file information to user
- Displaying options to user
- Disyplaying system status information
* EEPROM~
- Storing file contents
- Sending file contents


### Argument for Desired Prescaler

I believe this deserves a 100% prescalar due to the explicit goal chosen, and the utility of that goal. Additionally, I am going to meet the novelty-related 
hardware and software requirements with the potentiometers and the EEPROM interface. I am also surpassing or meeting the I/O quantity requirements. 

