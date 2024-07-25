/*
 * VCCmonitor.h
 *
 *   Monitors the on-chip supply voltage using
 *   ADC12_B, which is triggered by Timer_A1. A measurement
 *   is taken every 100ms, triggering an interrupt
 *   if it is below/above defined threshold voltages.
 *
 *   AVCC / 2 is used as the VCC reference, along with a 1.2V
 *   internal reference, thus the formula to convert digital
 *   readings to analog voltages is:
 *
 *    analogVolts = (2 * digitalVolts * (1.2 / 4095))
 *
 *   The 1.2V reference provides accurate VCC measurement
 *   between 1.8V and 2.4V.
 *
 */

#include <msp430.h>

#ifndef VCCMONITOR_H_
#define VCCMONITOR_H_

// Starts Timer_A1 to trigger ADC12_B
void initADCTimer(void);

// Initialises ADC12_B for VCC measurement
void initVCCADC (void);

void restoreFunctionSet(void);

#if DEBUG
  //Prints analog and digital voltage values for debugging
  void DEBUGreadADC (void);
#endif

#endif /* VCCMONITOR_H_ */
