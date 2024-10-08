
/*
* Hibernus for MSP430FR5994:
* 
* Hibernus: Software-based approach to intelligently hibernate and restore the system's state
* in response to a power failure. This software exploits an ADC VCC Monitor.
*
*/

#ifndef HIBERNATION_H
#define HIBERNATION_H

#include "sys/cdefs.h"
#include <stdint.h>
#include <msp430.h>
#include <stdio.h>
#include <string.h>



#define LED1 BIT7  // P3.7
#define LED2 BIT6  // P3.6
#define LED3 BIT5  // P3.5
#define LED4 BIT4  // P3.4
#define LED5 BIT3  // P7.3


//FRAM
// #define FRAM_START_EXPLICIT 0x45E0
// #define FRAM2_START 0x10000
//#define FRAM_END 0xFF40

#define FRAM_START_EXPLICIT 0xEDF0
#define FRAM_END 0xFF40

// RAM
#define RAM_START 0x1C00
#define RAM_END 0x2C00

//PC
#define PC 0xEDF0





void systemInitialisation(void);

void Hibernate(void);
void SaveRAMSnapshot (void);
void SaveGPRegister (void);

void Restore(void);
void RestoreGPRegisters (void);



void initGPIO(void);
void initMSPClock(void);

//code for debugging hibernus
void setupButtonInterruptP5();
void setupButtonInterruptP6();

void updateBlockSelectRetention();

#endif
