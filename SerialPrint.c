/*
 *  SerialPrint.cpp
 *
 *  Prints to a serial terminal/E-cube log
 *  file via UART. Requires full printf
 *  support to be enabled in compiler settings.
 *
 *  Usage: serialPrint("Integer: %d", intValue): To print integers
 *         serialPrint("Float: %f", floatValue): To print floating point numbers.
 *
 *  Baud: 9600
 *  TX: P2.0
 *  eUSCI_A0
 *
 */

#include <msp430.h>
#include <stdio.h>
#include "SerialPrint.h"

void serialPrintInit() 
{

    //Set P2.0 to UCA0TXD function
    P2SEL1 |= BIT0;
    P2SEL0 &= ~BIT0;

    //Wait for DCO to stabilise in case it hasn't
    __delay_cycles(10);

    //Configure eESCU_A0 for UART
    UCA0CTLW0 = UCSWRST;                   // Reset state
    UCA0CTLW0 |= UCSSEL__SMCLK;            // SMCLK as clock source
    UCA0BRW = 52;                          // = (int) Freq/OverS/baud = 8000000/16/9600
    UCA0MCTLW |= UCOS16 | UCBRF_1 | 0x4900;// Oversampling by 16, 1st & 2nd stage modulation
    UCA0CTLW0 &= ~UCSWRST;                 // Enable

}

void serialPrint(const char *format, ...) 
{

    char buffer[BUFFER_SIZE];
    
    va_list args;
    va_start(args, format);                        // Variable arguments
    vsnprintf(buffer, BUFFER_SIZE, format, args);  // Format to string
    va_end(args);

    const char *data = buffer;

    while (*data) {
        while (!(UCA0IFG & UCTXIFG));       // Wait for TX buffer to be ready
        UCA0TXBUF = *data++;                // Send byte
    }

}


// void serialPrint(const char * fmt, ...)
// {
// 	//va_list declares argument pointer
// 	va_list ap;
// 	//va_start makes ap point to the first argument
// 	va_start(ap, fmt);

// 	//char condtion;
// 	//int numValue;
// 	float floatValue;
// 	//double doubleValue;
// 	//char charValue;
// 	//char * s;

//     char buffer[BUFFER_SIZE];

// 	//fmt has the base address of first string argument
// 	while (*fmt)
// 	{
// 		//if we encounter % then go inside if
// 		if (*fmt == '%')
// 		{
// 			//increment fmt to know what is the datatype
// 			fmt++;
// 			switch (*fmt)
// 			{
// 				// case 'd':
// 				// 	//for int datatype
// 				// 	//va_args gets the next argument of the list
// 				// 	//need to specify data type
// 				// 	numValue = va_arg(ap, int);
// 				// 	//print op
// 				// 	fprintf(buffer, "%d", numValue);
// 				// 	break;
// 				// case 'c':
// 				// 	//for character dt
// 				// 	charValue = va_arg(ap, int);
// 				// 	fprintf(buffer, "%c", charValue);
// 				// 	break;
// 				case 'f':
// 					//for float data type
// 					floatValue = (float)va_arg(ap, double);
// 					fprintf(buffer, "%f", floatValue);
// 					break;
				
// 				// case 's':
// 				// 	//for string
// 				// 	s = va_arg(ap, char *);
// 				// 	while(*s)
//    			 	// 	{
//         		// 			putchar(*s);
//         		// 			s++;
//    	 			// 	}
// 				// 	break;
// 				default:
// 					break;
// 			}
// 		}
// 		//if no match then just print that character
// 		else
// 		{
// 			putchar(*fmt);
// 		}
// 		fmt++;
// 	}

//     const char *data = buffer;
//     while (*data) 
//     {
//         while (!(UCA0IFG & UCTXIFG));// Wait for TX buffer to be ready
//         UCA0TXBUF = *data++;         // Send byte
//     }

//     	//clear argument pointer
// 	va_end(ap);
// 	return;
// }

