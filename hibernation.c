/*
*  for MSP430FR5994:
* 
* This Firmware approach is to intelligently save and restore the system's state
* in response to a power failure. This software exploits an ADC VCC Monitor.
*
*/


#include <msp430.h>
#include "hibernation.h"
#include "intrinsics.h"
#include "VCCmonitor.h"
#include "msp430fr5994.h"
#include <SerialPrint.h>

#pragma SET_DATA_SECTION(".fram_vars")

//contains FRAM and RAM addresses
unsigned long int *FRAM_write_ptr = (unsigned long int *) FRAM_START_EXPLICIT; 
unsigned long int *RAM_copy_ptr = (unsigned long int *) RAM_START; 

//pointers for FRAM and currentStackPointer
unsigned long int *FRAM_pc = (unsigned long int *) PC; 
unsigned long int *current_SP;

//This array is used to restore the register data
unsigned int Registers[532];
unsigned int *Reg_copy_ptr;

//flags to maintain down and up wakeup of system during power outage and restoration
int restoreDoneSetFlag = 0;
int hibernateDoneFlagSet = 0xA0;
int additionalFlag = 0;
int hibernateRecalled = 0;
int hibernusInitial = 0;

#pragma SET_DATA_SECTION()


const unsigned int *gen[532] = {
                0x00800, 0x00802, 0x00804, 0x00806, 0x00808, 0x0080A, 0x0080C, 0x0080E, 0x00810, 0x00812, 0x00814, 0x00816, 0x00818, 0x00820, 0x00822, 0x00824, 0x00826, 0x00828, 0x0082A, 0x0082C, 0x0082E, 0x00830, 0x00832, 0x00834, 0x00836, 0x00838, 0x0083A, 0x0083C, 0x0083E, 0x00840, 0x00842, 0x00844, 0x00846, 0x00848, 0x0084A, 0x0084C, 0x0084E, 0x00850, 0x00852, 0x00854, 0x00856, 0x00858, 0x0085A, 0x0085C, 0x0085E, 0x00860, 0x00862, 0x00864, 0x00866, 0x00868, 0x0086A, 0x0086C, 0x0086E, 0x00870, 0x00872, 0x00874, 0x00878, 0x0087A, 0x0087C, 0x0087E, 0x00880, 0x00882, 0x00884, 0x00886, 0x00888, 0x0088A, 0x0088C, 0x0088E, 0x00890, 0x00892, 0x00894, 0x00896, 0x00898, 0x0089A, 0x0089C, 0x0089E,
                0x009C0, 0x009C2, 0x009C4, 0x009C6, 0x009C8, 0x009CA, 0x009CC, 0x009CE,
                0x0043E,
                0x0047E,
                0x008C0, 0x008C2, 0x008C4, 0x008C6, 0x008CC, 0x008CE,
                0x00150, 0x00152, 0x00154, 0x00156,
                0x00980, 0x00982, 0x00984, 0x00986, 0x00988, 0x0098A, 0x0098C, 0x0098E, 0x00990, 0x00996, 0x00998, 0x0099E,
                0x00160, 0x00162, 0x00164, 0x00168, 0x00168, 0x0016C,
                0x0020E, 0x00200, 0x00202, 0x00204, 0x00206, 0x0020A, 0x0020C, 0x00216, 0x00218, 0x0021A, 0x0021C,
                0x0021E, 0x00201, 0x00203, 0x00205, 0x00207, 0x0020B, 0x0020D, 0x00217, 0x00219, 0x0021B, 0x00021D,
                0x0022E, 0x00220, 0x00222, 0x00224, 0x00226, 0x0022A, 0x0022C, 0x00236, 0x00238, 0x0023A, 0x0023C,
                0x0023E, 0x00221, 0x00223, 0x00225, 0x00227, 0x0022B, 0x0022D, 0x00237, 0x00239, 0x0023B, 0x0023D,
                0x00240, 0x00242, 0x00244, 0x00246, 0x0024A, 0x0024C, 0x00256, 0x00258, 0x0025A, 0x0025C,
                0x0024E, 0x00240, 0x00242, 0x00244, 0x00246, 0x0024A, 0x0024C, 0x00256, 0x00258, 0x0025A, 0x0025C,
                0x0025E, 0x00241, 0x00243, 0x00245, 0x00247, 0x0024B, 0x0024D, 0x00257, 0x00259, 0x0025B, 0x0025D,
                0x00260, 0x00262, 0x00264, 0x00266, 0x0026A, 0x0026C, 0x00276, 0x00278, 0x0027A, 0x0027C, 0x0026E,
                0x0027E, 0x00261, 0x00263, 0x00265, 0x00267, 0x0026B, 0x0026D, 0x00277, 0x00279, 0x0027B, 0x0027D,
                0x00280, 0x00282, 0x00284, 0x00286, 0x0028A, 0x0028C, 0x00296, 0x00298, 0x0029A, 0x0029C, 0x0028E,
                0x00320, 0x00322, 0x00324, 0x00326, 0x0032A, 0x0032C, 0x00336,
                0x00500, 0x00502, 0x00504, 0x00508, 0x0050E, 0x00510, 0x00516, 0x0051A, 0x00520, 0x00522, 0x00526, 0x0052A, 0x00530, 0x00532, 0x00536, 0x0053A, 0x00540, 0x00542, 0x00546, 0x0054A, 0x00550, 0x00552, 0x00556, 0x0055A, 0x00560, 0x00562, 0x00566, 0x0056A,
                0x00140, 0x00144, 0x00146,
                0x00A80, 0x00A84, 0x00A88, 0x00A8C, 0x00A90, 0x00A94, 0x00A98, 0x00A9C, 0x00AA8, 0x00AAC, 0x00AB0, 0x00AB4, 0x00AC0, 0x00AC4, 0x00AC8, 0x00ACC, 0x00AD0, 0x00AF0, 0x00AF4, 0x00AF8, 0x00AFC,
                0x005A0, 0x005A2, 0x005A4, 0x005A6, 0x005A8, 0x005AA, 0x005AC, 0x005AE,
                0x004C0, 0x004C2, 0x004C4, 0x004C6, 0x004C8, 0x004CA, 0x004CC, 0x004CE, 0x004D0, 0x004D2, 0x004D4, 0x004D6, 0x004D8, 0x004DA, 0x004DC, 0x004DE, 0x004E0, 0x004E2, 0x004E4, 0x004E6, 0x004E8, 0x004EA, 0x004EC,
                0x00120, 0x00120, 0x00130,
                0x00158,
                0x001B0,
                0x004A0, 0x004A2, 0x004A4 , 0x004A6, 0x004A8, 0x004AA, 0x004AC, 0x004AE, 0x004B0, 0x004B2, 0x004B4, 0x004B6, 0x004B8, 0x004BA, 0x004BC, 0x004BE, 0x004AC, 0x004AD, 0x00004B1, 0x004B3,
                0x00100, 0x00102, 0x00104,
                0x00180, 0x00186, 0x00188, 0x0018A, 0x0018C, 0x0018E, 0x0019A, 0x0019C, 0x0019E,
                0x00340, 0x00342, 0x00344, 0x00346, 0x00350, 0x00352, 0x00354, 0x00356, 0x00360, 0x0036E,
                0x00380, 0x00382, 0x00384, 0x00386, 0x00390, 0x00392, 0x00394, 0x00396, 0x003A0, 0x003AE,
                0x00400, 0x00402, 0x00404, 0x00410, 0x00412, 0x00414, 0x00420, 0x0042E,
                0x00440, 0x00442, 0x00444, 0x00450, 0x00452, 0x00454, 0x00460, 0x0046E, 
                0x007C0, 0x007C2, 0x007C4, 0x007C6, 0x007D0, 0x007D2, 0x007D4, 0x007D6, 0x007E0, 0x007EE,
                0x003C0, 0x003C2, 0x003C4, 0x003C6, 0x003C8, 0x003CA, 0x003CC, 0x003CE, 0x003D0, 0x003D2, 0x003D4, 0x003D6, 0x003D8, 0x003DA, 0x003DC, 0x003DE, 0x003E0, 0x003EE,
                0x0015C, 
                0x005C0, 0x005C2, 0x005C6, 0x005C8, 0x005CA, 0x005CC, 0x005CE, 0x005D0, 0x005D2, 0x005DA, 0x005DC, 0x005DE,
                0x005E0, 0x005E2, 0x005E6, 0x005E8, 0x005EA, 0x005EC, 0x005EE, 0x005F0, 0x005F2, 0x005FA, 0x005FC, 0x005FE,
                0x00600, 0x00602, 0x00606, 0x00608, 0x0060A, 0x0060C, 0x0060E, 0x00610, 0x00612, 0x0061A, 0x0061C, 0x0061E,
                0x00620, 0x00622, 0x00626, 0x00628, 0x0062A, 0x0062C, 0x0062E, 0x00630, 0x00632, 0x0063A, 0x0063C, 0x0063E,
                0x00640, 0x00642, 0x00646, 0x00648, 0x0064A, 0x0064C, 0x0064E, 0x00654, 0x00656, 0x00658, 0x0065A, 0x0065C, 0x0065E, 0x00660, 0x0066A, 0x0066C, 0x0066E,
                0x00680, 0x00682, 0x00686, 0x00688, 0x0068A, 0x0068C, 0x0068E, 0x00694, 0x00696, 0x00698, 0x0069A, 0x0069C, 0x0069E, 0x006A0, 0x006AA, 0x006AC, 0x006AE,
                0x006C0, 0x006C2, 0x006C6, 0x006C8, 0x006CA, 0x006CC, 0x006CE, 0x006D4, 0x006D6, 0x006D8, 0x006DA, 0x006DC, 0x006DE, 0x006E0, 0x006EA, 0x006EC, 0x006EE,
                0x00700, 0x00702, 0x00704, 0x00706, 0x00708, 0x0070A, 0x0070C, 0x0070E, 0x00714, 0x00716, 0x00718, 0x0071A, 0x0071C, 0x0071E, 0x00720, 0x0072A, 0x0072C, 0x0072E
};

unsigned int i;


void systemInitialisation(void)
{

    //System_Init
    initGPIO();
    initMSPClock();

    //create initial snapshot in fram when system just boots up
    if(hibernateDoneFlagSet == 0xA0)
        updateBlockSelectRetention();

    initVCCADC();
    initADCTimer();
  

    //serialPrintInit();

    // setupButtonInterruptP5();
    // setupButtonInterruptP6();

    // if(hibernateDoneFlagSet == 1)
    // {
    //     hibernateDoneFlagSet = 0xA0;
    //     Restore(); 
    // }

    // if(lowPowerMode == 1)
    // { 
    //     __bis_SR_register(LPM4_bits+GIE);   // Enter LPM4 with interrupts enabled
    //     __no_operation();                   // For debug 
    // }
}

void initGPIO(void)
{

    //GPIO configuration
    P1OUT &= ~(0xFF);
    P2OUT &= ~(0xBF);
    P3OUT &= ~(0xFB);
    P4OUT &= ~(0xFF);
    PJOUT &= ~(0xFF); // Configure the pull-down resistor

    //TRIS REGISTERS
    P1DIR |= 0xFF;
    P2DIR |= 0x3F;
    P3DIR |= 0xFB;
    P4DIR |= 0xFF;
    PJDIR |= 0xFF;  // Direction = output

}


void initMSPClock(void)
{

    //setting core frequency
    CSCTL0_H = 0xA5;                            // Unlock register
    CSCTL1 &= ~(DCORSEL);                       //Set max. DCO setting. 5.33MH in this case
    CSCTL1 |= DCOFSEL0 + DCOFSEL1;              //DCO frequency select register 8MHz
    CSCTL2 = SELA_1 + SELS_3 + SELM_3;          // Selects the ACLK, MCLK AND SMCLK sources register
                                                // Set ACLK = VLO; MCLK = DCO; SMCLK = DCO;
    CSCTL3 = DIVA_0 + DIVS_0 + DIVM_0;          // MCLK, SMCLK and ACLK source divider/prescaler register.
    CSCTL0_H = 0x01;                            // Lock Register
}

void Hibernate(void)
{
    //Added for Debugging and measuring hibernus speed
    // P3OUT &= ~LED1;  // Set P3.7 low
    // P3OUT ^= LED1;  // Set P3.7 high

    //copy Core registers to FRAM
    asm(" MOVA R1,&0xEDF4");
    asm(" MOVA R2,&0xEDF8");
    asm(" MOVA R3,&0xEDFC");
    asm(" MOVA R4,&0xEE00");
    asm(" MOVA R5,&0xEE04");
    asm(" MOVA R6,&0xEE08");
    asm(" MOVA R7,&0xEE0C");
    asm(" MOVA R8,&0xEE10");
    asm(" MOVA R9,&0xEE14");
    asm(" MOVA R10,&0xEE18");
    asm(" MOVA R11,&0xEE1C");
    asm(" MOVA R12,&0xEE20");
    asm(" MOVA R13,&0xEE24");
    asm(" MOVA R14,&0xEE28");
    asm(" MOVA R15,&0xEE2C");
    //uptohere downtime executes
    
    //PC
    current_SP = (unsigned long int *)__get_SP_register();
    *FRAM_pc = *current_SP;

    //added for downtime debugging
    //P3OUT |= LED1;  // Set P3.7 high 
    
    // copy all the RAM Segmetns onto the FRAM
    SaveRAMSnapshot();
    
    //save all GP registers to FRAM
    SaveGPRegister();

    // copy all the general registers onto the FRAM
    restoreDoneSetFlag = 0;
    additionalFlag = 0;

    hibernateRecalled = 1;
    hibernateDoneFlagSet = 1;
    
    //Added for Debugging and measuring hibernus speed
    //P3OUT ^= LED1;  // Set P3.7 low
}

void Restore(void)
{
    
    // Restore all general purpose Register contents
    RestoreGPRegisters();
    
    // Restore RAM
    FRAM_write_ptr = (unsigned long int *) FRAM_START_EXPLICIT;

    //Leave the space for the core registers
    for ( i = 0; i < 16; i++)
    {
        *FRAM_write_ptr++;
    }

    RAM_copy_ptr = (unsigned long int *) RAM_START;

    while(RAM_copy_ptr < (unsigned long int *) (RAM_END)) 
    {
        *RAM_copy_ptr++ = *FRAM_write_ptr++;
    }

    //copy Core registers contents from FRAM back to CPU
    asm(" MOVA &0xEDF4,R1");
    asm(" MOVA &0xEDF8,R2");
    asm(" MOVA &0xEDFC,R3");
    asm(" MOVA &0xEE00,R4");
    asm(" MOVA &0xEE04,R5");
    asm(" MOVA &0xEE08,R6");
    asm(" MOVA &0xEE0C,R7");
    asm(" MOVA &0xEE10,R8");
    asm(" MOVA &0xEE14,R9");
    asm(" MOVA &0xEE18,R10");
    asm(" MOVA &0xEE1C,R11");
    asm(" MOVA &0xEE20,R12");
    asm(" MOVA &0xEE24,R13");
    asm(" MOVA &0xEE28,R14");
    asm(" MOVA &0xEE2C,R15");

  
    *current_SP = *FRAM_pc; 

    // //Debugging: Restore finished
    // P1OUT &= ~BIT3;
    
    restoreDoneSetFlag = 1;  
    additionalFlag = 1;

    hibernateRecalled = 0;

    
    // //For Debugging: System active
    // P2DIR |= BIT6;
    // P2OUT |= BIT6;
    //restore done reset hibernate flag
    hibernateDoneFlagSet = 0xA0;

    ADC12IFGR2 &= ~ADC12HIIFG;   // Clear high interrupt flag
    ADC12IFGR2 &= ~ADC12LOIFG;   // Clear low interrupt flag
    ADC12IFGR2 &= ~ADC12INIFG;  // Clear in interrupt flag

    ADC12IER2 &= ~ADC12HIIE;    // Disable high interrupt
    ADC12IER2 |= ADC12LOIE;     // Enable low interrupt
    ADC12IER2 |= ADC12INIE;     // Enable in window interrupt


    //added for debugging
    //P3OUT ^= LED1;  // Set P3.7 low


    __bis_SR_register(GIE);        //interrupts enabled
    __no_operation();             // For debug
   
}


void SaveRAMSnapshot(void)
{

    FRAM_write_ptr = (unsigned long int *) FRAM_START_EXPLICIT;
        
    // for ( i = 0; i < 16; i++)
    // {
    //     *FRAM_write_ptr++;
    // }
    FRAM_write_ptr += 16;

    //copy all RAM onto the FRAM
    RAM_copy_ptr = (unsigned long int *) RAM_START;

    while(RAM_copy_ptr <= (unsigned long int *) (RAM_END)) 
    {
        if(*FRAM_write_ptr != *RAM_copy_ptr)
        {
            *FRAM_write_ptr++ = *RAM_copy_ptr++; 
        }
        FRAM_write_ptr++;
        RAM_copy_ptr++;
    }

    // RAM_copy_ptr = (unsigned long int *) RAM_START;
    // while(RAM_copy_ptr <= (unsigned long int *) (RAM_END)) 
    // {
    //     *FRAM_write_ptr++ = *RAM_copy_ptr++;
    // }
    
}

void SaveGPRegister(void)
{
    // // save GP registers data into registers array in fram.vars
    // for(i = 0; i < 532; i++)
    // {
    //     Reg_copy_ptr = (unsigned  int *)gen[i];
    //     Registers[i] = *Reg_copy_ptr;
    // }

    //selectively save data into registers array since initial value already created during INIT
    for(i = 0; i < 532; i++)
    {
        if(Registers[i] != *gen[i])
            Registers[i] = *gen[i];     
    }
}

void RestoreGPRegisters(void)
{

    //unlock registers
    MPUCTL0_H = 0xA5;
    PMMCTL0_H = 0xA5;
    FRCTL0_H = 0xA5;
    CSCTL0_H = 0xA5;

    //Restore registers
    for(i = 0; i < 3; ++i)
    {
        Reg_copy_ptr = (unsigned  int *)gen[i];
        *Reg_copy_ptr = Registers[i];
    }
    
    for(i = 4; i < 6; ++i)
    {
        Reg_copy_ptr = (unsigned  int *)gen[i];
        *Reg_copy_ptr = Registers[i];
    }

    for(i = 6; i < 13; ++i)
    {
        Reg_copy_ptr = (unsigned  int *)gen[i];
        *Reg_copy_ptr = Registers[i];
    }

    for(i = 13; i < 34; ++i)
    {
        Reg_copy_ptr = (unsigned  int *)gen[i];
        *Reg_copy_ptr = Registers[i];
    }

    for(i = 34; i < 47; ++i)
    {
        Reg_copy_ptr = (unsigned  int *)gen[i];
        *Reg_copy_ptr = Registers[i];
    }
    for(i = 47; i < 169; ++i)
    {
        if((i != 108) && (i != 167) )
        {
            Reg_copy_ptr = (unsigned  int *)gen[i];
            *Reg_copy_ptr = Registers[i];
        }
    }   
    for(i = 169; i < 230; ++i)
    {
        Reg_copy_ptr = (unsigned  int *)gen[i];
        *Reg_copy_ptr = Registers[i];
    }
    for(i = 230; i < 314; ++i)
    {
        
        if((i != 258) && (i != 282) && (i != 313))
        {
            Reg_copy_ptr = (unsigned  int *)gen[i];
            *Reg_copy_ptr = Registers[i];
        }
    }
    for(i = 314; i < 400; ++i)
    {
        if((i != 314) )
        {
            Reg_copy_ptr = (unsigned  int *)gen[i];
            *Reg_copy_ptr = Registers[i];
        }
    }
    for(i = 400; i <= 532; ++i)
    {
        if((i != 414) && (i != 446))
        {
            Reg_copy_ptr = (unsigned  int *)gen[i];
            *Reg_copy_ptr = Registers[i];
        }
        
    }
    
    MPUCTL0_H = 0x01;
    PMMCTL0_H = 0x01;
    FRCTL0_H = 0x01;
    FRCTL0_H = 0x01;
    CSCTL0_H = 0x01;
  
}


void updateBlockSelectRetention()
{
    //update FRAM with RAM snapshot
    FRAM_write_ptr = (unsigned long int *) FRAM_START_EXPLICIT;

    for ( i = 0; i < 16; i++)
    {
        *FRAM_write_ptr++;
    }

    while(RAM_copy_ptr <= (unsigned long int *) (RAM_END)) 
    {
        *FRAM_write_ptr++ = *RAM_copy_ptr++;
    }

    //update initial GP registers in FRAM.vars
    for(i = 0; i < 532; i++)
    {
        Reg_copy_ptr = (unsigned  int *)gen[i];
        Registers[i] = *Reg_copy_ptr;
    }
}


void setupButtonInterruptP5() 
{
    // Configure P5.5 as input with pull-up resistor
    P5DIR &= ~BIT5;    // Set P5.5 as input
    P5REN |= BIT5;     // Enable pull-up/pull-down resistor on P5.5
    P5OUT |= BIT5;     // Select pull-up resistor on P5.5
    
    // Configure interrupt
    P5IES |= BIT5;     // Select falling edge (change to P5IES &= ~BIT5 for rising edge)
    P5IFG &= ~BIT5;    // Clear interrupt flag for P5.5
    P5IE |= BIT5;      // Enable interrupt on P5.5
    
    // Enable global interrupts
    __bis_SR_register(GIE);  // Enable global interrupts
}

void setupButtonInterruptP6() 
{
    // Configure P5.6 as input with pull-up resistor
    P5DIR &= ~BIT6;    // Set P5.6 as input
    P5REN |= BIT6;     // Enable pull-up/pull-down resistor on P5.6
    P5OUT |= BIT6;     // Select pull-up resistor on P5.6
    
    // Configure interrupt
    P5IES |= BIT6;     // Select falling edge (change to P5IES &= ~BIT6 for rising edge)
    P5IFG &= ~BIT6;    // Clear interrupt flag for P5.6
    P5IE |= BIT6;      // Enable interrupt on P5.6
    
    // Enable global interrupts
    __bis_SR_register(GIE);  // Enable global interrupts
}

// Port 5 interrupt service routine
#pragma vector=PORT5_VECTOR
__interrupt void Port_5(void) 
{
    if (P5IFG & BIT5) //Hibernate
    {
        // Handle button press event here
        P1OUT ^= BIT0;  // Toggle P1.0
        // Clear the interrupt flag

        Hibernate(); 
        if(restoreDoneSetFlag == 0)
        {
            //hibernateDoneFlagSet = 1;        
            __bis_SR_register(LPM4_bits+GIE);   // Enter LPM4 with inetrrupts enabled
            __no_operation();                   // For debug 
        }
        P5IFG &= ~BIT5;
    }

    if (P5IFG & BIT6) //Restore
    {
        // Handle button press event here
        P1OUT ^= BIT1;  // Toggle P1.0

        if(hibernateDoneFlagSet == 1)
        {
            __delay_cycles(1000);
            Restore();
        }
        // Clear the interrupt flag
        P5IFG &= ~BIT6;
    }

    restoreDoneSetFlag = 0;
    __bic_SR_register_on_exit(LPM4_bits); // Exit LPM4
    
}

