
/*
* Hibernus for MSP430FR5994:
* 
* Hibernus: Software-based approach to intelligently saves and restore the system's state
* in response to a power failure. This software exploits an ADC VCC Monitor and selectively restores allocated spaces and restores data in the RAM
*
*/

#include "hibernation.h"
#include <msp430.h>
#include "SerialPrint.h"


int main(void) 
{
	//For Debugging: System active
	P2DIR |= BIT6;
	P2OUT |= BIT6;

    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

    // Clear the LOCKLPM5 bit to enable GPIO
    PM5CTL0 &= ~LOCKLPM5;

    systemInitialisation();

    P3DIR |= (LED1 | LED2 | LED3 | LED4 );

    P1DIR |= BIT0;      // Set P1.0 as output (BIT0 corresponds to P1.0)
    P1OUT &= ~BIT0;     // Initialize the LED to be off

    P1DIR |= BIT1;      // Set P1.0 as output (BIT0 corresponds to P1.0)
    P1OUT &= ~BIT1;     // Initialize the LED to be off
    //__delay_cycles(10000);

    
    //APPLICATION CODE
    while(1)
    {
        // // Blink LED on P3.7
        // P3OUT |= LED1;  // Set P3.7 high
        // __delay_cycles(10000000);
        // P3OUT &= ~LED1;  // Set P3.7 low
        // __delay_cycles(10000000);
        
        // Blink LED on P3.6
        P3OUT |= LED2;  // Set P3.6 high
        __delay_cycles(10000000);
        P3OUT &= ~LED2;  // Set P3.6 low
       __delay_cycles(10000000);

        // Blink LED on P3.5
        P3OUT |= LED3;  // Set P3.5 high
        __delay_cycles(10000000);
        P3OUT &= ~LED3;  // Set P3.5 low
       __delay_cycles(10000000);
        
        // Blink LED on P3.4
        P3OUT |= LED4;  // Set P3.4 high
        __delay_cycles(10000000);
        P3OUT &= ~LED4;  // Set P3.4 low
        __delay_cycles(10000000);
    }
}

