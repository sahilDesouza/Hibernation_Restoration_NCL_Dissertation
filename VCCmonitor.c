/*
 * VCCmonitor.cpp
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

#include "VCCmonitor.h"
#include "hibernation.h"
#include "intrinsics.h"
#include "msp430fr5994.h"

#define LOW_VCC 3583  // 2.1V
#define HIGH_VCC 3924 // 2.3V

// #define LOW_VCC 2354  // 2.1V
// #define HIGH_VCC 2867 // 2.3V

// #define LOW_VCC 2354  // <2.3V
// #define HIGH_VCC 3071 // >3V

#define DEBUG 0

#if DEBUG
  #include <SerialPrint.h>
#endif

extern int restoreDoneSetFlag;
extern int hibernateDoneFlagSet;
extern int additionalFlag;
extern int hibernateRecalled;
extern int hibernusInitial;

void initVCCADC(void) 
{
    ADC12CTL0 &= ~ADC12ENC;        // Disable conversion
    ADC12CTL0 |= ADC12SHT0_3;      // 32 cycle sampling time
    ADC12CTL0 |= ADC12ON;          // Turn ADC12_B on
    ADC12CTL1 |= ADC12SHP;         // Use sampling timer
    ADC12CTL1 |= ADC12SHS_4;       // Trigger on TA1 CCR1 output
    ADC12CTL1 |= ADC12CONSEQ_2;    // Repeat-single-channel mode
    ADC12CTL2 |= ADC12RES_2;       // 12-bit conversion
    ADC12MCTL0 |= ADC12VRSEL_1;    // VR+ = VREF buffered, VR- = AVSS
    ADC12MCTL0 |= ADC12WINC;       // Window comparator enable
    ADC12CTL3 |= ADC12BATMAP_1;    // Set A31 to AVCC / 2
    ADC12MCTL0 |= ADC12INCH_31;    // Input channel A31 for internal AVCC/2 reference
    while (REFCTL0 & REFGENBUSY);  // Wait if the reference generator is busy
    REFCTL0 |= REFVSEL_0 | REFON;  // Enable 1.2V reference (Requires AVCC to be at least 1.8V)
    while(!(REFCTL0 & REFGENRDY)); // Wait for reference to settle
    ADC12CTL0 |= ADC12ENC;         // Enable conversion
    ADC12HI = HIGH_VCC;                 // Set HIGH window comparator threshold
    ADC12LO = LOW_VCC;                  // Set LOW window comparator threshold
    ADC12IER2 |= ADC12LOIE | ADC12HIIE | ADC12INIE; // Enable high, in, low threshold interrupts
    __delay_cycles(400);
    

}

void initADCTimer(void) 
{
    TA1CTL |= TACLR;         // Clear timer
    TA1CCR0 = 328 - 1;       // Set the period for Timer A1
    TA1CCTL1 |= OUTMOD_7;    // Set/Reset mode
    TA1CCR1 = 164;           // Set the duty cycle to 50%
    TA1CTL |= TASSEL__ACLK;  // Use ACLK (32768Hz) = 100ms  period
    TA1CTL |= MC__UP;        // Up-mode
    __delay_cycles(400);
    __bis_SR_register(GIE);             // Enable global interrupts
}


#if DEBUG
#include <stdint.h>
void DEBUGreadADC (void) 
{
    while (ADC12CTL1 & ADC12BUSY);      // Wait for conversion to complete
    uint16_t digitalVolts = ADC12MEM0;  // Read result from the memory buffer

    float analogVolts = (2.0 * (float) digitalVolts * (1.2 / 4095.0)); // Convert digital ADC reading to analog volts

    serialPrint("VCC = %fV, ", analogVolts); // Print analog VCC in volts
    serialPrint("ADC: %d \n\r", digitalVolts); // Print digital ADC reading
}

#endif

//ADC12_B Interrupt service routine
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = ADC12_B_VECTOR
__interrupt void ADC12_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(ADC12_B_VECTOR))) ADC12_ISR (void)
#else
#error Compiler not supported!
#endif
{
    switch(__even_in_range(ADC12IV, ADC12IV__ADC12RDYIFG))
    {
        case ADC12IV__NONE:        break;   // Vector  0:  No interrupt
        case ADC12IV__ADC12OVIFG:  break;   // Vector  2:  ADC12MEMx Overflow
        case ADC12IV__ADC12TOVIFG: break;   // Vector  4:  Conversion time overflow

        case ADC12IV__ADC12HIIFG:           // High Interrupt    

            ADC12IFGR2 &= ~ADC12HIIFG;      // Clear high interrupt flag
            ADC12IFGR2 &= ~ADC12LOIFG;      // Clear low interrupt flag
            ADC12IFGR2 &= ~ADC12INIFG;      // Clear in interrupt flag
            
            ADC12IER2 &= ~ADC12HIIE; // Disable high interrupt            
            ADC12IER2 |= ADC12LOIE;  // Enable low interrupt
            ADC12IER2 |= ADC12INIE;  // Enable in window interrupt
            
            

            #if DEBUG
              //serialPrint("High voltage threshold crossed, ");
              //DEBUGreadADC();
            #endif

            __delay_cycles(400);
            hibernusInitial = 1;
            //lowVoltageDetected == 1;
            P1OUT ^= BIT1;  // Toggle P1.0
            if(hibernateDoneFlagSet == 1)
            {
                Restore();
            }   
        // Handle high threshold cross
        break;
        
        case ADC12IV__ADC12LOIFG:           // Low Interrupt
             
            ADC12IFGR2 &= ~ADC12LOIFG;      // Clear low interrupt flag
            ADC12IFGR2 &= ~ADC12HIIFG;      // Clear high interrupt flag
            ADC12IFGR2 &= ~ADC12INIFG;      // Clear in interrupt flag
            
            #if DEBUG
              //serialPrint("Low voltage threshold crossed, ");
             // DEBUGreadADC();
            #endif

            ADC12IER2 &= ~ADC12LOIE; // Disable low interrupt
            ADC12IER2 |= ADC12HIIE;  // Enable high interrupt
            ADC12IER2 |= ADC12INIE;  // Enable in window interrupt
            
            P1OUT ^= BIT0;  // Toggle P1.0
            if((hibernateRecalled == 0) && (hibernusInitial == 1))
            {
                Hibernate(); 
            }
            
            if((restoreDoneSetFlag == 0) || (hibernateRecalled == 1) || (hibernusInitial == 0))
            { 
                __bis_SR_register(LPM4_bits+GIE);   // Enter LPM4 with interrupts enabled
                __no_operation();                   // For debug 
            }
            else if(additionalFlag == 1)
            {
                __bis_SR_register(GIE);         // Enter LPM4 with interrupts enabled
                __no_operation();               // For debug 
            }
          



        //Handle low threshold cross
        break;

        case ADC12IV__ADC12INIFG:           // In window interrupt
            ADC12IER2 &= ~ADC12INIE;        // Disable in window interrupt
            ADC12IFGR2 &= ~ADC12INIFG;      // Clear in interrupt flag
            ADC12IFGR2 &= ~ADC12LOIFG;      // Clear low interrupt flag
            ADC12IFGR2 &= ~ADC12HIIFG;      // Clear high interrupt flag
            ADC12IER2 |= ADC12LOIE;         // Enable low interrupt
            ADC12IER2 |= ADC12HIIE;         // Enable high interrupt


            #if DEBUG
              //serialPrint("Within window, ");
              //DEBUGreadADC();
            #endif

            break;
        case ADC12IV__ADC12IFG0:   break;   // Vector 12:  ADC12MEM0
        case ADC12IV__ADC12IFG1:   break;   // Vector 14:  ADC12MEM1
        case ADC12IV__ADC12IFG2:   break;   // Vector 16:  ADC12MEM2
        case ADC12IV__ADC12IFG3:   break;   // Vector 18:  ADC12MEM3
        case ADC12IV__ADC12IFG4:   break;   // Vector 20:  ADC12MEM4
        case ADC12IV__ADC12IFG5:   break;   // Vector 22:  ADC12MEM5
        case ADC12IV__ADC12IFG6:   break;   // Vector 24:  ADC12MEM6
        case ADC12IV__ADC12IFG7:   break;   // Vector 26:  ADC12MEM7
        case ADC12IV__ADC12IFG8:   break;   // Vector 28:  ADC12MEM8
        case ADC12IV__ADC12IFG9:   break;   // Vector 30:  ADC12MEM9
        case ADC12IV__ADC12IFG10:  break;   // Vector 32:  ADC12MEM10
        case ADC12IV__ADC12IFG11:  break;   // Vector 34:  ADC12MEM11
        case ADC12IV__ADC12IFG12:  break;   // Vector 36:  ADC12MEM12
        case ADC12IV__ADC12IFG13:  break;   // Vector 38:  ADC12MEM13
        case ADC12IV__ADC12IFG14:  break;   // Vector 40:  ADC12MEM14
        case ADC12IV__ADC12IFG15:  break;   // Vector 42:  ADC12MEM15
        case ADC12IV__ADC12IFG16:  break;   // Vector 44:  ADC12MEM16
        case ADC12IV__ADC12IFG17:  break;   // Vector 46:  ADC12MEM17
        case ADC12IV__ADC12IFG18:  break;   // Vector 48:  ADC12MEM18
        case ADC12IV__ADC12IFG19:  break;   // Vector 50:  ADC12MEM19
        case ADC12IV__ADC12IFG20:  break;   // Vector 52:  ADC12MEM20
        case ADC12IV__ADC12IFG21:  break;   // Vector 54:  ADC12MEM21
        case ADC12IV__ADC12IFG22:  break;   // Vector 56:  ADC12MEM22
        case ADC12IV__ADC12IFG23:  break;   // Vector 58:  ADC12MEM23
        case ADC12IV__ADC12IFG24:  break;   // Vector 60:  ADC12MEM24
        case ADC12IV__ADC12IFG25:  break;   // Vector 62:  ADC12MEM25
        case ADC12IV__ADC12IFG26:  break;   // Vector 64:  ADC12MEM26
        case ADC12IV__ADC12IFG27:  break;   // Vector 66:  ADC12MEM27
        case ADC12IV__ADC12IFG28:  break;   // Vector 68:  ADC12MEM28
        case ADC12IV__ADC12IFG29:  break;   // Vector 70:  ADC12MEM29
        case ADC12IV__ADC12IFG30:  break;   // Vector 72:  ADC12MEM30
        case ADC12IV__ADC12IFG31:  break;   // Vector 74:  ADC12MEM31
        case ADC12IV__ADC12RDYIFG: break;   // Vector 76:  ADC12RDY
        default: break;
    }

    restoreDoneSetFlag = 0;
    __bic_SR_register_on_exit(LPM4_bits); // Exit LPM4
}
