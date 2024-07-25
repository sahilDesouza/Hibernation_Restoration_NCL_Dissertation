/*
 *  SerialPrint.h
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


#ifndef SERIALPRINT_H_
#define SERIALPRINT_H_

#include <stdarg.h>

#define BUFFER_SIZE 128

//Initialises GPIO pins & UART for printing
void serialPrintInit();

//Prints
void serialPrint(const char *format, ...);

#endif /* SERIALPRINT_H_ */
